#ifndef ADMIN_H
#define ADMIN_H

void admin_menu(void);
void admin_manage_positions(void);
void admin_manage_candidates(void);
void admin_register_voter(void);
int  admin_open_voting(void);
int  admin_close_voting(void);
int  admin_get_election_status(char *out, int out_len);
void admin_view_status(void);
int  admin_reset_system(void);
int  admin_reset_direct(void);

#endif // ADMIN_H
