# SONU Voting System TCP - Design Documentation

**Version**: TCP (Connection-Oriented)  
**Protocol**: TCP (SOCK_STREAM)  
**Architecture**: Iterative Connection-Oriented Server

---

## Architectural Design

### Overall Architecture

Client-server architecture with iterative connection-oriented server model. Server handles all business logic and data persistence. Clients provide terminal interface and communicate over TCP.

```
Client (Terminal UI) → TCP Network (Port 9100) → Server (Business Logic + Data Storage)
```

### Communication Model

- Server creates listening socket on port 9100
- Client establishes persistent TCP connection
- Server accepts connection, creates dedicated socket
- Multiple commands sent over single connection
- Connection closed on client quit or logout

---

## Module Outline

### Server Modules

1. **Network Handler** (`net_handler.c`): TCP socket operations
   - `nh_server_init()` - Create/bind socket
   - `nh_server_accept()` - Accept connections
   - `nh_send_line()` - Send to client
   - `nh_recv_line()` - Receive from client
   - `nh_close()` - Close connection

2. **Command Dispatcher** (`server_main.c`): Parse and route commands
   - 18 command handlers (LOGIN, ADMIN_LOGIN, STATUS, etc.)
   - Session management per connection

3. **Business Logic Modules**:
   - `auth.c` - Voter/admin authentication
   - `position.c` - Position management
   - `candidate.c` - Candidate registration
   - `voter.c` - Voter registration
   - `voting.c` - Vote casting
   - `tally.c` - Vote counting
   - `admin.c` - Admin operations
   - `application.c` - Candidacy applications

4. **File Handler** (`file_handler.c`): Text file I/O
   - Pipe-delimited text files
   - voters.txt, candidates.txt, positions.txt, votes.txt, election_status.txt, applications.txt

### Client Modules

1. **Network Handler** (`net_handler.c`): TCP client socket operations
2. **Client Main** (`client_main.c`): User interface (voter menu, admin menu, voting workflow)

---

## Process Design

### Server Flow
```
Initialize socket → Bind to 0.0.0.0:9100 → Load data files
→ Accept loop: accept() → handle_session()
→ Session loop: recv_line() → dispatch_command() → send_line()
```

### Client Flow
```
Connect to server → Main menu (Voter/Admin Login)
→ Authenticated menu (voter: vote, apply; admin: manage positions, voting, results)
→ Send commands, receive responses
```

---

## Algorithm Design

### Key Algorithms

1. **Voter Registration**: Generate unique ID, validate password (min 10 chars), store in file
2. **Admin Authentication**: Hardcoded credentials (admin/admin123)
3. **Vote Casting**: Validate voter hasn't voted, record vote per position, prevent duplicates
4. **Vote Tallying**: Count votes per candidate, calculate percentages, determine winners
5. **Application Workflow**: Voters apply for candidacy, admins approve/reject

### Complexity
- Registration/Login: O(n) where n is number of voters
- Vote Casting: O(m) where m is number of positions
- Results: O(v) where v is number of votes

---

## Data/File Design

### Data Structures

```c
typedef struct { int id; char name[MAX_NAME_LEN]; } Position;
typedef struct { int id; char name[MAX_NAME_LEN]; int position_id; } Candidate;
typedef struct { int id; char name[MAX_NAME_LEN]; char password[MAX_PASS_LEN]; int has_voted; } Voter;
typedef struct { int candidate_id; int position_id; int vote_count; float percentage; } Result;
```

### File Format (Pipe-Delimited Text)

- **voters.txt**: `id|name|password|has_voted`
- **candidates.txt**: `id|name|position_id`
- **positions.txt**: `id|name`
- **votes.txt**: `voter_id|position_id|candidate_id`
- **election_status.txt**: `OPEN` or `CLOSED`
- **applications.txt**: `id|voter_name|position_id|position_name|status`

---

## Concurrency Design

**Current**: No concurrency (iterative server)

**If Required**: I/O multiplexing with select() recommended for TCP:
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

**Error Handling**: Return codes (SUCCESS, ERR_FILE, ERR_NOT_FOUND, ERR_DUPLICATE, ERR_AUTH_FAIL, ERR_VOTED, ERR_CLOSED, ERR_FULL, ERR_CONN, ERR_UNKNOWN)

---

## Testing Plan

### Test Cases

| Category | Tests |
|----------|-------|
| Authentication | Voter registration, login, admin login |
| Position Management | Add position, list positions |
| Candidate Management | Register candidate, list candidates, applications |
| Voting | Cast vote, prevent duplicate voting |
| Admin Operations | Open/close voting, view results, reset system |

### Test Results

✅ All 18 commands tested and verified
✅ Vote integrity maintained
✅ Application workflow functional
✅ Results calculation accurate

---

## Socket Operation Details

### Server Socket Operations

