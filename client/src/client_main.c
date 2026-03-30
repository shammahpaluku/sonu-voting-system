#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "net_handler.h"

// Global state
static int  g_sock = -1;
static int  g_user_id = -1;
static char g_user_name[MAX_NAME_LEN];
static int  g_is_admin = 0;

// Helper functions
void client_send_recv(const char *cmd, char *response, int resp_len);
void client_recv_multiline(void);
void print_header(const char *title);
void print_error(const char *response);
int is_error(const char *response);
void replace_underscores(char *str);
void replace_spaces(char *str);
void pause_screen(void);

// Voter menu functions
void voter_handle_vote(void);
void voter_menu(void);

// Admin menu functions
void admin_print_positions(void);
void admin_handle_add_position(void);
void admin_handle_register_candidate(void);
void admin_handle_register_voter(void);
void admin_handle_results(void);
void admin_manage_applications(void);
void admin_menu(void);

// Login functions
void handle_voter_login(void);
void handle_voter_registration(void);
void handle_admin_login(void);
void handle_candidate_application(void);

// Main menu
void main_menu(void);

void client_send_recv(const char *cmd, char *response, int resp_len) {
    if (nh_send_line(g_sock, cmd) != SUCCESS) {
        printf("\nLost connection to server. Exiting.\n");
        nh_close(g_sock);
        exit(1);
    }
    
    if (nh_recv_line(g_sock, response, resp_len) != SUCCESS) {
        printf("\nLost connection to server. Exiting.\n");
        nh_close(g_sock);
        exit(1);
    }
}

void client_recv_multiline(void) {
    char line[CMD_BUF_LEN];
    
    while (1) {
        if (nh_recv_line(g_sock, line, sizeof(line)) != SUCCESS) {
            printf("\nLost connection to server. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(line, "END") == 0) {
            break;
        }
        
        if (is_error(line)) {
            print_error(line);
            break;
        }
        
        printf("%s\n", line);
    }
}

void print_header(const char *title) {
    printf("\033[2J\033[H");
    printf("============================================================\n");
    printf("SONU ELECTRONIC VOTING SYSTEM\n");
    printf("%s\n", title);
    printf("============================================================\n\n");
}

void print_error(const char *response) {
    if (strcmp(response, "ERR_NOT_FOUND") == 0) {
        printf("[!] Record not found.\n");
    } else if (strcmp(response, "ERR_AUTH_FAIL") == 0) {
        printf("[!] Invalid credentials.\n");
    } else if (strcmp(response, "ERR_DUPLICATE") == 0) {
        printf("[!] Record already exists.\n");
    } else if (strcmp(response, "ERR_VOTED") == 0) {
        printf("[!] You have already cast your vote.\n");
    } else if (strcmp(response, "ERR_CLOSED") == 0) {
        printf("[!] Voting is currently closed.\n");
    } else if (strcmp(response, "ERR_FULL") == 0) {
        printf("[!] Maximum limit reached.\n");
    } else if (strcmp(response, "ERR_EMPTY") == 0) {
        printf("[!] No records found.\n");
    } else if (strcmp(response, "ERR_UNKNOWN") == 0) {
        printf("[!] Unknown command sent to server.\n");
    } else {
        printf("[!] %s\n", response);
    }
}

int is_error(const char *response) {
    return strncmp(response, "ERR_", 4) == 0;
}

void replace_underscores(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == '_') {
            str[i] = ' ';
        }
    }
}

void replace_spaces(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') {
            str[i] = '_';
        }
    }
}

void pause_screen(void) {
    printf("\nPress Enter...");
    getchar();
}

