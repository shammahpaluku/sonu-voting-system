# SONU Concurrent UDP Server - Technical Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Main Algorithm](#main-algorithm)
4. [Function Documentation](#function-documentation)
5. [Concurrency Model](#concurrency-model)
6. [Data Flow](#data-flow)
7. [UDP vs TCP Comparison](#udp-vs-tcp-comparison)

---

## Overview

The SONU (Student Organization of Nairobi University) Voting System UDP server is a concurrent, connectionless server that manages student elections. Unlike the TCP version, it uses UDP datagrams for communication and POSIX message queues for master-slave coordination.

**Key Features:**
- Concurrent datagram handling using fork-based master-slave model
- POSIX message queue for passing datagrams from master to slaves
- Stateless processing (each datagram is independent)
- Authentication for voters and administrators
- Position and candidate management
- Voting system with real-time tallying
- File-based data persistence

---

## Architecture

### Server Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Master Process                          │
│  - Listens on UDP port 9100                                 │
│  - Receives datagrams from any client                       │
│  - Pushes datagrams to POSIX message queue                  │
│  - Forks slave processes for each datagram                  │
│  - Reaps zombie processes via SIGCHLD handler              │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ fork()
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     Slave Process                            │
│  - Reads one datagram from message queue                    │
│  - Resets auth state (stateless UDP)                        │
│  - Processes command via dispatch_command()                 │
│  - Saves data to files                                      │
│  - Exits immediately after handling one datagram            │
└─────────────────────────────────────────────────────────────┘
```

### POSIX Message Queue

The message queue (`/sonu_mq`) serves as the communication channel between master and slave processes:

```
┌─────────────┐     mq_send()     ┌──────────────┐     mq_receive()     ┌─────────────┐
│   Master    │ ──────────────────>│ Message Queue│ ──────────────────>│   Slave     │
│  Process    │   DgramMsg struct  │  /sonu_mq    │   DgramMsg struct  │  Process    │
└─────────────┘                    └──────────────┘                    └─────────────┘
```

**DgramMsg Structure:**
```c
typedef struct {
    char cmd_buf[CMD_BUF_LEN];          // Command text
    struct sockaddr_in client_addr;      // Client's IP and port
} DgramMsg;
```

### Data Storage

All data is persisted in text files in the `server/data/` directory:
- `voters.txt` - Registered voters
- `candidates.txt` - Approved candidates
- `positions.txt` - Election positions
- `votes.txt` - Cast votes
- `status.txt` - Election status (OPEN/CLOSED)
- `applications.txt` - Candidate applications

---

## Main Algorithm

### `main()` Function

**Purpose:** Initialize the server and enter the master receive loop.

**Algorithm:**
```
1. Initialize data files (fh_init_files)
2. Create UDP listening socket (nh_server_init)
3. Install SIGCHLD signal handler
   - Prevents zombie slave processes
   - Uses SA_RESTART to restart interrupted system calls
   - Uses SA_NOCLDWAIT to automatically reap child processes
4. Create POSIX message queue
   - Set queue attributes (max messages, message size)
   - Open with O_CREAT | O_RDWR
   - Permissions: 0666
5. Enter infinite receive loop:
   a. Receive UDP datagram (nh_recv_from)
      - Stores command text in msg.cmd_buf
      - Stores client address in msg.client_addr
   b. Push datagram to message queue (mq_send)
   c. Fork a child process
   d. If fork fails:
      - Log error
      - Continue to next iteration
   e. If child process (pid == 0):
      - Read datagram from message queue (mq_receive)
      - Reset auth state (auth_logout) - stateless UDP
      - Process command (dispatch_command)
      - Close message queue
      - Exit (handles exactly one datagram)
   f. If parent process (pid > 0):
      - Loop back immediately to receive next datagram
6. Cleanup (unreachable):
   - Close message queue
   - Unlink message queue
   - Close socket
```

**Key Design Decisions:**
- **Message queue:** Enables master to pass datagram context (command + address) to slave
- **Stateless processing:** Each slave resets auth state since UDP is connectionless
- **One datagram per slave:** Slave exits after processing one command
- **Non-blocking master:** Master immediately loops back after forking

---

## Function Documentation

### Helper Functions

#### `replace_underscores(char *str)`
**Purpose:** Replace underscores with spaces in-place.

**Algorithm:**
```
Iterate through string:
  If character is '_':
    Replace with ' '
```

**Use Case:** Converts wire format (e.g., "Vice_Chairperson") to display format.

#### `replace_spaces(char *str)`
**Purpose:** Replace spaces with underscores in-place.

**Algorithm:**
```
Iterate through string:
  If character is ' ':
    Replace with '_'
```

**Use Case:** Converts display format to wire format for transmission.

---

### Command Handlers

**Note:** All command handlers in the UDP version take a socket descriptor and a `struct sockaddr_in*` for the client address, unlike the TCP version which only takes a file descriptor.

#### `cmd_login(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Authenticate a voter using ID and password.

**Algorithm:**
```
1. Parse args: "voter_id password"
2. Validate both tokens are present
3. Call auth_voter_login(voter_id, password)
4. If successful:
   - Send "OK <voter_name>" to client via nh_send_to()
5. If failed:
   - Send "ERR_NOT_FOUND" if voter doesn't exist
   - Send "ERR_AUTH_FAIL" if password incorrect
```

**UDP-Specific:** Uses `nh_send_to()` which requires the client address to send the response.

#### `cmd_admin_login(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Authenticate the administrator.

**Algorithm:**
```
1. Parse args: "username password"
2. Validate both tokens are present
3. Call auth_admin_login(username, password)
4. If successful:
   - Send "OK" to client
5. If failed:
   - Send "ERR_AUTH_FAIL"
```

#### `cmd_logout(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Clear the current authentication session.

**Algorithm:**
```
1. Call auth_logout() to clear session state
2. Send "OK" to client
```

**Stateless Note:** In UDP, logout is per-datagram. The next datagram from the same client will require re-authentication.

#### `cmd_status(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Retrieve the current election status.

**Algorithm:**
```
1. Call admin_get_election_status()
2. If successful:
   - Send status ("OPEN" or "CLOSED") to client
3. If failed:
   - Send "ERR_FILE"
```

---

### Voter Management

#### `cmd_self_register(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Allow a voter to self-register with a name and password.

**Algorithm:**
```
1. Parse args: "name password"
2. Validate both tokens are present
3. Replace underscores in name with spaces
4. Get next available voter ID (voter_next_id)
5. Call voter_register(name, password)
6. If successful:
   - Send "OK <voter_id>" to client
7. If failed:
   - Send "ERR_DUPLICATE" if name already exists
   - Send "ERR_FULL" if voter limit reached
   - Send "ERR_UNKNOWN" for other errors
```

#### `cmd_register_voter(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin registers a voter (deprecated, uses same logic as self-register).

**Algorithm:** Same as `cmd_self_register`.

---

### Position Management

#### `cmd_add_position(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin adds a new election position.

**Algorithm:**
```
1. Validate args is not empty
2. Replace underscores with spaces in position name
3. Call pos_add(name)
4. If successful:
   - Send "OK <position_id>" to client
5. If failed:
   - Send "ERR_DUPLICATE" if position exists
   - Send "ERR_FILE" for other errors
```

#### `cmd_list_positions(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** List all available election positions.

**Algorithm:**
```
1. Call pos_get_all() to retrieve all positions
2. If no positions or error:
   - Send "ERR_EMPTY"
   - Return
3. For each position:
   - Copy position name
   - Replace spaces with underscores (wire format)
   - Send "id name" to client
4. Send "END" marker
```

**Output Format:** Multi-line response ending with "END".

---

### Candidate Management

#### `cmd_apply_candidate(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Voter applies to become a candidate for a position.

**Algorithm:**
```
1. Check if user is logged in (auth_is_logged_in)
2. Parse args: "position_id"
3. Validate position exists (pos_get_by_id)
4. Get current voter info (voter_get_by_id)
5. Check if voter already has a pending application
6. If not:
   - Create application (application_add)
   - Send "OK"
7. If duplicate:
   - Send "ERR_DUPLICATE"
```

#### `cmd_register_cand(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin directly registers a candidate (bypasses application).

**Algorithm:**
```
1. Parse args: "name position_id" (find last space for position_id)
2. Replace underscores in name with spaces
3. Call cand_register(name, position_id)
4. If successful:
   - Send "OK <candidate_id>"
5. If failed:
   - Send "ERR_NOT_FOUND" if position doesn't exist
   - Send "ERR_DUPLICATE" if candidate exists
   - Send "ERR_FILE" for other errors
```

#### `cmd_list_cands(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** List candidates for a specific position.

**Algorithm:**
```
1. Parse args: "position_id"
2. Call cand_get_for_position(position_id)
3. If no candidates or error:
   - Send "ERR_EMPTY"
   - Return
4. For each candidate:
   - Copy candidate name
   - Replace spaces with underscores
   - Send "id name" to client
5. Send "END" marker
```

#### `cmd_list_applications(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin lists pending candidate applications.

**Algorithm:**
```
1. Check if user is admin (auth_is_admin)
2. Call application_get_pending() to get pending applications
3. If no applications:
   - Send "ERR_EMPTY"
   - Return
4. For each application:
   - Send "id voter_name position_id position_name"
5. Send "END" marker
```

#### `cmd_approve_application(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin approves a candidate application.

**Algorithm:**
```
1. Check if user is admin
2. Parse args: "application_id"
3. Find application by ID (must be pending)
4. If not found:
   - Send "ERR_NOT_FOUND"
   - Return
5. Register as candidate (cand_register)
6. If successful:
   - Update application status to approved
   - Send "OK"
7. If failed:
   - Send appropriate error (duplicate, full, etc.)
```

#### `cmd_reject_application(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin rejects a candidate application.

**Algorithm:**
```
1. Check if user is admin
2. Parse args: "application_id"
3. Update application status to rejected (application_update_status)
4. If successful:
   - Send "OK"
5. If failed:
   - Send "ERR_NOT_FOUND"
```

---

### Voting Operations

#### `cmd_open_voting(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin opens the election for voting.

**Algorithm:**
```
1. Check if user is admin
2. Call admin_open_voting()
3. If successful:
   - Send "OK"
4. If failed:
   - Send "ERR_FILE"
```

#### `cmd_close_voting(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin closes the election (ends voting period).

**Algorithm:**
```
1. Check if user is admin
2. Call admin_close_voting()
3. If successful:
   - Send "OK"
4. If failed:
   - Send "ERR_FILE"
```

#### `cmd_cast_vote(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Voter casts a vote for a candidate.

**Algorithm:**
```
1. Parse args: "position_id candidate_id"
2. Validate both tokens are present
3. Get current voter ID from session
4. If not logged in:
   - Send "ERR_AUTH_FAIL"
   - Return
5. Call voting_cast_vote(voter_id, position_id, candidate_id)
6. Handle result:
   - SUCCESS: Send "OK"
   - ERR_CLOSED: Send "ERR_CLOSED" (voting not open)
   - ERR_VOTED: Send "ERR_VOTED" (already voted for this position)
   - ERR_NOT_FOUND: Send "ERR_NOT_FOUND" (invalid candidate)
   - Otherwise: Send "ERR_FILE"
```

#### `cmd_cast_all_votes(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Voter casts multiple votes at once (UDP-specific optimization).

**Algorithm:**
```
1. Validate args is not empty
2. Get current voter ID from session
3. If not logged in:
   - Send "ERR_AUTH_FAIL"
   - Return
4. Initialize success_count and error_count
5. Parse position:candidate pairs (format: "pos_id:cand_id pos_id:cand_id ...")
6. For each pair:
   - Parse position_id and candidate_id
   - Call voting_cast_vote()
   - If successful: increment success_count
   - If failed: increment error_count
7. Send response:
   - If all successful: "OK"
   - If partial success: "OK PARTIAL"
   - If all failed: "ERR_FILE"
```

**Use Case:** Allows clients to vote for all positions in a single datagram, reducing network overhead.

#### `cmd_results(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin views election results (only after voting closes).

**Algorithm:**
```
1. Check if user is admin
2. Get election status
3. If not CLOSED:
   - Send "ERR_CLOSED"
   - Return
4. Compute tally (tally_compute)
5. If no votes:
   - Send "ERR_EMPTY"
   - Return
6. For each position:
   - Send "POSITION <name>"
   - For each candidate in position:
     - Send "CANDIDATE <name> <votes> <percentage>"
   - Determine winner (most votes)
   - Send "WINNER <name> <votes>"
7. Send voter turnout percentage
8. Send "END" marker
```

#### `cmd_reset(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Admin resets all election data (clears votes, candidates, etc.).

**Algorithm:**
```
1. Check if user is admin
2. Call admin_reset_direct()
3. If successful:
   - Send "OK"
4. If failed:
   - Send "ERR_FILE"
```

---

### Session Management

#### `cmd_quit(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Client requests to disconnect (no-op in UDP).

**Algorithm:**
```
1. Send "OK" to client
2. Set last_was_quit = 1 (signals command processed)
```

**UDP Note:** Since UDP is connectionless, QUIT doesn't actually disconnect anything. It's a protocol convention.

#### `dispatch_command(int sock, struct sockaddr_in *client_addr, char *cmd_buf)`
**Purpose:** Parse command and route to appropriate handler.

**Algorithm:**
```
1. Copy command buffer (to avoid modifying original)
2. Extract verb (first word before space)
3. Extract args (remainder after verb)
4. Reset last_was_quit flag
5. Compare verb against known commands:
   - LOGIN → cmd_login
   - ADMIN_LOGIN → cmd_admin_login
   - LOGOUT → cmd_logout
   - STATUS → cmd_status
   - SELF_REGISTER → cmd_self_register
   - APPLY_CANDIDATE → cmd_apply_candidate
   - LIST_APPLICATIONS → cmd_list_applications
   - APPROVE_APPLICATION → cmd_approve_application
   - REJECT_APPLICATION → cmd_reject_application
   - ADD_POSITION → cmd_add_position
   - LIST_POSITIONS → cmd_list_positions
   - REGISTER_CAND → cmd_register_cand
   - LIST_CANDS → cmd_list_cands
   - OPEN_VOTING → cmd_open_voting
   - CLOSE_VOTING → cmd_close_voting
   - CAST_VOTE → cmd_cast_vote
   - CAST_ALL_VOTES → cmd_cast_all_votes (UDP-specific)
   - RESULTS → cmd_results
   - RESET → cmd_reset
   - QUIT → cmd_quit
6. If unknown verb:
   - Send "ERR_UNKNOWN"
7. Log command with client address
```

#### `sigchld_handler(int sig)`
**Purpose:** Signal handler to reap zombie child processes.

**Algorithm:**
```
1. Call waitpid(-1, NULL, WNOHANG) in a loop
2. Continue until no more child processes to reap
3. Return
```

---

## Concurrency Model

### Master-Slave Architecture with Message Queue

**Master Process:**
- Single process that runs for the lifetime of the server
- Receives UDP datagrams from any client
- Pushes datagram (command + client address) to message queue
- Forks a slave for each datagram
- Does not process any commands
- Immediately loops back to receive next datagram

**Slave Process:**
- One slave per datagram (not per client)
- Reads one datagram from message queue
- Resets authentication state (stateless)
- Processes exactly one command
- Sends response directly to client using stored address
- Exits immediately after processing

### Message Queue Communication

**Why Message Queue?**
- UDP is connectionless, so the master cannot pass the socket to the slave
- The slave needs both the command text AND the client address to respond
- Message queue provides a clean IPC mechanism for passing this data

**Queue Attributes:**
- Name: `/sonu_mq`
- Max messages: 10
- Message size: sizeof(DgramMsg) = 1024 + 16 = 1040 bytes
- Permissions: 0666 (read/write for all)

### Stateless Processing

**Key Difference from TCP:**
- TCP: One slave per client session, maintains state across multiple commands
- UDP: One slave per datagram, no state between datagrams

**Implications:**
- Each slave calls `auth_logout()` before processing
- Clients must re-authenticate for each command (or include credentials)
- No session management
- Simpler but more chatty protocol

### Process Isolation

Each slave process has:
- **Independent memory space:** No shared variables
- **Independent file descriptors:** Each has its own socket copy
- **Independent authentication state:** Each datagram is independent

### Zombie Process Prevention

Same as TCP version:
```
sigchld_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
```

**Flags:**
- `SA_RESTART`: Automatically restart interrupted system calls
- `SA_NOCLDWAIT`: Kernel automatically reaps children

---

## Data Flow

### Typical Datagram Exchange

```
Client                    Master Process           Message Queue           Slave Process
  |                           |                          |                     |
  |-- UDP Datagram --------->|                          |                     |
  |   (LOGIN 1 pass)         |                          |                     |
  |                           |-- mq_send() ----------->|                     |
  |                           |                          |                     |
  |                           |-- fork() -------------->|                     |
  |                           |                          |                     |
  |                           |                          |-- mq_receive() ---->|
  |                           |                          |                     |
  |                           |                          |                     |-- auth_logout()
  |                           |                          |                     |-- auth_voter_login()
  |                           |                          |                     |-- read voters.txt
  |                           |                          |                     |
  |<-- UDP Response ---------|--------------------------|<-- nh_send_to() ---|
  |   (OK John_Doe)          |                          |                     |
  |                           |                          |                     |-- exit()
  |                           |                          |                     |
```

### Multiple Concurrent Datagrams

```
Time →
      Datagram 1     Datagram 2     Datagram 3
Client1 ──────┐
              │
Master ───────┼─── MQ ──── Slave1 ──┐
              │                            │
Client2 ──────┼─── MQ ──── Slave2 ──┤     │
              │                            │     │
Client3 ──────┼─── MQ ──── Slave3 ──┼─────┼─────┘
              │                            │
Master ───────┴────────────────────────────┴─────> Ready for next
```

### File Access Pattern

Same as TCP version:
- **Read operations:** Load entire file into memory, no locking needed
- **Write operations:** Rewrite entire file, requires file locking (flock)

---

## UDP vs TCP Comparison

### Architecture Differences

| Aspect | TCP Server | UDP Server |
|--------|-----------|------------|
| Connection | Connection-oriented (TCP) | Connectionless (UDP) |
| Slave Lifetime | Per client session | Per datagram |
| State Management | Maintains session state | Stateless (reset per datagram) |
| IPC Mechanism | Socket inheritance | POSIX message queue |
| Client Tracking | Socket descriptor | Client address (IP:port) |
| Multi-command Support | Yes (session loop) | No (one command per datagram) |

### Code Differences

**TCP Command Handler:**
```c
void cmd_login(int client_fd, char *args) {
    // Process login
    nh_send_line(client_fd, "OK");
}
```

**UDP Command Handler:**
```c
void cmd_login(int sock, struct sockaddr_in *client_addr, char *args) {
    // Process login
    nh_send_to(sock, "OK", client_addr);
}
```

**TCP Session Loop:**
```c
void handle_session(int client_fd) {
    while (1) {
        nh_recv_line(client_fd, cmd_buf, CMD_BUF_LEN);
        dispatch_command(client_fd, cmd_buf);
        if (last_was_quit) break;
    }
}
```

**UDP Main Loop:**
```c
while (1) {
    nh_recv_from(server_fd, msg.cmd_buf, CMD_BUF_LEN, &msg.client_addr);
    mq_send(mq, (char *)&msg, sizeof(DgramMsg), 0);
    fork();
    if (pid == 0) {
        mq_receive(mq, (char *)&slave_msg, sizeof(DgramMsg), NULL);
        auth_logout(); // Stateless reset
        dispatch_command(server_fd, &slave_msg.client_addr, slave_msg.cmd_buf);
        exit(0); // One datagram only
    }
}
```

### Performance Characteristics

| Metric | TCP | UDP |
|--------|-----|-----|
| Connection Overhead | High (3-way handshake) | None |
| Per-Command Overhead | Low (session established) | High (fork per command) |
| Latency | Higher (connection setup) | Lower (no connection) |
| Throughput | Higher (session reuse) | Lower (fork overhead) |
| Scalability | Limited by connections | Limited by fork rate |
| Reliability | Built-in (TCP) | Application-level |

### Use Cases

**Choose TCP when:**
- Multiple commands per session
- Session state is important
- Reliable delivery is critical
- Higher throughput needed

**Choose UDP when:**
- Single command per request
- Stateless operation is acceptable
- Low latency is critical
- Simple request-response pattern

---

## Error Handling

### Error Codes

Same as TCP version:
| Code | Value | Meaning |
|------|-------|---------|
| SUCCESS | 0 | Operation succeeded |
| ERR_FILE | -1 | File I/O error |
| ERR_NOT_FOUND | -2 | Record not found |
| ERR_DUPLICATE | -3 | Duplicate record |
| ERR_AUTH_FAIL | -4 | Authentication failed |
| ERR_VOTED | -5 | Already voted |
| ERR_CLOSED | -6 | Election closed |
| ERR_FULL | -7 | Capacity limit reached |
| ERR_CONN | -8 | Connection error |
| ERR_UNKNOWN | -9 | Unknown error |

### Error Response Format

All errors are sent as plain text datagrams:
```
ERR_AUTH_FAIL
ERR_NOT_FOUND
ERR_DUPLICATE
ERR_FILE
ERR_UNKNOWN
```

---

## Security Considerations

### Password Storage
- Passwords are hashed using SHA-256
- Plain text passwords are never stored
- Hash comparison for authentication

### Access Control
- Admin commands require `auth_is_admin()` check
- Voter commands require `auth_is_logged_in()` check
- Session state is per-datagram (stateless)

### Input Validation
- Position names validated for alphabetic characters only
- Voter ID and candidate ID validated as integers
- Buffer sizes enforced (strncpy with length limits)

### UDP-Specific Concerns
- No built-in authentication (IP spoofing possible)
- No built-in encryption (plaintext on wire)
- No built-in rate limiting (DoS vulnerability)
- Message queue permissions (0666 allows any local process to read)

**Recommendations for Production:**
- Implement application-level authentication tokens
- Add rate limiting per IP address
- Use message queue with stricter permissions
- Consider DTLS for encrypted UDP

---

## Performance Characteristics

### Scalability
- **Datagram handling:** Limited by fork rate (~100-1000/sec)
- **Memory usage:** ~2MB per slave process (short-lived)
- **Message queue:** 10 messages max (configurable)
- **File I/O:** Each slave reads/writes independently

### Bottlenecks
- **File locking:** Contention on writes (mitigated by flock)
- **Process creation:** fork() overhead per datagram
- **Message queue:** Limited to 10 pending messages
- **File loading:** Each slave loads entire files

### Optimization Opportunities
- Use thread pool instead of fork (reduce overhead)
- Use shared memory for read-only data
- Cache frequently accessed data
- Increase message queue size
- Implement connection pooling (if switching to TCP)

---

## Testing Recommendations

### Unit Tests
- Test each command handler independently
- Test error conditions (invalid input, missing auth)
- Test file I/O operations
- Test message queue operations

### Integration Tests
- Test complete datagram exchanges
- Test concurrent access (multiple clients)
- Test admin workflow (approve applications, results)
- Test stateless behavior (re-authentication)

### Load Tests
- Test with high datagram rate
- Test message queue under contention
- Monitor fork rate and process creation
- Monitor memory usage

### UDP-Specific Tests
- Test lost datagrams (simulate packet loss)
- Test out-of-order delivery
- Test duplicate datagrams
- Test message queue overflow

---

## Conclusion

The SONU Concurrent UDP Server implements a stateless, message-queue-based concurrent architecture that provides:
- **Simplicity:** No session management to maintain
- **Low Latency:** No connection overhead
- **Isolation:** Each datagram in its own process
- **Flexibility:** Stateless design allows easy scaling

The master-slave model with POSIX message queues ensures the server can handle multiple concurrent datagrams without blocking, while the stateless design simplifies the code at the cost of requiring re-authentication for each command.

**Trade-offs:**
- **Pros:** Simple, low latency, no connection overhead
- **Cons:** High per-command overhead, stateless, no multi-command sessions

**Best suited for:** Simple request-response protocols where each interaction is independent and low latency is more important than throughput.
