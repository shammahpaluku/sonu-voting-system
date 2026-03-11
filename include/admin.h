#ifndef ADMIN_H
#define ADMIN_H
#include "config.h"

void admin_menu(void);
void admin_manage_positions(void);
void admin_open_voting(void);
void admin_close_voting(void);
int  admin_get_election_status(char *out_status);
void admin_view_status(void);
void admin_reset_system(void);

#endif /* ADMIN_H */
