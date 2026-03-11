#ifndef CANDIDATE_H
#define CANDIDATE_H
#include "config.h"

typedef struct {
    int  id;
    char name[MAX_NAME_LEN];
    int  position_id;
} Candidate;

int  cand_register(const char *name, int position_id);
int  cand_delete(int id);
int  cand_get_by_id(int id, Candidate *out);
int  cand_get_all(Candidate out[], int *count);
int  cand_get_for_position(int position_id, Candidate out[], int *count);
int  cand_validate_id(int id, int position_id);
int  cand_next_id(void);
void cand_display_all(void);
void cand_display_for_position(int position_id);

#endif /* CANDIDATE_H */
