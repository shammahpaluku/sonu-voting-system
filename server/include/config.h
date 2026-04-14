#ifndef CONFIG_H
#define CONFIG_H

// File path constants
#define VOTERS_FILE "data/voters.txt"
#define CANDIDATES_FILE "data/candidates.txt"
#define POSITIONS_FILE "data/positions.txt"
#define VOTES_FILE "data/votes.txt"
#define STATUS_FILE "data/election_status.txt"
#define APPLICATIONS_FILE "data/applications.txt"

// Size limits
#define MAX_NAME_LEN 64
#define MAX_PASS_LEN 128
#define MAX_LINE_LEN 256
#define MAX_VOTERS 500
#define MAX_CANDIDATES 200
#define MAX_POSITIONS 20
#define MAX_VOTES 2000

// Pipe delimiter
#define DELIM "|"

// Admin credentials
#define ADMIN_USER "admin"
#define ADMIN_PASS "admin123"

// Server configuration
#define SERVER_IP "0.0.0.0"  // Bind to all interfaces for network access
#define SERVER_PORT 9100
#define CMD_BUF_LEN 1024
#define BACKLOG 5

// Return codes
#define SUCCESS 0
#define ERR_FILE -1
#define ERR_NOT_FOUND -2
#define ERR_DUPLICATE -3
#define ERR_AUTH_FAIL -4
#define ERR_VOTED -5
#define ERR_CLOSED -6
#define ERR_FULL -7
#define ERR_CONN -8
#define ERR_UNKNOWN -9

// Data structures
typedef struct {
    int id;
    char name[MAX_NAME_LEN];
} Position;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int position_id;
} Candidate;

typedef struct {
    int id;
    int voter_id;
    char voter_name[MAX_NAME_LEN];
    int position_id;
    char position_name[MAX_NAME_LEN];
    int status; // 0 = pending, 1 = approved, 2 = rejected
} CandidateApplication;

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    int has_voted;
} Voter;

typedef struct {
    int logged_in;
    int is_admin;
    int voter_id;
    char voter_name[MAX_NAME_LEN];
} Session;

typedef struct {
    int candidate_id;
    char candidate_name[MAX_NAME_LEN];
    int position_id;
    int vote_count;
    float percentage;
} Result;

#endif // CONFIG_H
