#ifndef AUTH_H
#define AUTH_H
#include "config.h"

typedef struct {
    int  logged_in;
    int  is_admin;
    int  voter_id;
    char voter_name[MAX_NAME_LEN];
} Session;

int         auth_voter_login(int voter_id, const char *plain_password);
int         auth_admin_login(const char *username, const char *password);
void        auth_logout(void);
int         auth_is_logged_in(void);
int         auth_is_admin(void);
int         auth_get_voter_id(void);
const char *auth_get_voter_name(void);

#endif /* AUTH_H */
