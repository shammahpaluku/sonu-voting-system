#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "auth.h"
#include "voter.h"
#include "utils.h"

static Session current_session;

int auth_voter_login(int voter_id, const char *plain_password) {
    Voter v;
    int result = voter_get_by_id(voter_id, &v);
    
    if (result != SUCCESS) {
        return ERR_NOT_FOUND;
    }
    
    char hashed_password[MAX_PASS_LEN];
    utils_hash_password(plain_password, hashed_password, MAX_PASS_LEN);
    
    if (strcmp(hashed_password, v.password) != 0) {
        return ERR_AUTH_FAIL;
    }
    
    current_session.logged_in = 1;
    current_session.is_admin = 0;
    current_session.voter_id = voter_id;
    strncpy(current_session.voter_name, v.name, MAX_NAME_LEN - 1);
    current_session.voter_name[MAX_NAME_LEN - 1] = '\0';
    
    return SUCCESS;
}

int auth_admin_login(const char *username, const char *password) {
    if (strcmp(username, ADMIN_USER) != 0 || strcmp(password, ADMIN_PASS) != 0) {
        return ERR_AUTH_FAIL;
    }
    
    current_session.logged_in = 1;
    current_session.is_admin = 1;
    current_session.voter_id = -1;
    current_session.voter_name[0] = '\0';
    
    return SUCCESS;
}

void auth_logout(void) {
    memset(&current_session, 0, sizeof(current_session));
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
