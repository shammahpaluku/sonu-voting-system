#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "config.h"
#include "tally.h"
#include "file_handler.h"
#include "utils.h"
#include "position.h"
#include "candidate.h"

typedef struct {
    int voter_id;
    int position_id;
    int candidate_id;
} Vote;

int tally_compute(TallyResult *results, int *count) {
    // Load all votes
    char vote_lines[MAX_VOTES][MAX_LINE_LEN];
    int vote_total = fh_read_all(VOTES_FILE, vote_lines, MAX_VOTES);
    
    if (vote_total == ERR_FILE) {
        return ERR_FILE;
    }
    
    Vote votes[MAX_VOTES];
    int vote_count = 0;
    
    for (int i = 0; i < vote_total; i++) {
        char buf[MAX_LINE_LEN];
        strncpy(buf, vote_lines[i], MAX_LINE_LEN - 1);
        buf[MAX_LINE_LEN - 1] = '\0';
        
        char *token = strtok(buf, DELIM);
        if (!token) continue;
        
        votes[vote_count].voter_id = atoi(token);
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        votes[vote_count].position_id = atoi(token);
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        votes[vote_count].candidate_id = atoi(token);
        
        vote_count++;
    }
    
    // Load all candidates
    Candidate candidates[MAX_CANDIDATES];
    int cand_count = cand_get_all(candidates, MAX_CANDIDATES);
    
    if (cand_count == ERR_FILE) {
        return ERR_FILE;
    }
    
    // Load all positions
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    if (pos_count == ERR_FILE) {
        return ERR_FILE;
    }
    
    // Initialize results for each candidate
    for (int i = 0; i < cand_count; i++) {
        results[i].candidate_id = candidates[i].id;
        strncpy(results[i].candidate_name, candidates[i].name, MAX_NAME_LEN - 1);
        results[i].candidate_name[MAX_NAME_LEN - 1] = '\0';
        results[i].position_id = candidates[i].position_id;
        results[i].vote_count = 0;
        results[i].percentage = 0.0f;
        
        // Find position name
        for (int j = 0; j < pos_count; j++) {
            if (positions[j].id == candidates[i].position_id) {
                strncpy(results[i].position_name, positions[j].name, MAX_NAME_LEN - 1);
                results[i].position_name[MAX_NAME_LEN - 1] = '\0';
                break;
            }
        }
    }
    
    // Count votes per candidate
    for (int i = 0; i < vote_count; i++) {
        for (int j = 0; j < cand_count; j++) {
            if (results[j].candidate_id == votes[i].candidate_id) {
                results[j].vote_count++;
                break;
            }
        }
    }
    
    // Calculate percentages per position
    for (int p = 0; p < pos_count; p++) {
        int total_for_position = 0;
        
        // Sum votes for this position
        for (int i = 0; i < cand_count; i++) {
            if (results[i].position_id == positions[p].id) {
                total_for_position += results[i].vote_count;
            }
        }
        
        // Calculate percentages
        if (total_for_position == 0) {
            for (int i = 0; i < cand_count; i++) {
                if (results[i].position_id == positions[p].id) {
                    results[i].percentage = 0.0f;
                }
            }
        } else {
            for (int i = 0; i < cand_count; i++) {
                if (results[i].position_id == positions[p].id) {
                    results[i].percentage = (results[i].vote_count / (float)total_for_position) * 100.0f;
                }
            }
        }
    }
    
    *count = cand_count;
    return SUCCESS;
}

void tally_display_results(const TallyResult *results, int count) {
    if (count == 0) {
        printf("No results to display.\n");
        return;
    }
    
    // Group by position
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    if (pos_count == ERR_FILE) {
        printf("Error loading positions.\n");
        return;
    }
    
    int total_votes = 0;
    for (int i = 0; i < count; i++) {
        total_votes += results[i].vote_count;
    }
    
    for (int p = 0; p < pos_count; p++) {
        printf("============================================================\n");
        printf("POSITION: %s\n", positions[p].name);
        printf("============================================================\n");
        printf("%-24s %6s %10s\n", "Candidate Name", "Votes", "Percentage");
        printf("------------------------------------------------------------\n");
        
        TallyResult winner;
        winner.vote_count = -1;
        
        for (int i = 0; i < count; i++) {
            if (results[i].position_id == positions[p].id) {
                printf("%-24s %6d %9.2f%%\n", results[i].candidate_name, 
                       results[i].vote_count, results[i].percentage);
                
                if (results[i].vote_count > winner.vote_count) {
                    winner = results[i];
                }
            }
        }
        
        printf("------------------------------------------------------------\n");
        if (winner.vote_count >= 0) {
            printf("Winner: %s (%d votes)\n", winner.candidate_name, winner.vote_count);
        }
        printf("============================================================\n\n");
    }
    
    printf("Total votes cast: %d\n", total_votes);
    printf("Voter turnout:    %.2f%%\n", tally_voter_turnout());
}

