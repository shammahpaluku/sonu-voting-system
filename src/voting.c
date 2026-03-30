#include <stdio.h>
#include <string.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "auth.h"
#include "voter.h"
#include "candidate.h"
#include "position.h"
#include "voting.h"

int voting_is_open(void) {
    char lines[1][MAX_LINE_LEN];
    int count = 0;
    if (fh_read_all(STATUS_FILE, lines, &count) != SUCCESS) return 0;
    if (count == 0) return 0;
    return strcmp(lines[0], STATUS_OPEN) == 0;
}

int voting_voter_has_voted(int voter_id) {
    return voter_has_voted(voter_id);
}

int voting_cast_vote(int voter_id, int position_id, int candidate_id) {
    /* 1. Check election is open */
    if (!voting_is_open()) return ERR_CLOSED;

    /* 2. Check voter has not already voted */
    if (voter_has_voted(voter_id)) return ERR_VOTED;

    /* 3. Validate candidate belongs to position */
    if (cand_validate_id(candidate_id, position_id) != SUCCESS)
        return ERR_NOT_FOUND;

    /* 4. Record vote */
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%d|%d",
             voter_id, position_id, candidate_id);
    if (fh_append_record(VOTES_FILE, record) != SUCCESS) return ERR_FILE;

    return SUCCESS;
    /* NOTE: do NOT call voter_mark_voted here.
       Call it only after voter has voted for ALL positions. */
}

int voting_get_total_count(void) {
    return fh_count_records(VOTES_FILE);
}

void voting_display_ballot(void) {
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    
    if (pos_get_all(positions, &pos_count) != SUCCESS) {
        printf("Error loading positions.\n");
        return;
    }
    
    printf("\n=== ELECTION BALLOT ===\n");
    
    for (int i = 0; i < pos_count; i++) {
        printf("\n--- Voting for: %s ---\n", positions[i].name);
        cand_display_for_position(positions[i].id);
    }
}

void voting_process(void) {
    if (!voting_is_open()) {
        printf("Voting is currently closed.\n");
        utils_pause(); return;
    }
    
    int voter_id = auth_get_voter_id();
    if (voter_has_voted(voter_id)) {
        printf("You have already cast your vote.\n");
        utils_pause(); return;
    }
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    pos_get_all(positions, &pos_count);

    for (int i = 0; i < pos_count; i++) {
        utils_clear_screen();
        printf("--- Voting for: %s ---\n", positions[i].name);
        cand_display_for_position(positions[i].id);
        int choice = utils_get_int("Enter candidate ID: ", 1, 9999);
        int res = voting_cast_vote(voter_id, positions[i].id, choice);
        if (res != SUCCESS) {
            printf("Invalid choice. Skipped.\n");
            utils_pause();
        }
    }
    
    voter_mark_voted(voter_id);   /* mark AFTER voting all positions */
    printf("Your votes have been recorded. Thank you.\n");
    utils_pause();
}
