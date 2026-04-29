#ifndef CONFIG_H
#define CONFIG_H

// Server configuration
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 9100
#define CMD_BUF_LEN 1024
#define BACKLOG 5

// Size limits
#define MAX_NAME_LEN 64
#define MAX_PASS_LEN 128
#define MAX_POSITIONS 20
#define MAX_CANDIDATES 200

// Admin credentials (for reference)
#define ADMIN_USER "admin"
#define ADMIN_PASS "admin123"

// Return codes
#define SUCCESS 0
#define ERR_CONN -8
#define ERR_UNKNOWN -9

#endif // CONFIG_H
