#ifndef AUTH_H
#define AUTH_H

int         auth_voter_login(int voter_id, const char *plain_password);
int         auth_admin_login(const char *username, const char *password);
void        auth_logout(void);
int         auth_is_logged_in(void);
int         auth_is_admin(void);
int         auth_get_voter_id(void);
const char *auth_get_voter_name(void);

#endif // AUTH_H
