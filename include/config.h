#ifndef CONFIG_H
#define CONFIG_H

/* ── Data file paths ── */
#define VOTERS_FILE     "data/voters.txt"
#define CANDIDATES_FILE "data/candidates.txt"
#define POSITIONS_FILE  "data/positions.txt"
#define VOTES_FILE      "data/votes.txt"
#define STATUS_FILE     "data/election_status.txt"
#define DATA_DIR        "data"

/* ── Field size limits ── */
#define MAX_NAME_LEN    64
#define MAX_PASS_LEN    128
#define MAX_LINE_LEN    256
#define MAX_VOTERS      500
#define MAX_CANDIDATES  100
#define MAX_POSITIONS   20

/* ── File delimiter ── */
#define DELIM           "|"
#define DELIM_CHAR      '|'

/* ── Admin credentials ── */
#define ADMIN_USERNAME  "admin"
#define ADMIN_PASSWORD  "admin123"

/* ── Election status strings ── */
#define STATUS_OPEN     "OPEN"
#define STATUS_CLOSED   "CLOSED"
#define STATUS_PENDING  "PENDING"

/* ── Return codes ── */
#define SUCCESS          0
#define ERR_FILE        -1
#define ERR_NOT_FOUND   -2
#define ERR_DUPLICATE   -3
#define ERR_AUTH_FAIL   -4
#define ERR_FULL        -5
#define ERR_VOTED       -6
#define ERR_CLOSED      -7

#endif /* CONFIG_H */
