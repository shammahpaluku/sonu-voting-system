# SONU Electronic Voting System
SCS3304 Assignment 2 — Client-Server Version

## Project Structure
  server/   — TCP server, all business logic and data files
  client/   — TCP client, terminal UI only

## How to Build and Run

  Step 1: Build the server
    cd server
    make

  Step 2: Build the client (in a new terminal)
    cd client
    make

  Step 3: Run the server first
    cd server
    make run

  Step 4: Run the client (in a separate terminal)
    cd client
    make run

## Notes
  - Server must be running before client is started
  - Server binds to 127.0.0.1:9100
  - All data files are stored in server/data/
  - Client has no access to data files directly
