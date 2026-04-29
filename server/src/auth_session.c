#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "auth_session.h"

#define MAX_CLIENTS 100
#define SESSION_TIMEOUT 300  // 5 minutes

static ClientSession sessions[MAX_CLIENTS];
static int session_count = 0;

void session_init(void) {
    memset(sessions, 0, sizeof(sessions));
    session_count = 0;
}

// Compare two sockaddr_in structures
static int addr_equal(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_family == b->sin_family &&
           a->sin_port == b->sin_port &&
           a->sin_addr.s_addr == b->sin_addr.s_addr;
}

// Find existing session or create new one
static int session_find_index(const struct sockaddr_in *client_addr) {
    for (int i = 0; i < session_count; i++) {
        if (addr_equal(&sessions[i].client_addr, client_addr)) {
            return i;
        }
    }
    return -1;
}

int session_find_or_create(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    
    if (index == -1) {
        // Create new session
        if (session_count >= MAX_CLIENTS) {
            return -1; // Session full
        }
        
        index = session_count;
        sessions[index].client_addr = *client_addr;
        sessions[index].logged_in = 0;
        sessions[index].is_admin = 0;
        sessions[index].voter_id = -1;
        sessions[index].voter_name[0] = '\0';
        sessions[index].last_activity = time(NULL);
        session_count++;
    }
    
    sessions[index].last_activity = time(NULL);
    return index;
}

int session_auth_admin(const struct sockaddr_in *client_addr, const char *username, const char *password) {
    if (strcmp(username, ADMIN_USER) != 0 || strcmp(password, ADMIN_PASS) != 0) {
        return ERR_AUTH_FAIL;
    }
    
    int index = session_find_or_create(client_addr);
    if (index == -1) return ERR_FULL;
    
    sessions[index].logged_in = 1;
    sessions[index].is_admin = 1;
    sessions[index].voter_id = -1;
    sessions[index].voter_name[0] = '\0';
    sessions[index].last_activity = time(NULL);
    
    return SUCCESS;
}

int session_auth_voter(const struct sockaddr_in *client_addr, int voter_id, const char *plain_password) {
    // This would need voter validation logic
    // For now, just create a basic voter session
    int index = session_find_or_create(client_addr);
    if (index == -1) return ERR_FULL;
    
    sessions[index].logged_in = 1;
    sessions[index].is_admin = 0;
    sessions[index].voter_id = voter_id;
    snprintf(sessions[index].voter_name, MAX_NAME_LEN, "Voter_%d", voter_id);
    sessions[index].last_activity = time(NULL);
    
    return SUCCESS;
}

void session_logout(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    if (index != -1) {
        memset(&sessions[index], 0, sizeof(ClientSession));
    }
}

int session_is_admin(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    return (index != -1) ? sessions[index].is_admin : 0;
}

int session_is_logged_in(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    return (index != -1) ? sessions[index].logged_in : 0;
}

int session_get_voter_id(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    return (index != -1) ? sessions[index].voter_id : -1;
}

const char *session_get_voter_name(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    if (index != -1 && sessions[index].logged_in) {
        return sessions[index].voter_name;
    }
    return "";
}

void session_cleanup_old(void) {
    time_t now = time(NULL);
    
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].logged_in && (now - sessions[i].last_activity > SESSION_TIMEOUT)) {
            printf("[+] Session timeout for client %s:%d\n", 
                   inet_ntoa(sessions[i].client_addr.sin_addr),
                   ntohs(sessions[i].client_addr.sin_port));
            memset(&sessions[i], 0, sizeof(ClientSession));
        }
    }
}
