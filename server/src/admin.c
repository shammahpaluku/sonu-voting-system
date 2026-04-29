#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "admin.h"
#include "auth.h"
#include "voter.h"
#include "position.h"
#include "candidate.h"
#include "file_handler.h"
#include "utils.h"
#include "tally.h"

int admin_get_election_status(char *out, int out_len) {
    // Hardcode status to always be "OPEN"
    strncpy(out, "OPEN", out_len - 1);
    out[out_len - 1] = '\0';
    return SUCCESS;
}

int admin_open_voting(void) {
    // Only require admin authentication - no data validation needed
    printf("Admin opening voting...\n");
    
    // Write "OPEN" to STATUS_FILE
    char lines[1][MAX_LINE_LEN] = {"OPEN"};
    int result = fh_write_all(STATUS_FILE, lines, 1);
    
    if (result == SUCCESS) {
        printf("Voting is now OPEN.\n");
    } else {
        printf("Failed to update voting status.\n");
    }
    
    return result;
}

int admin_close_voting(void) {
    char lines[1][MAX_LINE_LEN] = {"CLOSED"};
    int result = fh_write_all(STATUS_FILE, lines, 1);
    
    if (result == SUCCESS) {
        printf("Voting is now CLOSED.\n");
    }
    
    return result;
}

void admin_view_status(void) {
    char status[MAX_LINE_LEN];
    int result = admin_get_election_status(status, MAX_LINE_LEN);
    
    if (result == SUCCESS) {
        printf("Current election status: %s\n", status);
        
        int vote_count = fh_count_records(VOTES_FILE);
        if (vote_count != ERR_FILE) {
            printf("Total votes cast: %d\n", vote_count);
        }
    } else {
        printf("Error: Could not read election status.\n");
    }
}

int admin_reset_system(void) {
    char confirm[MAX_LINE_LEN];
    utils_get_string("Type CONFIRM to reset all data: ", confirm, MAX_LINE_LEN);
    
    if (strcmp(confirm, "CONFIRM") != 0) {
        printf("Reset cancelled.\n");
        return ERR_AUTH_FAIL;
    }
    
    return admin_reset_direct();
}

int admin_reset_direct(void) {
    // Clear data files by opening in write mode and closing immediately
    FILE *fp;
    
    fp = fopen(VOTERS_FILE, "w");
    if (fp) fclose(fp);
    
    fp = fopen(CANDIDATES_FILE, "w");
    if (fp) fclose(fp);
    
    fp = fopen(POSITIONS_FILE, "w");
    if (fp) fclose(fp);
    
    fp = fopen(VOTES_FILE, "w");
    if (fp) fclose(fp);
    
    // Reset status to PENDING
    char status_lines[1][MAX_LINE_LEN] = {"PENDING"};
    int result = fh_write_all(STATUS_FILE, status_lines, 1);
    
    return result;
}

void admin_register_voter(void) {
    char name[MAX_NAME_LEN];
    char password[MAX_PASS_LEN];
    
    utils_get_string("Enter voter name: ", name, MAX_NAME_LEN);
    utils_get_password("Enter voter password: ", password, MAX_PASS_LEN);
    
    int result = voter_register(name, password);
    
    switch (result) {
        case SUCCESS:
            printf("Voter registered. Voter ID: %d\n", result);
            break;
        case ERR_DUPLICATE:
            printf("A voter with that name already exists.\n");
            break;
        case ERR_FULL:
            printf("Maximum voter limit reached.\n");
            break;
        default:
            printf("Error registering voter.\n");
            break;
    }
}

void admin_manage_positions(void) {
    while (1) {
        utils_clear_screen();
        printf("=== Manage Positions ===\n\n");
        printf("1. Add Position\n");
        printf("2. View All Positions\n");
        printf("0. Back\n\n");
        
        int choice = utils_get_int("Enter choice: ", 0, 2);
        
        switch (choice) {
            case 1: {
                char name[MAX_NAME_LEN];
                utils_get_string("Enter position name: ", name, MAX_NAME_LEN);
                
                int result = pos_add(name);
                switch (result) {
                    case SUCCESS:
                        printf("Position added. ID: %d\n", result);
                        break;
                    case ERR_DUPLICATE:
                        printf("That position already exists.\n");
                        break;
                    default:
                        printf("Error adding position.\n");
                        break;
                }
                utils_pause();
                break;
            }
            case 2:
                pos_display_all();
                utils_pause();
                break;
            case 0:
                return;
        }
    }
}

void admin_manage_candidates(void) {
    while (1) {
        utils_clear_screen();
        printf("=== Manage Candidates ===\n\n");
        printf("1. Register Candidate\n");
        printf("2. View All Candidates\n");
        printf("0. Back\n\n");
        
        int choice = utils_get_int("Enter choice: ", 0, 2);
        
        switch (choice) {
            case 1: {
                pos_display_all();
                printf("\n");
                
                char name[MAX_NAME_LEN];
                utils_get_string("Enter candidate name: ", name, MAX_NAME_LEN);
                
                int position_id = utils_get_int("Enter position ID: ", 1, MAX_POSITIONS);
                
                int result = cand_register(name, position_id);
                switch (result) {
                    case SUCCESS:
                        printf("Candidate registered. ID: %d\n", result);
                        break;
                    case ERR_NOT_FOUND:
                        printf("Invalid position ID.\n");
                        break;
                    case ERR_DUPLICATE:
                        printf("Candidate already registered for that position.\n");
                        break;
                    default:
                        printf("Error registering candidate.\n");
                        break;
                }
                utils_pause();
                break;
            }
            case 2:
                cand_display_all();
                utils_pause();
                break;
            case 0:
                return;
        }
    }
}

void admin_menu(void) {
    while (1) {
        utils_clear_screen();
        
        char status[MAX_LINE_LEN];
        admin_get_election_status(status, MAX_LINE_LEN);
        
        printf("=== SONU Admin Panel ===\n");
        printf("Election status: %s\n\n", status);
        
        printf("1. Manage Positions\n");
        printf("2. Manage Candidates\n");
        printf("3. Register Voter\n");
        printf("4. Open Voting\n");
        printf("5. Close Voting\n");
        printf("6. View Results\n");
        printf("7. View Status\n");
        printf("8. Reset System\n");
        printf("0. Logout\n\n");
        
        int choice = utils_get_int("Enter choice: ", 0, 8);
        
        switch (choice) {
            case 1:
                admin_manage_positions();
                break;
            case 2:
                admin_manage_candidates();
                break;
            case 3:
                admin_register_voter();
                utils_pause();
                break;
            case 4:
                admin_open_voting();
                utils_pause();
                break;
            case 5:
                admin_close_voting();
                utils_pause();
                break;
            case 6: {
                // Status is always "OPEN" now - allow results anytime
                printf("Generating results (voting is always open)...\n");
                
                TallyResult results[MAX_CANDIDATES];
                int count = 0;
                
                int tally_result = tally_compute(results, &count);
                if (tally_result == SUCCESS) {
                    tally_display_results(results, count);
                    
                    int export_choice = utils_get_int("Export results to file? (1=Yes / 0=No): ", 0, 1);
                    if (export_choice == 1) {
                        tally_export(results, count);
                    }
                } else {
                    printf("Error computing results.\n");
                }
                utils_pause();
                break;
            }
            case 7:
                admin_view_status();
                utils_pause();
                break;
            case 8:
                admin_reset_system();
                utils_pause();
                break;
            case 0:
                auth_logout();
                return;
        }
    }
}