void voter_handle_vote(void) {
    print_header("Cast Your Vote");
    
    // Check voting status
    char response[CMD_BUF_LEN];
    client_send_recv("STATUS", response, sizeof(response));
    
    if (strcmp(response, "OPEN") != 0) {
        printf("[!] Voting is not currently open.\n");
        pause_screen();
        return;
    }
    
    // Load positions
    client_send_recv("LIST_POSITIONS", response, sizeof(response));
    
    int pos_ids[MAX_POSITIONS];
    char pos_names[MAX_POSITIONS][MAX_NAME_LEN];
    int pos_count = 0;
    
    if (!is_error(response)) {
        // Parse first line
        char buf[CMD_BUF_LEN];
        strncpy(buf, response, CMD_BUF_LEN - 1);
        buf[CMD_BUF_LEN - 1] = '\0';
        
        char *token = strtok(buf, " ");
        if (token) {
            pos_ids[pos_count] = atoi(token);
            token = strtok(NULL, "");
            if (token) {
                strncpy(pos_names[pos_count], token, MAX_NAME_LEN - 1);
                pos_names[pos_count][MAX_NAME_LEN - 1] = '\0';
                replace_underscores(pos_names[pos_count]);
                pos_count++;
            }
        }
        
        // Read remaining lines
        while (pos_count < MAX_POSITIONS) {
            if (nh_recv_line(g_sock, response, sizeof(response)) != SUCCESS) {
                printf("\nLost connection to server. Exiting.\n");
                nh_close(g_sock);
                exit(1);
            }
            
            if (strcmp(response, "END") == 0) {
                break;
            }
            
            strncpy(buf, response, CMD_BUF_LEN - 1);
            buf[CMD_BUF_LEN - 1] = '\0';
            
            token = strtok(buf, " ");
            if (token) {
                pos_ids[pos_count] = atoi(token);
                token = strtok(NULL, "");
                if (token) {
                    strncpy(pos_names[pos_count], token, MAX_NAME_LEN - 1);
                    pos_names[pos_count][MAX_NAME_LEN - 1] = '\0';
                    replace_underscores(pos_names[pos_count]);
                    pos_count++;
                }
            }
        }
    }
    
    if (pos_count == 0) {
        printf("[!] No positions on the ballot.\n");
        pause_screen();
        return;
    }
    
    int votes[MAX_POSITIONS] = {0};
    
    // Collect votes for each position
    for (int i = 0; i < pos_count; i++) {
        print_header("Cast Your Vote");
        printf("Position %d of %d: %s\n\n", i + 1, pos_count, pos_names[i]);
        
        printf("------------------------------------------------------------\n");
        
        // Get candidates for this position
        char cmd[CMD_BUF_LEN];
        snprintf(cmd, CMD_BUF_LEN, "LIST_CANDS %d", pos_ids[i]);
        client_send_recv(cmd, response, sizeof(response));
        
        int cand_ids[MAX_CANDIDATES];
        char cand_names[MAX_CANDIDATES][MAX_NAME_LEN];
        int cand_count = 0;
        
        if (!is_error(response)) {
            // Parse candidates
            char buf[CMD_BUF_LEN];
            strncpy(buf, response, CMD_BUF_LEN - 1);
            buf[CMD_BUF_LEN - 1] = '\0';
            
            char *token = strtok(buf, " ");
            if (token) {
                cand_ids[cand_count] = atoi(token);
                token = strtok(NULL, "");
                if (token) {
                    strncpy(cand_names[cand_count], token, MAX_NAME_LEN - 1);
                    cand_names[cand_count][MAX_NAME_LEN - 1] = '\0';
                    replace_underscores(cand_names[cand_count]);
                    cand_count++;
                }
            }
            
            // Read remaining candidate lines
            while (cand_count < MAX_CANDIDATES) {
                if (nh_recv_line(g_sock, response, sizeof(response)) != SUCCESS) {
                    printf("\nLost connection to server. Exiting.\n");
                    nh_close(g_sock);
                    exit(1);
                }
                
                if (strcmp(response, "END") == 0) {
                    break;
                }
                
                strncpy(buf, response, CMD_BUF_LEN - 1);
                buf[CMD_BUF_LEN - 1] = '\0';
                
                token = strtok(buf, " ");
                if (token) {
                    cand_ids[cand_count] = atoi(token);
                    token = strtok(NULL, "");
                    if (token) {
                        strncpy(cand_names[cand_count], token, MAX_NAME_LEN - 1);
                        cand_names[cand_count][MAX_NAME_LEN - 1] = '\0';
                        replace_underscores(cand_names[cand_count]);
                        cand_count++;
                    }
                }
            }
        }
        
        if (cand_count == 0) {
            printf("No candidates for this position. Skipping.\n");
            pause_screen();
            continue;
        }
        
        // Display candidates
        printf("ID  | Candidate\n");
        printf("----|--------------------\n");
        for (int j = 0; j < cand_count; j++) {
            printf("%-4d| %s\n", cand_ids[j], cand_names[j]);
        }
        printf("\n");
        
        // Get valid candidate choice
        int choice;
        while (1) {
            printf("Enter candidate ID: ");
            char input[32];
            fgets(input, sizeof(input), stdin);
            
            if (sscanf(input, "%d", &choice) == 1) {
                int valid = 0;
                for (int j = 0; j < cand_count; j++) {
                    if (cand_ids[j] == choice) {
                        valid = 1;
                        break;
                    }
                }
                
                if (valid) {
                    votes[i] = choice;
                    break;
                }
            }
            
            printf("[!] Invalid candidate ID. Try again.\n");
        }
    }
    
    // Confirm votes
    print_header("Confirm Your Votes");
    printf("============================================================\n");
    printf("REVIEW YOUR SELECTIONS\n");
    printf("============================================================\n");
    
    for (int i = 0; i < pos_count; i++) {
        if (votes[i] != 0) {
            // Find candidate name
            char cmd[CMD_BUF_LEN];
            snprintf(cmd, CMD_BUF_LEN, "LIST_CANDS %d", pos_ids[i]);
            client_send_recv(cmd, response, sizeof(response));
            
            if (!is_error(response)) {
                char buf[CMD_BUF_LEN];
                strncpy(buf, response, CMD_BUF_LEN - 1);
                buf[CMD_BUF_LEN - 1] = '\0';
                
                char *token = strtok(buf, " ");
                if (token) {
                    int cand_id = atoi(token);
                    if (cand_id == votes[i]) {
                        token = strtok(NULL, "");
                        if (token) {
                            char cand_name[MAX_NAME_LEN];
                            strncpy(cand_name, token, MAX_NAME_LEN - 1);
                            cand_name[MAX_NAME_LEN - 1] = '\0';
                            replace_underscores(cand_name);
                            printf("%-20s --> %s\n", pos_names[i], cand_name);
                        }
                    }
                }
                
                // Skip remaining lines
                while (1) {
                    if (nh_recv_line(g_sock, response, sizeof(response)) != SUCCESS) {
                        printf("\nLost connection to server. Exiting.\n");
                        nh_close(g_sock);
                        exit(1);
                    }
                    if (strcmp(response, "END") == 0) break;
                }
            }
        }
    }
    
    printf("============================================================\n");
    printf("Submit votes? (1=Yes / 0=No): ");
    
    char input[32];
    fgets(input, sizeof(input), stdin);
    int submit = atoi(input);
    
    if (submit != 1) {
        printf("Vote cancelled.\n");
        pause_screen();
        return;
    }
    
    // Submit votes
    for (int i = 0; i < pos_count; i++) {
        if (votes[i] != 0) {
            char cmd[CMD_BUF_LEN];
            snprintf(cmd, CMD_BUF_LEN, "CAST_VOTE %d %d", pos_ids[i], votes[i]);
            client_send_recv(cmd, response, sizeof(response));
            
            if (strcmp(response, "OK") != 0) {
                print_error(response);
            }
        }
    }
    
    // Logout
    client_send_recv("LOGOUT", response, sizeof(response));
    g_user_id = -1;
    g_is_admin = 0;
    
    printf("============================================================\n");
    printf("Your votes have been recorded. Thank you!\n");
    printf("============================================================\n");
    pause_screen();
}

