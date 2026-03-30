# SONU Voting System - Professional Edition

A comprehensive voting system available in both standalone and client-server architectures, with both GUI and enhanced terminal interfaces.

## 🎯 Available Versions

### 📱 Standalone Version (v2.0)
Single executable with all functionality in one process. Features dual interface modes (GUI + Terminal) with professional styling.

### 🌐 Client-Server Version (v3.0)
Distributed architecture with TCP server and separate client processes.

---

## 🚀 Standalone Version Features (v2.0)

### **Dual Interface Modes**
- **🖥️ GUI Mode**: Modern GTK3 graphical interface with professional styling
- **💻 Terminal Mode**: Enhanced terminal interface with rich colors and professional layout
- **🔄 Seamless Switching**: Choose your preferred interface at startup

### **Core Functionality**
- **👤 Voter Management**: Registration, authentication, and voting
- **🔑 Admin Controls**: Election management, position/candidate management
- **📊 Real-time Results**: Live vote counting and statistics
- **🔒 Security**: Secure authentication and vote validation
- **📈 Analytics**: Participation rates and election statistics

## 🌐 Client-Server Version Features (v3.0)

### Architecture
- **TCP Server**: Handles all business logic and data storage
- **TCP Client**: Terminal UI only, communicates with server
- **Network Protocol**: Custom TCP protocol for client-server communication
- **Data Storage**: Text files stored on server side only

### Key Differences from Standalone
- **Distributed**: Server and client run as separate processes
- **Network Access**: Clients can connect from different machines
- **Centralized Data**: All data managed by server
- **Scalability**: Multiple clients can connect (iterative server)

---

## 🏗️ Architecture Comparison

### Standalone (Dual-Mode Architecture)
```
┌─────────────────────────────────────┐
│         INTERFACE LAYER             │
│   (final_gui.c + main.c)           │
│  • GUI Mode (GTK3)                 │
│  • Terminal Mode (Enhanced)        │
│  • Console Mode (Simple)           │
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│      APPLICATION LOGIC LAYER        │
│  (admin.c, auth.c, candidate.c,    │
│   position.c, tally.c, voter.c,    │
│   voting.c, utils.c)               │
│  • Business logic & algorithms     │
│  • Data validation & processing    │
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│        FILE STORAGE LAYER           │
│      (file_handler.c)              │
│  • File I/O operations only        │
│  • Data persistence & loading       │
└─────────────────────────────────────┘
```

### Client-Server (Network Architecture)
```
Client Process                    Server Process
┌─────────────────┐         ┌─────────────────────────┐
│  Terminal UI    │         │   Business Logic       │
│  Network Client │◄─TCP────►│   File Storage         │
│  (client/)      │         │   (server/)            │
└─────────────────┘         └─────────────────────────┘
```

---

## 📁 Project Structure

### Standalone Version (v2.0)
```
sonu-voting-system/
├── README-Final-GUI.md    # Detailed documentation
├── Makefile              # Build configuration
├── bin/                  # Compiled executables
│   ├── sonu-voting-system   # Dual-mode (GUI + Terminal)
│   └── sonu-console         # Console-only version
├── src/                  # Source code
│   ├── final_gui.c         # Main dual-mode interface
│   ├── main.c              # Console interface
│   ├── admin.c             # Admin functions
│   ├── auth.c              # Authentication
│   ├── candidate.c         # Candidate management
│   ├── file_handler.c      # File operations
│   ├── position.c          # Position management
│   ├── tally.c             # Vote counting
│   ├── utils.c             # Utilities
│   ├── voter.c             # Voter management
│   └── voting.c            # Voting logic
├── include/               # Header files
├── data/                  # Data files (created automatically)
└── .cursor/               # Debug logs
```

### Client-Server Version (v3.0)
```
sonu-voting-system/
├── server/           # TCP server
│   ├── include/     # Server headers
│   ├── src/         # Server source
│   ├── data/        # Text data files
│   └── Makefile     # Server build
├── client/           # TCP client
│   ├── include/     # Client headers
│   ├── src/         # Client source
│   └── Makefile     # Client build
└── README.md        # This file
```

---

## 🛠️ Installation & Setup

### Standalone Version (v2.0)
```bash
# Prerequisites
sudo apt-get install libgtk-3-dev pkg-config

# Build both versions
make

# Run with interface selection (default)
make run

# Run GUI mode directly
make run-gui

# Run enhanced terminal mode directly  
make run-terminal

# Run simple console version
make run-console
```

### Client-Server Version (v3.0)
```bash
# Step 1: Build the server
cd server
make

# Step 2: Build the client (in a new terminal)
cd client
make

# Step 3: Run the server first
cd server
make run

# Step 4: Run the client (in a separate terminal)
cd client
make run
```

---

## 🎨 Interface Features

### Enhanced Terminal Mode (Standalone v2.0)
- **🌈 Rich Color Scheme**: Professional ANSI color coding
- **📐 Professional Layout**: Wide 120-character borders, consistent spacing
- **🎯 Visual Cues**: Emoji icons and descriptive menu items
- **📊 Color-coded Status**: Green for success, red for errors, blue for info
- **⚡ Fast Performance**: Lightweight and responsive

