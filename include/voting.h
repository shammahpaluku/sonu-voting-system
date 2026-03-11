#ifndef VOTING_H
#define VOTING_H
#include "config.h"

int  voting_cast_vote(int voter_id, int position_id, int candidate_id);
int  voting_is_open(void);
int  voting_voter_has_voted(int voter_id);
void voting_display_ballot(void);
int  voting_get_total_count(void);
void voting_process(void);

#endif /* VOTING_H */
