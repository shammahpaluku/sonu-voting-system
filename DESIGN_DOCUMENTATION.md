# SONU Voting System UDP - Design Documentation

**Version**: UDP (Connectionless)  
**Protocol**: UDP (SOCK_DGRAM)  
**Architecture**: Iterative Connectionless Server

---

## Architectural Design

### Overall Architecture

Client-server architecture with iterative connectionless server model. Server handles business logic and data persistence. Clients communicate via UDP datagrams.

**Key Difference from TCP**: No persistent connection. Each command is independent datagram. Server tracks client addresses for responses. Auth state maintained across datagrams from same client address.

### Communication Model

- Server binds to 0.0.0.0:9100
- Client sends datagrams directly to server address
- Server receives datagram, extracts client address, sends response
- Multi-line responses sent as separate datagrams with "END" marker
- Session state maintained per client address (not per connection)

---

## Module Outline

### Server Modules

1. **Network Handler** (`net_handler.c`): UDP socket operations
   - `nh_server_init()` - Create/bind socket
   - `nh_send_to()` - Send to specific address
   - `nh_recv_from()` - Receive with sender address
   - `nh_close()` - Close socket

2. **Command Dispatcher** (`server_main.c`): Parse and route commands
   - All handlers receive `sock` and `client_addr` parameters
   - Auth state keyed by client address
   - **Important**: auth_logout() NOT called before each command (unlike group-chat UDP)

3. **Business Logic Modules**: Same as TCP version
   - `auth.c`, `position.c`, `candidate.c`, `voter.c`, `voting.c`, `tally.c`, `admin.c`, `application.c`

4. **File Handler** (`file_handler.c`): Text file I/O (same as TCP)

### Client Modules

1. **Network Handler** (`net_handler.c`): UDP socket operations
   - `nh_client_init()` - Initialize with server address
   - `nh_send_to()` - Send to server
   - `nh_recv_from()` - Receive from server

2. **Client Main** (`client_main.c`): User interface
   - Multi-line response handling (receives multiple datagrams)
   - Full interactive menus (voter menu, admin menu, voting workflow)

---

## Process Design

### Server Flow
```
Initialize socket → Bind to 0.0.0.0:9100 → Load data files
→ Main loop: recv_from() → dispatch_command() (NO auth_logout)
→ Handlers use nh_send_to() for responses
→ Auth state persists across datagrams from same client address
```

### Client Flow
```
Initialize socket → Set server address
→ Main menu loop → Send commands via nh_send_to()
→ Receive responses via nh_recv_from()
→ Handle multi-line responses (multiple datagrams)
```

---

## Algorithm Design

Same algorithms as TCP version:
- Voter registration/login
- Admin authentication
- Position/candidate management
- Vote casting with integrity checks
- Vote tallying with percentages
- Application workflow

**UDP-Specific**: Multi-line responses require receiving multiple datagrams until "END" marker.

---

## Data/File Design

Same data structures and text file format as TCP:
- Pipe-delimited text files
- voters.txt, candidates.txt, positions.txt, votes.txt, election_status.txt, applications.txt
- Sequential read/write operations

---

## Concurrency Design

**Current**: No concurrency (iterative server)

**If Required**: I/O multiplexing with select() recommended for UDP:
- Single process handles multiple clients
- Event-driven architecture
- No locks needed for single-threaded event loop

---

## Implementation

**Build**:
```bash
gcc -std=c11 -Wall -Wextra -g -Iinclude src/*.c -o bin/sonu_server
gcc -std=c11 -Wall -Wextra -g -Iinclude src/*.c -o bin/sonu_client
```

**Dependencies**: Standard C libraries only

**Error Handling**: Same return codes as TCP

---

## Testing Plan

### Test Cases (Same as TCP)
- Authentication (voter, admin)
- Position/candidate management
- Voting workflow
- Results calculation
- Application management

### UDP-Specific Tests
- Datagram delivery
- Multi-line response handling
- Client address tracking
- Session state persistence across datagrams

### Test Results
✅ All 18 commands tested and working
✅ Multi-line responses received correctly
✅ Client address tracking works
✅ Session state maintained per address (persists across datagrams)

### Connectionless Demonstration

**Voting Workflow (UDP vs TCP):**

**TCP (Connection-Oriented):**
- For each position: Send CAST_VOTE → Receive response
- Multiple round-trips for multiple votes
- Persistent connection maintained

**UDP (Connectionless):**
- Collect all voting choices locally
- Send single `CAST_ALL_VOTES pos1:cand1 pos2:cand2 ...` datagram
- Server processes all votes atomically
- No persistent connection, independent datagrams

This demonstrates UDP's connectionless nature: data is batched and sent in a single datagram rather than requiring a persistent connection with multiple round-trips.

---

## Socket Operation Details

### Server Socket Operations

#### 1. Socket Creation
**Location**: `server/src/net_handler.c:11`
```c
int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
```
- Creates UDP socket (`SOCK_DGRAM`)
- Returns socket file descriptor
- On failure, returns `ERR_CONN`
- **Difference from TCP**: No connection-oriented socket

#### 2. Binding
**Location**: `server/src/net_handler.c:30`
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // "0.0.0.0"
server_addr.sin_port = htons(port);  // 9100

bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```
- Binds socket to all interfaces (`0.0.0.0`)
- Binds to port 9100
- Associates socket with local address
- **Difference from TCP**: No listen() call needed

#### 3. Passive (Listening)
**Location**: `N/A` - UDP does not use listen()
- UDP is connectionless, no passive listening state
- Socket is ready to receive datagrams immediately after bind()
- **Difference from TCP**: No listen() or backlog queue

#### 4. Accepting
**Location**: `N/A` - UDP does not use accept()
- No connection establishment in UDP
- Each datagram received independently
- **Difference from TCP**: No accept() call, no separate client socket

#### 5. Looping Back (Main Datagram Loop)
**Location**: `server/src/server_main.c:main`
```c
while (1) {
    char cmd_buf[CMD_BUF_LEN];
    struct sockaddr_in client_addr;
    
    int result = nh_recv_from(server_fd, cmd_buf, CMD_BUF_LEN, &client_addr);
    if (result == ERR_CONN) {
        continue;  // Loop back to receive next datagram
    }
    
    printf("CMD [%s] from %s:%d\n", verb, 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    dispatch_command(server_fd, &client_addr, cmd_buf);
}
```
- Infinite loop receiving datagrams
- Each datagram processed independently
- Captures client address from each datagram
- **Difference from TCP**: No session loop, single datagram per iteration

#### 6. Reading (Receiving)
**Location**: `server/src/net_handler.c:58` (in `nh_recv_from`)
```c
int bytes_read = recvfrom(sock, buf, buf_len - 1, 0, 
                         (struct sockaddr*)client_addr, &addr_len);
```
- Receives datagram from any sender
- Captures sender's address in `client_addr`
- Receives entire datagram at once (not character by character)
- **Difference from TCP**: Uses recvfrom() instead of recv(), captures sender address

**Location**: `server/src/server_main.c:main`
```c
int result = nh_recv_from(server_fd, cmd_buf, CMD_BUF_LEN, &client_addr);
```
- Called in main loop to receive commands

#### 7. Processing
**Location**: `server/src/server_main.c:dispatch_command`
```c
char *verb = strtok(cmd_copy, " ");  // Parse command
char *args = cmd_buf + strlen(verb) + 1;  // Extract arguments

if (strcmp(verb, "LOGIN") == 0) {
    cmd_login(sock, client_addr, args);
} else if (strcmp(verb, "CAST_ALL_VOTES") == 0) {
    cmd_cast_all_votes(sock, client_addr, args);
}
// ... other commands
```
- Parses command verb and arguments
- Routes to appropriate handler function
- All handlers receive `sock` and `client_addr` parameters
- **Difference from TCP**: Handlers receive client address instead of client FD

#### 8. Forming Response
**Location**: `server/src/server_main.c` (in command handlers)
```c
// Example from cmd_login
nh_send_to(sock, "OK", client_addr);
// Or error
nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
```
- Response string constructed in handler
- Can be single line or multi-line (with "END" marker)
- **Difference from TCP**: Response sent to client address, not socket FD

#### 9. Sending
**Location**: `server/src/net_handler.c:45` (in `nh_send_to`)
```c
char buf[CMD_BUF_LEN];
snprintf(buf, sizeof(buf), "%s\n", msg);
socklen_t addr_len = sizeof(struct sockaddr_in);
int bytes_sent = sendto(sock, buf, strlen(buf), 0, 
                       (struct sockaddr*)client_addr, addr_len);
```
- Sends datagram to specific client address
- Requires destination address parameter
- **Difference from TCP**: Uses sendto() instead of send(), requires destination address

#### 10. Closing
**Location**: `server/src/net_handler.c:75` (in `nh_close`)
```c
close(sock);
```
- Closes socket
- **Difference from TCP**: Only one socket (no separate client sockets)

**Location**: `server/src/server_main.c:main`
```c
nh_close(server_fd);  // Close on server shutdown
```

### Client Socket Operations

#### 1. Socket Creation
**Location**: `client/src/net_handler.c:28`
```c
int sock = socket(AF_INET, SOCK_DGRAM, 0);
```
- Creates UDP socket (`SOCK_DGRAM`)
- **Difference from TCP**: No connection-oriented socket

#### 2. No Connecting (Connectionless)
**Location**: `client/src/net_handler.c:36` (in `nh_client_init`)
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = inet_addr(ip);  // "127.0.0.1"
server_addr.sin_port = htons(port);  // 9100

// No connect() call - store server address for sendto()
g_server_addr = server_addr;
```
- No connect() call in UDP
- Server address stored for use with sendto()
- **Difference from TCP**: No connect() or three-way handshake

#### 3. Sending
**Location**: `client/src/net_handler.c:48` (in `nh_send_to`)
```c
socklen_t addr_len = sizeof(struct sockaddr_in);
int bytes_sent = sendto(sock, buf, strlen(buf), 0, 
                       (struct sockaddr*)&g_server_addr, addr_len);
```
- Sends datagram to server address
- Requires destination address parameter
- **Difference from TCP**: Uses sendto() instead of send()

#### 4. Reading (Receiving)
**Location**: `client/src/net_handler.c:58` (in `nh_recv_from`)
```c
int bytes_read = recvfrom(sock, buf, buf_len - 1, 0, 
                         (struct sockaddr*)&from_addr, &addr_len);
```
- Receives datagram from server
- Captures sender address (though typically not used)
- Receives entire datagram at once
- **Difference from TCP**: Uses recvfrom() instead of recv()

#### 5. Looping Back (Client Menu Loop)
**Location**: `client/src/client_main.c` (in menu functions)
```c
while (1) {
    display_menu();
    get_user_choice();
    send_command();  // Each command is independent datagram
    receive_response();
    // Loop back for next command
}
```
- Interactive menu loop
- Each command sent as independent datagram
- **Difference from TCP**: No persistent connection maintained

#### 6. Closing
**Location**: `client/src/net_handler.c:70`
```c
close(sock);
```

---

## Conclusion

UDP version maintains full feature parity with TCP while using connectionless datagram communication. All business logic identical. Network layer adapted for UDP (sendto/recvfrom). Auth state persists across datagrams from same client address (unlike group-chat UDP). Ready for cross-machine deployment.

**Author**: shammahpaluku  
**Date**: April 21, 2026
