#ifndef CANDIDATE_H
#define CANDIDATE_H

int  cand_register(const char *name, int position_id);
int  cand_get_all(Candidate *candidates, int max);
int  cand_get_by_id(int id, Candidate *out);
int  cand_get_for_position(int position_id, Candidate *out, int max);
int  cand_validate_id(int candidate_id, int position_id);
int  cand_next_id(void);
void cand_display_all(void);
void cand_display_for_position(int position_id);

#endif // CANDIDATE_H
