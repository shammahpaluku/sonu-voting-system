#ifndef VOTER_H
#define VOTER_H
#include "config.h"

typedef struct {
    int  id;
    char name[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];  /* always stored as hash, never plain text */
    int  has_voted;               /* 0 = not voted, 1 = voted */
} Voter;

int  voter_register(const char *name, const char *plain_password);
int  voter_get_by_id(int id, Voter *out);
int  voter_get_all(Voter out[], int *count);
int  voter_validate_id(int id);
int  voter_has_voted(int id);
int  voter_mark_voted(int id);
int  voter_next_id(void);
void voter_display_all(void);

#endif /* VOTER_H */
