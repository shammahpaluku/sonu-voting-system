#ifndef VOTER_H
#define VOTER_H

int  voter_register(const char *name, const char *plain_password);
int  voter_get_all(Voter *voters, int max);
int  voter_get_by_id(int id, Voter *out);
int  voter_validate_id(int id);
int  voter_has_voted(int id);
int  voter_mark_voted(int id);
int  voter_next_id(void);
void voter_display_all(void);

#endif // VOTER_H
