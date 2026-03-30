#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "voter.h"
#include "candidate.h"
#include "position.h"
#include "tally.h"

typedef struct {
    int voter_id;
    int position_id;
    int candidate_id;
} VoteRecord;

int tally_get_votes_for(int candidate_id, int position_id) {
    char lines[MAX_VOTERS][MAX_LINE_LEN];
    int count = 0;
    
    if (fh_read_all(VOTES_FILE, lines, &count) != SUCCESS) return 0;
    
    int vote_count = 0;
    for (int i = 0; i < count; i++) {
        char copy[MAX_LINE_LEN];
        strncpy(copy, lines[i], MAX_LINE_LEN - 1);
        copy[MAX_LINE_LEN - 1] = '\0';
        
        char *tok = strtok(copy, DELIM);
        int pos_id = tok ? atoi(tok) : 0;
        tok = strtok(NULL, DELIM);
        int cand_id = tok ? atoi(tok) : 0;
        
        if (cand_id == candidate_id && pos_id == position_id) {
            vote_count++;
        }
    }
    
    return vote_count;
}

int tally_get_winner(int position_id, Result *out) {
    Result results[MAX_CANDIDATES];
    int count = 0;
    
    if (tally_compute(results, &count) != SUCCESS) return ERR_FILE;
    
    int max_votes = -1;
    int winner_index = -1;
    
    for (int i = 0; i < count; i++) {
        if (results[i].position_id == position_id && results[i].vote_count > max_votes) {
            max_votes = results[i].vote_count;
            winner_index = i;
        }
    }
    
    if (winner_index >= 0) {
        *out = results[winner_index];
        return SUCCESS;
    }
    
    return ERR_NOT_FOUND;
}

float tally_voter_turnout(void) {
    int total_voters = fh_count_records(VOTERS_FILE);
    /* Count unique voters who have voted by checking has_voted field */
    Voter voters[MAX_VOTERS];
    int count = 0;
    voter_get_all(voters, &count);
    int voted = 0;
    for (int i = 0; i < count; i++)
        if (voters[i].has_voted) voted++;
    if (total_voters == 0) return 0.0f;
    return (voted / (float)total_voters) * 100.0f;
}

int tally_compute(Result results[], int *count) {
    Candidate candidates[MAX_CANDIDATES];
    int cand_count = 0;
    
    if (cand_get_all(candidates, &cand_count) != SUCCESS) return ERR_FILE;
    
    *count = 0;
    
    /* For each candidate, count votes */
    for (int i = 0; i < cand_count; i++) {
        int vote_count = tally_get_votes_for(candidates[i].id, candidates[i].position_id);
        
        if (vote_count > 0) {  /* Only include candidates with votes */
            results[*count].candidate_id = candidates[i].id;
            strncpy(results[*count].candidate_name, candidates[i].name, MAX_NAME_LEN - 1);
            results[*count].candidate_name[MAX_NAME_LEN - 1] = '\0';
            results[*count].position_id = candidates[i].position_id;
            results[*count].vote_count = vote_count;
            (*count)++;
        }
    }
    
    /* Calculate percentages for each position */
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    pos_get_all(positions, &pos_count);
    
    for (int p = 0; p < pos_count; p++) {
        int total_for_position = 0;
        for (int i = 0; i < *count; i++) {
            if (results[i].position_id == positions[p].id) {
                total_for_position += results[i].vote_count;
            }
        }
        
        /* Calculate percentage for this position */
        for (int i = 0; i < *count; i++) {
            if (results[i].position_id == positions[p].id) {
                results[i].percentage = total_for_position > 0 ? 
                    (float)results[i].vote_count / total_for_position * 100.0f : 0.0f;
            }
        }
    }
    
    return SUCCESS;
}

void tally_display_results(void) {
    Result results[MAX_CANDIDATES];
    int count = 0;
    
    if (tally_compute(results, &count) != SUCCESS) {
        printf("Error computing results.\n");
        return;
    }
    
    printf("\n=== ELECTION RESULTS ===\n");
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    pos_get_all(positions, &pos_count);
    
    for (int p = 0; p < pos_count; p++) {
        printf("\n--- Results for: %s ---\n", positions[p].name);
        printf("ID\tName\t\t\tVotes\tPercentage\n");
        printf("-----------------------------------------------\n");
        
        for (int i = 0; i < count; i++) {
            if (results[i].position_id == positions[p].id) {
                printf("%d\t%-20s\t%d\t%.1f%%\n", 
                       results[i].candidate_id, 
                       results[i].candidate_name,
                       results[i].vote_count,
                       results[i].percentage);
            }
        }
        
        Result winner;
        if (tally_get_winner(positions[p].id, &winner) == SUCCESS) {
            printf("\nWINNER: %s (%d votes)\n", 
                   winner.candidate_name, winner.vote_count);
        }
    }
    
    printf("\n--- SUMMARY ---\n");
    printf("Total Votes Cast: %d\n", fh_count_records(VOTES_FILE));
    printf("Voter Turnout: %.1f%%\n", tally_voter_turnout());
    printf("\n");
}

int tally_export(const char *filepath) {
    Result results[MAX_CANDIDATES];
    int count = 0;
    
    if (tally_compute(results, &count) != SUCCESS) return ERR_FILE;
    
    FILE *fp = fopen(filepath, "w");
    if (!fp) return ERR_FILE;
    
    fprintf(fp, "ELECTION RESULTS\n");
    fprintf(fp, "================\n\n");
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    pos_get_all(positions, &pos_count);
    
    for (int p = 0; p < pos_count; p++) {
        fprintf(fp, "Results for: %s\n", positions[p].name);
        fprintf(fp, "ID,Name,Votes,Percentage\n");
        
        for (int i = 0; i < count; i++) {
            if (results[i].position_id == positions[p].id) {
                fprintf(fp, "%d,%s,%d,%.1f%%\n", 
                        results[i].candidate_id, 
                        results[i].candidate_name,
                        results[i].vote_count,
                        results[i].percentage);
            }
        }
        
        Result winner;
        if (tally_get_winner(positions[p].id, &winner) == SUCCESS) {
            fprintf(fp, "\nWINNER: %s (%d votes)\n\n", 
                    winner.candidate_name, winner.vote_count);
        }
    }
    
    fprintf(fp, "Total Votes Cast: %d\n", fh_count_records(VOTES_FILE));
    fprintf(fp, "Voter Turnout: %.1f%%\n", tally_voter_turnout());
    
    fclose(fp);
    return SUCCESS;
}
