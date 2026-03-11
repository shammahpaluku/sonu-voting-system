#ifndef TALLY_H
#define TALLY_H
#include "config.h"

typedef struct {
    int   candidate_id;
    char  candidate_name[MAX_NAME_LEN];
    int   position_id;
    int   vote_count;
    float percentage;
} Result;

int   tally_compute(Result results[], int *count);
void  tally_display_results(void);
int   tally_get_votes_for(int candidate_id, int position_id);
int   tally_get_winner(int position_id, Result *out);
float tally_voter_turnout(void);
int   tally_export(const char *filepath);

#endif /* TALLY_H */
