#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "voting.h"
#include "file_handler.h"
#include "utils.h"
#include "auth.h"
#include "voter.h"
#include "position.h"
#include "candidate.h"

int voting_is_open(void) {
    char lines[1][MAX_LINE_LEN];
    int count = fh_read_all(STATUS_FILE, lines, 1);
    
    if (count == ERR_FILE) {
        return 0;
    }
    
    utils_trim(lines[0]);
    
    return (strcmp(lines[0], "OPEN") == 0) ? 1 : 0;
}

int voting_voter_has_voted(int voter_id) {
    return voter_has_voted(voter_id);
}

int voting_get_total_count(void) {
    int count = fh_count_records(VOTES_FILE);
    return (count == ERR_FILE) ? 0 : count;
}

void voting_display_ballot(void) {
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    if (pos_count == ERR_FILE || pos_count == 0) {
        printf("No positions available.\n");
        return;
    }
    
    for (int i = 0; i < pos_count; i++) {
        cand_display_for_position(positions[i].id);
        if (i < pos_count - 1) {
            utils_print_separator();
        }
    }
}

int voting_cast_vote(int voter_id, int position_id, int candidate_id) {
    if (voting_is_open() == 0) {
        return ERR_CLOSED;
    }
    
    if (voting_voter_has_voted(voter_id) == 1) {
        return ERR_VOTED;
    }
    
    if (cand_validate_id(candidate_id, position_id) != SUCCESS) {
        return ERR_NOT_FOUND;
    }
    
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%d|%d", voter_id, position_id, candidate_id);
    
    return fh_append_record(VOTES_FILE, record);
}

void voting_process(void) {
    // STEP 1 — Check election is open
    if (voting_is_open() == 0) {
        printf("Voting is currently closed.\n");
        return;
    }
    
    // STEP 2 — Check voter has not already voted
    int voter_id = auth_get_voter_id();
    if (voting_voter_has_voted(voter_id) == 1) {
        printf("You have already cast your vote.\n");
        return;
    }
    
    // STEP 3 — Display ballot and collect votes
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    if (pos_count == ERR_FILE || pos_count == 0) {
        printf("No positions on the ballot.\n");
        return;
    }
    
    printf("================================\n");
    printf("OFFICIAL BALLOT\n");
    printf("%s\n", auth_get_voter_name());
    printf("================================\n");
    
    int votes[MAX_POSITIONS] = {0};
    
    for (int i = 0; i < pos_count; i++) {
        utils_clear_screen();
        printf("Position %d of %d: %s\n\n", i + 1, pos_count, positions[i].name);
        
        Candidate candidates[MAX_CANDIDATES];
        int cand_count = cand_get_for_position(positions[i].id, candidates, MAX_CANDIDATES);
        
        if (cand_count == ERR_FILE || cand_count == 0) {
            printf("No candidates. Skipping.\n");
            utils_pause();
            continue;
        }
        
        cand_display_for_position(positions[i].id);
        
        int choice;
        while (1) {
            choice = utils_get_int("Enter candidate ID: ", 1, 9999);
            
            if (cand_validate_id(choice, positions[i].id) == SUCCESS) {
                votes[i] = choice;
                break;
            } else {
                printf("Invalid candidate for this position.\n");
            }
        }
    }
    
    // STEP 4 — Confirm before committing
    utils_clear_screen();
    printf("================================\n");
    printf("CONFIRM YOUR VOTES\n");
    printf("================================\n");
    
    for (int i = 0; i < pos_count; i++) {
        if (votes[i] != 0) {
            Candidate cand;
            if (cand_get_by_id(votes[i], &cand) == SUCCESS) {
                printf("%s --> %s\n", positions[i].name, cand.name);
            }
        }
    }
    
    int confirm = utils_get_int("Submit votes? (1=Yes / 0=No): ", 0, 1);
    
    if (confirm == 0) {
        printf("Vote cancelled. No votes recorded.\n");
        return;
    }
    
    // STEP 5 — Commit all votes
    int success_count = 0;
    
    for (int i = 0; i < pos_count; i++) {
        if (votes[i] != 0) {
            int result = voting_cast_vote(voter_id, positions[i].id, votes[i]);
            if (result != SUCCESS) {
                printf("Error recording vote for %s. Skipping.\n", positions[i].name);
            } else {
                success_count++;
            }
        }
    }
    
    // Mark voter as voted AFTER all positions are processed
    if (success_count > 0) {
        voter_mark_voted(voter_id);
        
        printf("================================\n");
        printf("Your votes have been recorded.\n");
        printf("Thank you for participating.\n");
        printf("================================\n");
        utils_pause();
    } else {
        printf("No votes were successfully recorded.\n");
        utils_pause();
    }
}