void voter_menu(void) {
    while (1) {
        print_header("Voter Menu");
        printf("Welcome, %s\n\n", g_user_name);
        printf("1. Cast Vote\n");
        printf("2. Apply for Candidacy\n");
        printf("3. Check Election Status\n");
        printf("0. Logout\n\n");
        printf("Choice: ");
        
        char input[32];
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input);
        
        switch (choice) {
            case 1:
                voter_handle_vote();
                return; // Logout after voting
            case 2:
                handle_candidate_application();
                break;
            case 3: {
                char response[CMD_BUF_LEN];
                client_send_recv("STATUS", response, sizeof(response));
                printf("Election Status: %s\n", response);
                pause_screen();
                break;
            }
            case 0: {
                char response[CMD_BUF_LEN];
                client_send_recv("LOGOUT", response, sizeof(response));
                g_user_id = -1;
                g_is_admin = 0;
                return;
            }
            default:
                printf("Invalid choice.\n");
                pause_screen();
                break;
        }
    }
}

void admin_print_positions(void) {
    print_header("Positions");
    printf("ID  | Position Name\n");
    printf("----|------------------\n");
    client_send_recv("LIST_POSITIONS", "dummy", 1); // Dummy response
    client_recv_multiline();
}

void admin_handle_add_position(void) {
    print_header("Add Position");
    printf("Position name: ");
    
    char name[MAX_NAME_LEN];
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    replace_spaces(name);
    
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "ADD_POSITION %s", name);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        printf("Position added. ID: %s\n", response + 3);
    } else {
        print_error(response);
    }
    pause_screen();
}

