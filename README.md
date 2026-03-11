# SONU Voting System - Professional Edition

A comprehensive, dual-mode voting system with both GUI and enhanced terminal interfaces.

## 🎯 Features

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

## 🚀 Quick Start

### **Prerequisites**
```bash
sudo apt-get install libgtk-3-dev pkg-config
```

### **Build & Run**
```bash
# Build both versions
make

# Run with interface selection (default)
make run
# or
./bin/sonu-voting-system

# Run GUI mode directly
make run-gui

# Run enhanced terminal mode directly  
make run-terminal

# Run simple console version
make run-console
```

### **Command Line Options**
```bash
# Direct terminal mode
./bin/sonu-voting-system --terminal

# Direct GUI mode
./bin/sonu-voting-system
```

## 📁 Project Structure

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

## 🎨 Interface Features

### **Enhanced Terminal Mode**
- **🌈 Rich Color Scheme**: Professional ANSI color coding
- **📐 Professional Layout**: Wide 120-character borders, consistent spacing
- **🎯 Visual Cues**: Emoji icons and descriptive menu items
- **📊 Color-coded Status**: Green for success, red for errors, blue for info
- **⚡ Fast Performance**: Lightweight and responsive

### **GUI Mode**
- **🖼️ Modern Design**: Professional GTK3 interface
- **📱 Responsive Layout**: Scalable voting areas
- **🎨 Visual Feedback**: Color-coded buttons and status indicators
- **📊 Tabbed Interface**: Organized position-based voting
- **🔍 Easy Navigation**: Intuitive mouse-based interaction

## 🔧 Administration

### **Default Credentials**
- **Admin Password**: `admin123`
- **Voter Registration**: 10+ character passwords required

### **Admin Functions**
- Open/close voting periods
- Add/remove positions and candidates
- View real-time results and statistics
- Manage voter registration

### **Voter Functions**
- Self-registration with secure passwords
- Secure login and voting
- View election results
- One-vote-per-voter enforcement

## 📊 Data Management

### **File Structure**
```
data/
├── voters.dat     # Voter registrations
├── candidates.dat  # Candidate information  
├── positions.dat  # Voting positions
├── votes.dat      # Cast votes
└── status.txt     # Election status (open/closed)
```

### **Security Features**
- Password validation (minimum 10 characters)
- Unique voter ID assignment
- Vote duplication prevention
- Secure admin authentication

## 🛠️ Development

### **Build Targets**
```bash
make all          # Build both versions
make build        # Build main version only
make clean        # Remove compiled files
make install-deps # Install dependencies
```

### **Run Targets**
```bash
make run          # Interface selection menu
make run-gui      # GUI mode
make run-terminal # Enhanced terminal mode
make run-console  # Simple console mode
```

## 📈 System Requirements

- **Operating System**: Linux (Ubuntu/Debian recommended)
- **Dependencies**: GTK3 development libraries
- **Memory**: Minimal footprint (< 50MB runtime)
- **Storage**: < 5MB disk space

## 🎯 Usage Examples

### **Scenario 1: Election Setup**
1. Run `make run`
2. Choose GUI mode (option 1)
3. Admin login with `admin123`
4. Add positions and candidates
5. Open voting for participants

### **Scenario 2: Terminal Voting**
1. Run `make run-terminal`
2. Voter registers or logs in
3. Selects candidates for each position
4. Confirms and casts vote

### **Scenario 3: Results Monitoring**
1. Admin login
2. View real-time statistics
3. Monitor participation rates
4. Export results if needed

## 🔒 Security Considerations

- All passwords are stored with basic encryption
- Voter IDs are unique and non-transferable
- Vote integrity is maintained through file validation
- Admin operations require authentication
- No vote modification after casting

## 📝 License

This project is developed for educational purposes as part of the SCS3304 course requirements.

## 🤝 Contributing

This is a standalone project. For issues or enhancements, please refer to the project documentation.

---

**SONU Voting System v2.0** - Professional Election Management Solution
