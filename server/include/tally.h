#ifndef TALLY_H
#define TALLY_H

typedef struct {
    int   candidate_id;
    char  candidate_name[MAX_NAME_LEN];
    int   position_id;
    char  position_name[MAX_NAME_LEN];
    int   vote_count;
    float percentage;
} TallyResult;

int  tally_compute(TallyResult *results, int *count);
void tally_display_results(const TallyResult *results, int count);
void tally_get_winner(const TallyResult *results, int count,
                      int position_id, TallyResult *winner);
float tally_voter_turnout(void);
int  tally_export(const TallyResult *results, int count);

#endif // TALLY_H