void admin_handle_register_candidate(void) {
    print_header("Register Candidate");
    admin_print_positions();
    
    printf("\nCandidate name: ");
    char name[MAX_NAME_LEN];
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    replace_spaces(name);
    
    printf("Position ID: ");
    char input[32];
    fgets(input, sizeof(input), stdin);
    int pos_id = atoi(input);
    
    // Verify position exists
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "LIST_CANDS %d", pos_id);
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (is_error(response)) {
        print_error(response);
        pause_screen();
        return;
    }
    
    // Clear the multiline response
    while (1) {
        if (nh_recv_line(g_sock, response, sizeof(response)) != SUCCESS) {
            printf("\nLost connection to server. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        if (strcmp(response, "END") == 0) break;
    }
    
    snprintf(cmd, CMD_BUF_LEN, "REGISTER_CAND %s %d", name, pos_id);
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        printf("Candidate registered. ID: %s\n", response + 3);
    } else {
        print_error(response);
    }
    pause_screen();
}

void admin_handle_register_voter(void) {
    print_header("Register Voter");
    printf("Voter full name: ");
    
    char name[MAX_NAME_LEN];
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    replace_spaces(name);
    
    printf("Password: ");
    char password[MAX_PASS_LEN];
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "REGISTER_VOTER %s %s", name, password);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        printf("Voter registered successfully.\n");
        printf("Voter ID: %s\n", response + 3);
        printf("Give this ID to the voter — they need it to log in.\n");
    } else {
        print_error(response);
    }
    pause_screen();
}