#### 1. Socket Creation
**Location**: `server/src/net_handler.c:11`
```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
```
- Creates TCP socket (`SOCK_STREAM`)
- Returns socket file descriptor
- On failure, returns `ERR_CONN`

#### 2. Binding
**Location**: `server/src/net_handler.c:32`
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

#### 3. Passive (Listening)
**Location**: `server/src/net_handler.c:39`
```c
listen(server_fd, BACKLOG);  // BACKLOG = 5
```
- Marks socket as passive (listening)
- Sets backlog queue to 5 pending connections
- Socket ready to accept connections

#### 4. Accepting
**Location**: `server/src/net_handler.c:53` (in `nh_server_accept`)
```c
int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
```
- Blocks until client connects
- Creates new socket for client connection
- Returns client socket file descriptor
- Captures client address

#### 5. Looping Back (Main Accept Loop)
**Location**: `server/src/server_main.c:main`
```c
while (1) {
    int client_fd = nh_server_accept(server_fd);
    if (client_fd == ERR_CONN) {
        continue;  // Loop back to accept next connection
    }
    handle_session(client_fd);
}
```
- Infinite loop accepting connections
- Iterative server (handles one client at a time)
- After session ends, loops back to accept next

#### 6. Reading (Receiving)
**Location**: `server/src/net_handler.c:99` (in `nh_recv_line`)
```c
int result = recv(fd, &ch, 1, 0);  // Read character by character
```
- Reads data from client socket
- Reads character by character until newline
- Blocks until data arrives

**Location**: `server/src/server_main.c:handle_session`
```c
int result = nh_recv_line(client_fd, cmd_buf, CMD_BUF_LEN);
```
- Called in session loop to receive commands

#### 7. Processing
**Location**: `server/src/server_main.c:dispatch_command`
```c
char *verb = strtok(cmd_copy, " ");  // Parse command
char *args = cmd_buf + strlen(verb) + 1;  // Extract arguments

if (strcmp(verb, "LOGIN") == 0) {
    cmd_login(client_fd, args);
} else if (strcmp(verb, "ADMIN_LOGIN") == 0) {
    cmd_admin_login(client_fd, args);
}
// ... other commands
```
- Parses command verb and arguments
- Routes to appropriate handler function
- Business logic executed in handler

#### 8. Forming Response
**Location**: `server/src/server_main.c` (in command handlers)
```c
// Example from cmd_login
nh_send_line(client_fd, "OK");
// Or error
nh_send_line(client_fd, "ERR_AUTH_FAIL");
```
- Response string constructed in handler
- Can be single line or multi-line (with "END" marker)

#### 9. Sending
**Location**: `server/src/net_handler.c:86` (in `nh_send_line`)
```c
char buf[CMD_BUF_LEN];
snprintf(buf, CMD_BUF_LEN, "%s\n", msg);
int bytes_sent = send(fd, buf, strlen(buf), 0);
```
- Sends response to client socket
- Appends newline to message
- Returns bytes sent or error

#### 10. Closing
**Location**: `server/src/net_handler.c:122` (in `nh_close`)
```c
close(fd);
```
- Closes socket connection
- Releases file descriptor

**Location**: `server/src/server_main.c:handle_session`
```c
nh_close(client_fd);  // Close after session ends
```

### Client Socket Operations

#### 1. Socket Creation
**Location**: `client/src/net_handler.c:63`
```c
int client_fd = socket(AF_INET, SOCK_STREAM, 0);
```

#### 2. Connecting (Active Open)
**Location**: `client/src/net_handler.c:73`
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = inet_addr(ip);  // "127.0.0.1"
server_addr.sin_port = htons(port);  // 9100

connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```
- Actively connects to server
- Establishes TCP three-way handshake
- Socket becomes connected

#### 3. Sending
**Location**: `client/src/net_handler.c:86` (in `nh_send_line`)
```c
int bytes_sent = send(fd, buf, strlen(buf), 0);
```
- Sends command to server

#### 4. Reading (Receiving)
**Location**: `client/src/net_handler.c:99` (in `nh_recv_line`)
```c
int result = recv(fd, &ch, 1, 0);
```
- Receives response from server
- Reads character by character

#### 5. Looping Back (Client Menu Loop)
**Location**: `client/src/client_main.c` (in menu functions)
```c
while (1) {
    display_menu();
    get_user_choice();
    send_command();
    receive_response();
    // Loop back for next command
}
```
- Interactive menu loop
- Continues until logout/quit

#### 6. Closing
**Location**: `client/src/net_handler.c:122`
```c
close(fd);
```

---

## Conclusion

TCP version implements full voting system with iterative connection-oriented server. All business logic on server, client provides terminal UI. Text file storage with pipe-delimited format. Ready for network deployment.

**Author**: shammahpaluku  
**Date**: April 21, 2026