### GUI Mode (Standalone v2.0)
- **🖼️ Modern Design**: Professional GTK3 interface
- **📱 Responsive Layout**: Scalable voting areas
- **🎨 Visual Feedback**: Color-coded buttons and status indicators
- **📊 Tabbed Interface**: Organized position-based voting
- **🔍 Easy Navigation**: Intuitive mouse-based interaction

### Client Interface (Client-Server v3.0)
- **💻 Terminal Only**: Clean terminal interface
- **🌐 Network Communication**: TCP-based client-server protocol
- **🔒 Secure Authentication**: Server-side validation
- **📊 Real-time Updates**: Live data from server

---

## 🔧 Administration

### Default Credentials
- **Admin Password**: `admin123`
- **Voter Registration**: 10+ character passwords required

### Admin Functions
- Open/close voting periods
- Add/remove positions and candidates
- View real-time results and statistics
- Manage voter registration

### Voter Functions
- Self-registration with secure passwords
- Secure login and voting
- View election results
- One-vote-per-voter enforcement

---

## 📊 Data Management

### Standalone Version (Text Files)
```
data/
├── voters.dat     # Voter registrations
├── candidates.dat  # Candidate information  
├── positions.dat  # Voting positions
├── votes.dat      # Cast votes
└── status.txt     # Election status (open/closed)
```

### Client-Server Version (Server Text Files)
```
server/data/
├── voters.txt     # Voter registrations
├── candidates.txt  # Candidate information  
├── positions.txt  # Voting positions
├── votes.txt      # Cast votes
└── election_status.txt # Election status
```

### Security Features
- Password validation (minimum 10 characters)
- Unique voter ID assignment
- Vote duplication prevention
- Secure admin authentication

---

## 🛠️ Development

### Standalone Build Targets
```bash
make all          # Build both versions
make build        # Build main version only
make clean        # Remove compiled files
make install-deps # Install dependencies
```

### Standalone Run Targets
```bash
make run          # Interface selection menu
make run-gui      # GUI mode
make run-terminal # Enhanced terminal mode
make run-console  # Simple console mode
```

---

## 📈 System Requirements

- **Operating System**: Linux (Ubuntu/Debian recommended)
- **Dependencies**: GTK3 development libraries (for GUI mode)
- **Memory**: Minimal footprint (< 50MB runtime)
- **Storage**: < 5MB disk space

---

## 🎯 Usage Examples

### Scenario 1: Election Setup (Standalone)
1. Run `make run`
2. Choose GUI mode (option 1)
3. Admin login with `admin123`
4. Add positions and candidates
5. Open voting for participants

### Scenario 2: Terminal Voting (Standalone)
1. Run `make run-terminal`
2. Voter registers or logs in
3. Selects candidates for each position
4. Confirms and casts vote

### Scenario 3: Client-Server Voting
1. Start server: `cd server && make run`
2. Start client: `cd client && make run`
3. Voter registers or logs in
4. Vote through network interface

### Scenario 4: Results Monitoring
1. Admin login
2. View real-time statistics
3. Monitor participation rates
4. Export results if needed

---

## 🔒 Security Considerations

- All passwords are stored with basic encryption
- Voter IDs are unique and non-transferable
- Vote integrity is maintained through file validation
- Admin operations require authentication
- No vote modification after casting
- Server-side validation in client-server version

---

## � Future Enhancements

### Planned Features
- [ ] Real-time network voting (✅ Client-server version)
- [ ] Web-based voting interface
- [ ] Mobile voting application
- [ ] Advanced encryption and security
- [ ] Multi-language support
- [ ] Advanced analytics and reporting
- [ ] Voter verification systems
- [ ] Blockchain-based voting

### Technical Improvements
- [ ] Concurrent server handling
- [ ] Database integration (SQLite/PostgreSQL)
- [ ] RESTful API for web interface
- [ ] Load balancing for large elections
- [ ] Audit trail system
- [ ] Automated testing framework

---

## 📊 Project Statistics

- **Languages**: C (ANSI C99), GTK3
- **Files**: 20+ source files per version
- **Lines of Code**: 4,000+ (standalone), 4,500+ (client-server)
- **Architecture**: Modular design + Network architecture
- **Standards**: ANSI C99 compliance

---

## 🤝 Contributing

### Development Setup
1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly
4. Commit changes: `git commit -m "Add feature description"`
5. Push to branch: `git push origin feature-name`
6. Submit pull request

### Coding Standards
- Follow ANSI C99 standards
- Use consistent naming conventions
- Add comprehensive comments
- Maintain modular architecture
- Test all functionality

---

## 📝 License

This project is developed for educational purposes as part of the SCS3304 course requirements.

---

## 🙏 Acknowledgments

- **SCS3304 Course** - Systems Programming concepts
- **GTK3 Library** - Graphical interface framework
- **C Standard Library** - String handling, time functions
- **ANSI/ISO Standards** - C99 compliance guidelines

---

**Author**: shammahpaluku  
**Email**: skyssando@gmail.com  
**Course**: SCS3304 - Systems Programming  
**Versions**: 
- v2.0 - Standalone Dual-Mode (GUI + Terminal)
- v3.0 - Client-Server Network Implementation

---

**SONU Voting System** - Professional Election Management Solution