void admin_handle_results(void) {
    print_header("Election Results");
    
    // Send the command first
    if (nh_send_line(g_sock, "RESULTS") != SUCCESS) {
        printf("\nLost connection to server. Exiting.\n");
        nh_close(g_sock);
        exit(1);
    }
    
    char line[CMD_BUF_LEN];
    while (1) {
        if (nh_recv_line(g_sock, line, sizeof(line)) != SUCCESS) {
            printf("\nLost connection to server. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(line, "END") == 0) {
            break;
        }
        
        if (is_error(line)) {
            print_error(line);
            break;
        }
        
        // Add safety check to prevent buffer overflow
        if (strlen(line) >= CMD_BUF_LEN - 1) {
            printf("[!] Invalid data received from server.\n");
            break;
        }
        
        if (strncmp(line, "POSITION", 8) == 0) {
            printf("\n%s\n", line);
            printf("------------------------------------------------------------\n");
            printf("%-24s %6s %10s\n", "Candidate Name", "Votes", "Percentage");
            printf("------------------------------------------------------------\n");
        } else if (strncmp(line, "CANDIDATE", 9) == 0) {
            char buf[CMD_BUF_LEN];
            strncpy(buf, line, CMD_BUF_LEN - 1);
            buf[CMD_BUF_LEN - 1] = '\0';
            
            char *token = strtok(buf, " ");
            token = strtok(NULL, " "); // Skip "CANDIDATE"
            if (token) {
                char cand_name[MAX_NAME_LEN];
                strncpy(cand_name, token, MAX_NAME_LEN - 1);
                cand_name[MAX_NAME_LEN - 1] = '\0';
                replace_underscores(cand_name);
                
                token = strtok(NULL, " "); // votes
                int votes = 0;
                if (token) votes = atoi(token);
                
                token = strtok(NULL, " "); // percentage
                float percentage = 0.0f;
                if (token) percentage = atof(token);
                
                printf("%-24s %6d %9.2f%%\n", cand_name, votes, percentage);
            }
        } else if (strncmp(line, "WINNER", 6) == 0) {
            printf("------------------------------------------------------------\n");
            printf("Winner: %s\n", line + 7);
            printf("------------------------------------------------------------\n");
        } else if (strncmp(line, "TURNOUT", 7) == 0) {
            printf("\nVoter Turnout: %s%%\n", line + 8);
        }
    }
    
    pause_screen();
}

void admin_manage_applications(void) {
    while (1) {
        print_header("Manage Candidate Applications");
        
        // Get pending applications
        client_send_recv("LIST_APPLICATIONS", "dummy", 1);
        
        char line[CMD_BUF_LEN];
        int app_ids[MAX_CANDIDATES];
        char voter_names[MAX_CANDIDATES][MAX_NAME_LEN];
        char position_names[MAX_CANDIDATES][MAX_NAME_LEN];
        int pos_ids[MAX_CANDIDATES];
        int app_count = 0;
        
        // Read applications
        if (nh_recv_line(g_sock, line, sizeof(line)) == SUCCESS) {
            if (!is_error(line)) {
                // Parse first application
                char buf[CMD_BUF_LEN];
                strncpy(buf, line, CMD_BUF_LEN - 1);
                buf[CMD_BUF_LEN - 1] = '\0';
                
                char *token = strtok(buf, " ");
                if (token) {
                    app_ids[app_count] = atoi(token);
                    token = strtok(NULL, " ");
                    if (token) {
                        strncpy(voter_names[app_count], token, MAX_NAME_LEN - 1);
                        voter_names[app_count][MAX_NAME_LEN - 1] = '\0';
                        replace_underscores(voter_names[app_count]);
                        
                        token = strtok(NULL, " ");
                        if (token) {
                            pos_ids[app_count] = atoi(token);
                            
                            token = strtok(NULL, "");
                            if (token) {
                                strncpy(position_names[app_count], token, MAX_NAME_LEN - 1);
                                position_names[app_count][MAX_NAME_LEN - 1] = '\0';
                                replace_underscores(position_names[app_count]);
                                app_count++;
                            }
                        }
                    }
                }
                
                // Read remaining applications
                while (app_count < MAX_CANDIDATES) {
                    if (nh_recv_line(g_sock, line, sizeof(line)) != SUCCESS) break;
                    if (strcmp(line, "END") == 0) break;
                    
                    strncpy(buf, line, CMD_BUF_LEN - 1);
                    buf[CMD_BUF_LEN - 1] = '\0';
                    
                    token = strtok(buf, " ");
                    if (token) {
                        app_ids[app_count] = atoi(token);
                        token = strtok(NULL, " ");
                        if (token) {
                            strncpy(voter_names[app_count], token, MAX_NAME_LEN - 1);
                            voter_names[app_count][MAX_NAME_LEN - 1] = '\0';
                            replace_underscores(voter_names[app_count]);
                            
                            token = strtok(NULL, " ");
                            if (token) {
                                pos_ids[app_count] = atoi(token);
                                
                                token = strtok(NULL, "");
                                if (token) {
                                    strncpy(position_names[app_count], token, MAX_NAME_LEN - 1);
                                    position_names[app_count][MAX_NAME_LEN - 1] = '\0';
                                    replace_underscores(position_names[app_count]);
                                    app_count++;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        if (app_count == 0) {
            printf("No pending candidate applications.\n");
            pause_screen();
            return;
        }
        
        // Display applications
        printf("Pending Applications:\n");
        printf("ID  | Applicant         | Position\n");
        printf("----|-------------------|------------------\n");
        for (int i = 0; i < app_count; i++) {
            printf("%-4d| %-17s| %s\n", app_ids[i], voter_names[i], position_names[i]);
        }
        printf("\n");
        printf("1. Approve Application\n");
        printf("2. Reject Application\n");
        printf("0. Back\n\n");
        printf("Choice: ");
        
        char input[32];
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input);
        
        if (choice == 1) {
            printf("Enter application ID to approve: ");
            fgets(input, sizeof(input), stdin);
            int app_id = atoi(input);
            
            char cmd[CMD_BUF_LEN];
            snprintf(cmd, CMD_BUF_LEN, "APPROVE_APPLICATION %d", app_id);
            
            char response[CMD_BUF_LEN];
            client_send_recv(cmd, response, sizeof(response));
            
            if (strcmp(response, "OK") == 0) {
                printf("Application approved! Candidate registered.\n");
            } else {
                print_error(response);
            }
            pause_screen();
        } else if (choice == 2) {
            printf("Enter application ID to reject: ");
            fgets(input, sizeof(input), stdin);
            int app_id = atoi(input);
            
            char cmd[CMD_BUF_LEN];
            snprintf(cmd, CMD_BUF_LEN, "REJECT_APPLICATION %d", app_id);
            
            char response[CMD_BUF_LEN];
            client_send_recv(cmd, response, sizeof(response));
            
            if (strcmp(response, "OK") == 0) {
                printf("Application rejected.\n");
            } else {
                print_error(response);
            }
            pause_screen();
        } else if (choice == 0) {
            break;
        } else {
            printf("Invalid choice.\n");
            pause_screen();
        }
    }
}

void admin_menu(void) {
    while (1) {
        print_header("Admin Panel");
        
        char status[CMD_BUF_LEN];
        client_send_recv("STATUS", status, sizeof(status));
        printf("Election Status: %s\n\n", status);
        
        printf("1. Manage Positions\n");
        printf("2. Manage Candidate Applications\n");
        printf("3. Register Candidate (Direct)\n");
        printf("4. Register Voter (Direct)\n");
        printf("5. Open Voting\n");
        printf("6. Close Voting\n");
        printf("7. View Results\n");
        printf("8. Reset System\n");
        printf("0. Logout\n\n");
        printf("Choice: ");
        
        char input[32];
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input);
        
        switch (choice) {
            case 1: {
                // Sub-menu for positions
                while (1) {
                    print_header("Manage Positions");
                    printf("1. Add Position\n");
                    printf("2. View Positions\n");
                    printf("0. Back\n\n");
                    printf("Choice: ");
                    
                    fgets(input, sizeof(input), stdin);
                    int sub_choice = atoi(input);
                    
                    if (sub_choice == 1) {
                        admin_handle_add_position();
                    } else if (sub_choice == 2) {
                        admin_print_positions();
                        pause_screen();
                    } else if (sub_choice == 0) {
                        break;
                    } else {
                        printf("Invalid choice.\n");
                        pause_screen();
                    }
                }
                break;
            }
            case 2:
                admin_manage_applications();
                break;
            case 3:
                admin_handle_register_candidate();
                break;
            case 4:
                admin_handle_register_voter();
                break;
            case 5: {
                char response[CMD_BUF_LEN];
                client_send_recv("OPEN_VOTING", response, sizeof(response));
                if (strcmp(response, "OK") == 0) {
                    printf("Voting is now OPEN.\n");
                } else {
                    print_error(response);
                }
                pause_screen();
                break;
            }
            case 6: {
                char response[CMD_BUF_LEN];
                client_send_recv("CLOSE_VOTING", response, sizeof(response));
                if (strcmp(response, "OK") == 0) {
                    printf("Voting is now CLOSED.\n");
                } else {
                    print_error(response);
                }
                pause_screen();
                break;
            }
            case 7:
                admin_handle_results();
                break;
            case 8:
                printf("Type CONFIRM to reset: ");
                char confirm[32];
                fgets(confirm, sizeof(confirm), stdin);
                confirm[strcspn(confirm, "\n")] = '\0';
                
                if (strcmp(confirm, "CONFIRM") == 0) {
                    char response[CMD_BUF_LEN];
                    client_send_recv("RESET", response, sizeof(response));
                    if (strcmp(response, "OK") == 0) {
                        printf("System reset successfully.\n");
                    } else {
                        print_error(response);
                    }
                } else {
                    printf("Reset cancelled.\n");
                }
                pause_screen();
                break;
            case 0: {
                char response[CMD_BUF_LEN];
                client_send_recv("LOGOUT", response, sizeof(response));
                g_is_admin = 0;
                return;
            }
            default:
                printf("Invalid choice.\n");
                pause_screen();
                break;
        }
    }
}

void handle_voter_registration(void) {
    print_header("Voter Registration");
    printf("Full Name: ");
    
    char name[MAX_NAME_LEN];
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    replace_spaces(name);
    
    printf("Password: ");
    char password[MAX_PASS_LEN];
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "SELF_REGISTER %s %s", name, password);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        printf("Registration successful!\n");
        printf("Your Voter ID is: %s\n", response + 3);
        printf("Please save this ID - you need it to login and vote.\n");
    } else {
        print_error(response);
    }
    pause_screen();
}

void handle_candidate_application(void) {
    print_header("Apply for Candidacy");
    
    // Get available positions
    if (nh_send_line(g_sock, "LIST_POSITIONS") != SUCCESS) {
        printf("\nLost connection to server. Exiting.\n");
        nh_close(g_sock);
        exit(1);
    }
    
    char line[CMD_BUF_LEN];
    int pos_ids[MAX_POSITIONS];
    char pos_names[MAX_POSITIONS][MAX_NAME_LEN];
    int pos_count = 0;
    
    // Read positions
    if (nh_recv_line(g_sock, line, sizeof(line)) == SUCCESS) {
        if (!is_error(line)) {
            // Parse first position
            char buf[CMD_BUF_LEN];
            strncpy(buf, line, CMD_BUF_LEN - 1);
            buf[CMD_BUF_LEN - 1] = '\0';
            
            int pos_id;
            char pos_name[MAX_NAME_LEN];
            if (sscanf(buf, "%d %s", &pos_id, pos_name) == 2) {
                pos_ids[pos_count] = pos_id;
                strncpy(pos_names[pos_count], pos_name, MAX_NAME_LEN - 1);
                pos_names[pos_count][MAX_NAME_LEN - 1] = '\0';
                replace_underscores(pos_names[pos_count]);
                pos_count++;
            }
            
            // Read remaining positions
            while (pos_count < MAX_POSITIONS) {
                if (nh_recv_line(g_sock, line, sizeof(line)) != SUCCESS) break;
                if (strcmp(line, "END") == 0) break;
                
                strncpy(buf, line, CMD_BUF_LEN - 1);
                buf[CMD_BUF_LEN - 1] = '\0';
                
                int pos_id;
                char pos_name[MAX_NAME_LEN];
                if (sscanf(buf, "%d %s", &pos_id, pos_name) == 2) {
                    pos_ids[pos_count] = pos_id;
                    strncpy(pos_names[pos_count], pos_name, MAX_NAME_LEN - 1);
                    pos_names[pos_count][MAX_NAME_LEN - 1] = '\0';
                    replace_underscores(pos_names[pos_count]);
                    pos_count++;
                }
            }
        }
    }
    
    if (pos_count == 0) {
        printf("[!] No positions available for candidacy.\n");
        pause_screen();
        return;
    }
    
    // Display positions
    printf("Available Positions:\n");
    printf("ID  | Position\n");
    printf("----|------------------\n");
    for (int i = 0; i < pos_count; i++) {
        printf("%-4d| %s\n", pos_ids[i], pos_names[i]);
    }
    printf("\n");
    
    // Get position choice
    int choice;
    while (1) {
        printf("Enter position ID to apply for: ");
        char input[32];
        fgets(input, sizeof(input), stdin);
        
        if (sscanf(input, "%d", &choice) == 1) {
            int valid = 0;
            for (int i = 0; i < pos_count; i++) {
                if (pos_ids[i] == choice) {
                    valid = 1;
                    break;
                }
            }
            
            if (valid) break;
        }
        
        printf("[!] Invalid position ID. Try again.\n");
    }
    
    // Submit application
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "APPLY_CANDIDATE %d", choice);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strcmp(response, "OK") == 0) {
        printf("Application submitted successfully!\n");
        printf("Your application will be reviewed by the admin.\n");
        printf("You can check the status later.\n");
    } else {
        print_error(response);
    }
    
    pause_screen();
}

void handle_voter_login(void) {
    print_header("Voter Login");
    printf("Voter ID: ");
    
    char input[32];
    fgets(input, sizeof(input), stdin);
    int voter_id = atoi(input);
    
    printf("Password: ");
    char password[MAX_PASS_LEN];
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "LOGIN %d %s", voter_id, password);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        strncpy(g_user_name, response + 3, MAX_NAME_LEN - 1);
        g_user_name[MAX_NAME_LEN - 1] = '\0';
        replace_underscores(g_user_name);
        g_user_id = voter_id;
        g_is_admin = 0;
        voter_menu();
    } else {
        print_error(response);
        pause_screen();
    }
}