void tally_get_winner(const TallyResult *results, int count,
                      int position_id, TallyResult *winner) {
    winner->vote_count = -1;
    
    for (int i = 0; i < count; i++) {
        if (results[i].position_id == position_id && 
            results[i].vote_count > winner->vote_count) {
            *winner = results[i];
        }
    }
}

float tally_voter_turnout(void) {
    int total_voters = fh_count_records(VOTERS_FILE);
    if (total_voters == ERR_FILE || total_voters == 0) {
        return 0.0f;
    }
    
    // Count unique voter IDs in votes
    char vote_lines[MAX_VOTES][MAX_LINE_LEN];
    int vote_line_count = fh_read_all(VOTES_FILE, vote_lines, MAX_VOTES);
    
    if (vote_line_count == ERR_FILE) {
        return 0.0f;
    }
    
    int unique_voters[MAX_VOTERS];
    int unique_count = 0;
    
    for (int i = 0; i < vote_line_count; i++) {
        char buf[MAX_LINE_LEN];
        strncpy(buf, vote_lines[i], MAX_LINE_LEN - 1);
        buf[MAX_LINE_LEN - 1] = '\0';
        
        char *token = strtok(buf, DELIM);
        if (!token) continue;
        
        int voter_id = atoi(token);
        
        // Check if already counted
        int found = 0;
        for (int j = 0; j < unique_count; j++) {
            if (unique_voters[j] == voter_id) {
                found = 1;
                break;
            }
        }
        
        if (!found && unique_count < MAX_VOTERS) {
            unique_voters[unique_count] = voter_id;
            unique_count++;
        }
    }
    
    return (unique_count / (float)total_voters) * 100.0f;
}

int tally_export(const TallyResult *results, int count) {
    FILE *fp = fopen("data/results_export.txt", "w");
    if (!fp) {
        return ERR_FILE;
    }
    
    // Header
    fprintf(fp, "SONU ELECTION RESULTS\n");
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(fp, "Exported: %s\n\n", timestamp);
    
    // Group by position and write results
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    if (pos_count != ERR_FILE) {
        for (int p = 0; p < pos_count; p++) {
            fprintf(fp, "============================================================\n");
            fprintf(fp, "POSITION: %s\n", positions[p].name);
            fprintf(fp, "============================================================\n");
            fprintf(fp, "%-24s %6s %10s\n", "Candidate Name", "Votes", "Percentage");
            fprintf(fp, "------------------------------------------------------------\n");
            
            TallyResult winner;
            winner.vote_count = -1;
            
            for (int i = 0; i < count; i++) {
                if (results[i].position_id == positions[p].id) {
                    fprintf(fp, "%-24s %6d %9.2f%%\n", results[i].candidate_name, 
                           results[i].vote_count, results[i].percentage);
                    
                    if (results[i].vote_count > winner.vote_count) {
                        winner = results[i];
                    }
                }
            }
            
            fprintf(fp, "------------------------------------------------------------\n");
            if (winner.vote_count >= 0) {
                fprintf(fp, "Winner: %s (%d votes)\n", winner.candidate_name, winner.vote_count);
            }
            fprintf(fp, "============================================================\n\n");
        }
    }
    
    // Summary
    int total_votes = 0;
    for (int i = 0; i < count; i++) {
        total_votes += results[i].vote_count;
    }
    
    fprintf(fp, "Total votes cast: %d\n", total_votes);
    fprintf(fp, "Voter turnout:    %.2f%%\n", tally_voter_turnout());
    
    fclose(fp);
    
    printf("Results exported to data/results_export.txt\n");
    return SUCCESS;
}
