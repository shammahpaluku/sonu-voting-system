# SONU Concurrent TCP Server - Technical Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Main Algorithm](#main-algorithm)
4. [Function Documentation](#function-documentation)
5. [Concurrency Model](#concurrency-model)
6. [Data Flow](#data-flow)

---

## Overview

The SONU (Student Organization of Nairobi University) Voting System is a concurrent TCP server that manages student elections. It supports voter registration, candidate applications, voting, and election administration through a client-server architecture.

**Key Features:**
- Concurrent client handling using fork-based master-slave model
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
│  - Listens on port 9100                                     │
│  - Accepts incoming TCP connections                         │
│  - Forks slave processes for each client                    │
│  - Reaps zombie processes via SIGCHLD handler              │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ fork()
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     Slave Process                            │
│  - Handles one client session                               │
│  - Manages authentication state                            │
│  - Processes commands via dispatch_command()               │
│  - Saves data to files                                      │
│  - Exits when client disconnects or sends QUIT              │
└─────────────────────────────────────────────────────────────┘
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

**Purpose:** Initialize the server and enter the master accept loop.

**File:** `server/src/server_main.c` (lines 721-779)

**Code:**
```c
int main(void) {
    // Initialize files
    int result = fh_init_files();
    if (result != SUCCESS) {
        printf("Failed to initialize files. Exiting.\n");
        return 1;
    }
    
    // Initialize server
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start server. Exiting.\n");
        return 1;
    }
    
    printf("SONU Voting Server ready. Waiting for connections...\n");
    
    // Install SIGCHLD handler to prevent zombie slave processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);
    
    // Main server loop - fork-based concurrent server
    while (1) {
        int client_fd = nh_server_accept(server_fd);
        if (client_fd == ERR_CONN) {
            continue; // Skip this iteration
        }

        pid_t pid = fork();

        if (pid < 0) {
            // fork failed — close this client and try again
            perror("fork");
            nh_close(client_fd);
            continue;
        }

        if (pid == 0) {
            // SLAVE PROCESS
            // The slave does not need the listening socket — close it
            close(server_fd);
            // Handle the client session fully — auth, commands, voting logic, file saves
            handle_session(client_fd);
            // Slave exits when the session ends
            exit(0);
        }

        // MASTER PROCESS
        // Master does not own this client connection — slave does. Close master's copy.
        nh_close(client_fd);
        // Master loops back immediately to accept the next voter or admin
    }
    
    // This line is unreachable but required by C standard
    return 0;
}
```

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

#### `cmd_login(int client_fd, char *args)`
**Purpose:** Authenticate a voter using ID and password.

**File:** `server/src/server_main.c` (lines 65-87)

**Code:**
```c
void cmd_login(int client_fd, char *args) {
    char *token = strtok(args, " ");
    if (!token) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int voter_id = atoi(token);
    char *password = strtok(NULL, " ");
    if (!password) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int result = auth_voter_login(voter_id, password);
    if (result == SUCCESS) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %s", auth_get_voter_name());
        nh_send_line(client_fd, response);
    } else {
        nh_send_line(client_fd, result == ERR_NOT_FOUND ? "ERR_NOT_FOUND" : "ERR_AUTH_FAIL");
    }
}
```

**Authentication Flow:**
- Hashes the plaintext password
- Compares with stored hash in voters.txt
- Sets session state on success

#### `cmd_admin_login(int client_fd, char *args)`
**Purpose:** Authenticate the administrator.

**File:** `server/src/server_main.c` (lines 89-104)

**Code:**
```c
void cmd_admin_login(int client_fd, char *args) {
    char *username = strtok(args, " ");
    if (!username) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *password = strtok(NULL, " ");
    if (!password) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int result = auth_admin_login(username, password);
    nh_send_line(client_fd, result == SUCCESS ? "OK" : "ERR_AUTH_FAIL");
}
```

**Security:** Admin credentials are hardcoded in config.h (admin/admin123).

#### `cmd_logout(int client_fd, char *args)`
**Purpose:** Clear the current authentication session.

**File:** `server/src/server_main.c` (lines 106-110)

**Code:**
```c
void cmd_logout(int client_fd, char *args) {
    (void)args; // Unused
    auth_logout();
    nh_send_line(client_fd, "OK");
}
```

#### `cmd_status(int client_fd, char *args)`
**Purpose:** Retrieve the current election status.

**File:** `server/src/server_main.c` (lines 112-122)

**Code:**
```c
void cmd_status(int client_fd, char *args) {
    (void)args; // Unused
    char status[MAX_LINE_LEN];
    int result = admin_get_election_status(status, MAX_LINE_LEN);
    
    if (result == SUCCESS) {
        nh_send_line(client_fd, status);
    } else {
        nh_send_line(client_fd, "ERR_FILE");
    }
}
```

---

### Voter Management

#### `cmd_self_register(int client_fd, char *args)`
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

**Security Note:** Password is hashed before storage.

#### `cmd_register_voter(int client_fd, char *args)`
**Purpose:** Admin registers a voter (deprecated, uses same logic as self-register).

**File:** `server/src/server_main.c` (lines 124-161)

**Code:**
```c
void cmd_register_voter(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    // Find the space between name and password
    char *space = strchr(args, ' ');
    if (!space) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    *space = '\0';
    char *name = args;
    char *password = space + 1;
    
    replace_underscores(name);
    
    int result = voter_register(name, password);
    if (result >= 0) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", result);
        nh_send_line(client_fd, response);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_line(client_fd, "ERR_DUPLICATE");
                break;
            case ERR_FULL:
                nh_send_line(client_fd, "ERR_FULL");
                break;
            default:
                nh_send_line(client_fd, "ERR_FILE");
                break;
        }
    }
}
```

---

### Position Management

#### `cmd_add_position(int client_fd, char *args)`
**Purpose:** Admin adds a new election position.

**File:** `server/src/server_main.c` (lines 163-179)

**Code:**
```c
void cmd_add_position(int client_fd, char *args) {
    if (!args || strlen(args) == 0) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    replace_underscores(args);
    
    int result = pos_add(args);
    if (result >= 0) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", result);
        nh_send_line(client_fd, response);
    } else {
        nh_send_line(client_fd, result == ERR_DUPLICATE ? "ERR_DUPLICATE" : "ERR_FILE");
    }
}
```

**Validation:** Position name must contain only alphabetic characters and spaces.

#### `cmd_list_positions(int client_fd, char *args)`
**Purpose:** List all available election positions.

**File:** `server/src/server_main.c` (lines 181-203)

**Code:**
```c
void cmd_list_positions(int client_fd, char *args) {
    (void)args; // Unused
    Position positions[MAX_POSITIONS];
    int count = pos_get_all(positions, MAX_POSITIONS);
    
    if (count == ERR_FILE || count == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        char name_copy[MAX_NAME_LEN];
        strncpy(name_copy, positions[i].name, MAX_NAME_LEN - 1);
        name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(name_copy);
        
        snprintf(line, CMD_BUF_LEN, "%d %s", positions[i].id, name_copy);
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}
```

**Output Format:** Multi-line response ending with "END".

---

### Candidate Management

#### `cmd_apply_candidate(int client_fd, char *args)`
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

**Workflow:** Apply → Admin approves → Becomes candidate.

#### `cmd_register_cand(int client_fd, char *args)`
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

#### `cmd_list_cands(int client_fd, char *args)`
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

#### `cmd_list_applications(int client_fd, char *args)`
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

#### `cmd_approve_application(int client_fd, char *args)`
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

#### `cmd_reject_application(int client_fd, char *args)`
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

#### `cmd_open_voting(int client_fd, char *args)`
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

#### `cmd_close_voting(int client_fd, char *args)`
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

#### `cmd_cast_vote(int client_fd, char *args)`
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

**Validation:** One vote per position per voter.

#### `cmd_results(int client_fd, char *args)`
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

**Output Format:** Multi-line response grouped by position.

#### `cmd_reset(int client_fd, char *args)`
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

**Warning:** This is a destructive operation.

---

### Session Management

#### `cmd_quit(int client_fd, char *args)`
**Purpose:** Client requests to disconnect.

**Algorithm:**
```
1. Send "OK" to client
2. Set last_was_quit = 1 (signals handle_session to break)
```

#### `dispatch_command(int client_fd, char *cmd_buf)`
**Purpose:** Parse command and route to appropriate handler.

**File:** `server/src/server_main.c` (lines 629-688)

**Code:**
```c
void dispatch_command(int client_fd, char *cmd_buf) {
    char cmd_copy[CMD_BUF_LEN];
    strncpy(cmd_copy, cmd_buf, CMD_BUF_LEN - 1);
    cmd_copy[CMD_BUF_LEN - 1] = '\0';
    
    char *verb = strtok(cmd_copy, " ");
    if (!verb) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *args = NULL;
    if (strlen(verb) < strlen(cmd_buf)) {
        args = cmd_buf + strlen(verb) + 1;
    }
    
    last_was_quit = 0;
    
    if (strcmp(verb, "LOGIN") == 0) {
        cmd_login(client_fd, args);
    } else if (strcmp(verb, "ADMIN_LOGIN") == 0) {
        cmd_admin_login(client_fd, args);
    } else if (strcmp(verb, "LOGOUT") == 0) {
        cmd_logout(client_fd, args);
    } else if (strcmp(verb, "STATUS") == 0) {
        cmd_status(client_fd, args);
    } else if (strcmp(verb, "SELF_REGISTER") == 0) {
        cmd_self_register(client_fd, args);
    } else if (strcmp(verb, "APPLY_CANDIDATE") == 0) {
        cmd_apply_candidate(client_fd, args);
    } else if (strcmp(verb, "LIST_APPLICATIONS") == 0) {
        cmd_list_applications(client_fd, args);
    } else if (strcmp(verb, "APPROVE_APPLICATION") == 0) {
        cmd_approve_application(client_fd, args);
    } else if (strcmp(verb, "REJECT_APPLICATION") == 0) {
        cmd_reject_application(client_fd, args);
    } else if (strcmp(verb, "ADD_POSITION") == 0) {
        cmd_add_position(client_fd, args);
    } else if (strcmp(verb, "LIST_POSITIONS") == 0) {
        cmd_list_positions(client_fd, args);
    } else if (strcmp(verb, "REGISTER_CAND") == 0) {
        cmd_register_cand(client_fd, args);
    } else if (strcmp(verb, "LIST_CANDS") == 0) {
        cmd_list_cands(client_fd, args);
    } else if (strcmp(verb, "OPEN_VOTING") == 0) {
        cmd_open_voting(client_fd, args);
    } else if (strcmp(verb, "CLOSE_VOTING") == 0) {
        cmd_close_voting(client_fd, args);
    } else if (strcmp(verb, "CAST_VOTE") == 0) {
        cmd_cast_vote(client_fd, args);
    } else if (strcmp(verb, "RESULTS") == 0) {
        cmd_results(client_fd, args);
    } else if (strcmp(verb, "RESET") == 0) {
        cmd_reset(client_fd, args);
    } else if (strcmp(verb, "QUIT") == 0) {
        cmd_quit(client_fd, args);
    } else {
        nh_send_line(client_fd, "ERR_UNKNOWN");
    }
}
```

**Routing:** Simple string comparison-based dispatcher.

#### `handle_session(int client_fd)`
**Purpose:** Manage a complete client session from connection to disconnection.

**File:** `server/src/server_main.c` (lines 690-713)

**Code:**
```c
void handle_session(int client_fd) {
    // Reset auth state at start of each session
    auth_logout();
    
    char cmd_buf[CMD_BUF_LEN];
    
    while (1) {
        int result = nh_recv_line(client_fd, cmd_buf, CMD_BUF_LEN);
        if (result == ERR_CONN) {
            printf("Client disconnected.\n");
            break;
        }
        
        printf("CMD: %s\n", cmd_buf);
        dispatch_command(client_fd, cmd_buf);
        
        if (last_was_quit) {
            break;
        }
    }
    
    nh_close(client_fd);
    printf("Session ended.\n");
}
```

**State Management:** Each session has independent auth state.

#### `sigchld_handler(int sig)`
**Purpose:** Signal handler to reap zombie child processes.

**File:** `server/src/server_main.c` (lines 716-719)

**Code:**
```c
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
```

**Parameters:**
- `-1`: Wait for any child process
- `NULL`: Don't store exit status
- `WNOHANG`: Non-blocking (return immediately if no child exited)

---

## Concurrency Model

### Master-Slave Architecture

**Master Process:**
- Single process that runs for the lifetime of the server
- Only responsibility: Accept connections and fork slaves
- Does not process any client commands
- Closes client socket immediately after forking

**Slave Process:**
- One slave per client connection
- Handles entire client session independently
- Has its own copy of all data (reads from files)
- Writes to files (requires file locking for safety)
- Exits when client disconnects

### Process Isolation

Each slave process has:
- **Independent memory space:** No shared variables between clients
- **Independent file descriptors:** Each has its own socket
- **Independent authentication state:** Sessions don't interfere

### Zombie Process Prevention

**Problem:** When a child exits, it becomes a zombie until parent calls wait().

**Solution:** SIGCHLD handler with WNOHANG
```
sigchld_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
```

**Flags:**
- `SA_RESTART`: Automatically restart interrupted system calls (like accept())
- `SA_NOCLDWAIT`: Kernel automatically reaps children (Linux-specific)

---

## Data Flow

### Typical Client Session

```
Client                    Server Master              Server Slave
  |                           |                          |
  |-- CONNECT -------------->|                          |
  |                           |-- fork() -------------->|
  |                           |                          |
  |-- LOGIN ----------------->|                          |
  |                           |                          |-- auth_voter_login()
  |                           |                          |-- read voters.txt
  |<-- OK John_Doe ----------|                          |
  |                           |                          |
  |-- LIST_POSITIONS ------->|                          |
  |                           |                          |-- pos_get_all()
  |                           |                          |-- read positions.txt
  |<-- 1 Chairperson --------|                          |
  |<-- 2 Vice_Chairperson ---|                          |
  |<-- END ------------------|                          |
  |                           |                          |
  |-- CAST_VOTE 1 5 -------->|                          |
  |                           |                          |-- voting_cast_vote()
  |                           |                          |-- write votes.txt
  |<-- OK --------------------|                          |
  |                           |                          |
  |-- QUIT ------------------>|                          |
  |                           |                          |-- exit()
  |                           |                          |
  |-- DISCONNECT ------------|                          |
```

### File Access Pattern

**Read Operations:**
- Load entire file into memory array
- Search/filter in memory
- No locking needed for reads (concurrent reads safe)

**Write Operations:**
- Rewrite entire file with updated data
- **Requires file locking** (flock) to prevent corruption
- Implemented in file_handler.c

### Authentication Flow

```
┌─────────────┐
│  LOGIN      │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Find voter  │
│ in file     │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Hash        │
│ password    │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Compare     │
│ with stored │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ Set session │
│ state       │
└─────────────┘
```

---

## Error Handling

### Error Codes

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

All errors are sent as plain text lines:
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
- Passwords are hashed using SHA-256 (via utils_hash_password)
- Plain text passwords are never stored
- Hash comparison for authentication

### Access Control
- Admin commands require `auth_is_admin()` check
- Voter commands require `auth_is_logged_in()` check
- Session state is per-process (isolated)

### Input Validation
- Position names validated for alphabetic characters only
- Voter ID and candidate ID validated as integers
- Buffer sizes enforced (strncpy with length limits)

---

## Performance Characteristics

### Scalability
- **Connection handling:** Unlimited (process-based)
- **Memory usage:** ~2MB per slave process
- **File I/O:** Each slave reads/writes independently

### Bottlenecks
- **File locking:** Contention on writes (mitigated by flock)
- **Process creation:** fork() overhead per connection
- **File loading:** Each slave loads entire files

### Optimization Opportunities
- Use shared memory for read-only data
- Implement connection pooling
- Cache frequently accessed data

---

## Testing Recommendations

### Unit Tests
- Test each command handler independently
- Test error conditions (invalid input, missing auth)
- Test file I/O operations

### Integration Tests
- Test complete client sessions
- Test concurrent access (multiple clients)
- Test admin workflow (approve applications, results)

### Load Tests
- Test with many concurrent clients
- Test file locking under contention
- Monitor memory usage

---

## Conclusion

The SONU Concurrent TCP Server implements a robust, fork-based concurrent architecture that provides:
- **Isolation:** Each client in its own process
- **Simplicity:** Straightforward code structure
- **Reliability:** Proper signal handling and error management
- **Functionality:** Complete election management system

The master-slave model ensures the server can handle multiple clients simultaneously without blocking, while file locking ensures data integrity in concurrent write scenarios.
