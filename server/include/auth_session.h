#ifndef AUTH_SESSION_H
#define AUTH_SESSION_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"

// Session tracking by client address
typedef struct {
    struct sockaddr_in client_addr;
    int logged_in;
    int is_admin;
    int voter_id;
    char voter_name[MAX_NAME_LEN];
    time_t last_activity;
} ClientSession;

// Session management functions
void session_init(void);
int session_find_or_create(const struct sockaddr_in *client_addr);
int session_auth_admin(const struct sockaddr_in *client_addr, const char *username, const char *password);
int session_auth_voter(const struct sockaddr_in *client_addr, int voter_id, const char *password);
void session_logout(const struct sockaddr_in *client_addr);
int session_is_admin(const struct sockaddr_in *client_addr);
int session_is_logged_in(const struct sockaddr_in *client_addr);
int session_get_voter_id(const struct sockaddr_in *client_addr);
const char *session_get_voter_name(const struct sockaddr_in *client_addr);
void session_cleanup_old(void);

#endif // AUTH_SESSION_H
