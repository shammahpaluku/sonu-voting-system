#include <stdio.h>
#include <string.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "voter.h"
#include "auth.h"

/* auth.c — not visible outside this translation unit */
static Session current_session = {0, 0, -1, ""};

int auth_voter_login(int voter_id, const char *plain_password) {
    Voter v;
    if (voter_get_by_id(voter_id, &v) != SUCCESS) return ERR_AUTH_FAIL;
    
    char hashed[MAX_PASS_LEN];
    utils_hash_password(plain_password, hashed, MAX_PASS_LEN);
    
    if (strcmp(hashed, v.password) == 0) {
        current_session.logged_in = 1;
        current_session.is_admin = 0;
        current_session.voter_id = voter_id;
        strncpy(current_session.voter_name, v.name, MAX_NAME_LEN - 1);
        current_session.voter_name[MAX_NAME_LEN - 1] = '\0';
        return SUCCESS;
    }
    
    return ERR_AUTH_FAIL;
}

int auth_admin_login(const char *username, const char *password) {
    if (strcmp(username, ADMIN_USERNAME) == 0 && 
        strcmp(password, ADMIN_PASSWORD) == 0) {
        current_session.logged_in = 1;
        current_session.is_admin = 1;
        current_session.voter_id = -1;
        strcpy(current_session.voter_name, "Administrator");
        return SUCCESS;
    }
    
    return ERR_AUTH_FAIL;
}

void auth_logout(void) {
    current_session.logged_in = 0;
    current_session.is_admin = 0;
    current_session.voter_id = -1;
    strcpy(current_session.voter_name, "");
}

int auth_is_logged_in(void) {
    return current_session.logged_in;
}

int auth_is_admin(void) {
    return current_session.is_admin;
}

int auth_get_voter_id(void) {
    return current_session.voter_id;
}

const char *auth_get_voter_name(void) {
    return current_session.voter_name;
}