void handle_admin_login(void) {
    print_header("Admin Login");
    printf("Username: ");
    
    char username[MAX_NAME_LEN];
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = '\0';
    
    printf("Password: ");
    char password[MAX_PASS_LEN];
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    char cmd[CMD_BUF_LEN];
    snprintf(cmd, CMD_BUF_LEN, "ADMIN_LOGIN %s %s", username, password);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strcmp(response, "OK") == 0) {
        g_is_admin = 1;
        g_user_id = -1;
        admin_menu();
    } else {
        print_error(response);
        pause_screen();
    }
}

void main_menu(void) {
    while (1) {
        print_header("Main Menu");
        printf("1. Voter Login\n");
        printf("2. Register as Voter\n");
        printf("3. Admin Login\n");
        printf("0. Exit\n\n");
        printf("Choice: ");
        
        char input[32];
        fgets(input, sizeof(input), stdin);
        int choice = atoi(input);
        
        switch (choice) {
            case 1:
                handle_voter_login();
                break;
            case 2:
                handle_voter_registration();
                break;
            case 3:
                handle_admin_login();
                break;
            case 0: {
                char response[CMD_BUF_LEN];
                client_send_recv("QUIT", response, sizeof(response));
                nh_close(g_sock);
                printf("Goodbye.\n");
                exit(0);
            }
            default:
                printf("Invalid choice.\n");
                pause_screen();
                break;
        }
    }
}

int main(void) {
    // Connect to server
    g_sock = nh_client_connect(SERVER_IP, SERVER_PORT);
    if (g_sock == ERR_CONN) {
        printf("Cannot connect to SONU server at %s:%d\n", SERVER_IP, SERVER_PORT);
        printf("Please ensure the server is running first.\n");
        return 1;
    }
    
    printf("Connected to SONU server.\n");
    
    main_menu();
    
    return 0;
}
