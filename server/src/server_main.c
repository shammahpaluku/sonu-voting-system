#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "config.h"
#include "net_handler.h"
#include "file_handler.h"
#include "utils.h"
#include "auth.h"
#include "admin.h"
#include "voter.h"
#include "position.h"
#include "candidate.h"
#include "voting.h"
#include "tally.h"
#include "application.h"

// Command handler functions
void cmd_login(int client_fd, char *args);
void cmd_admin_login(int client_fd, char *args);
void cmd_logout(int client_fd, char *args);
void cmd_status(int client_fd, char *args);
void cmd_self_register(int client_fd, char *args);
void cmd_apply_candidate(int client_fd, char *args);
void cmd_list_applications(int client_fd, char *args);
void cmd_approve_application(int client_fd, char *args);
void cmd_reject_application(int client_fd, char *args);
void cmd_add_position(int client_fd, char *args);
void cmd_list_positions(int client_fd, char *args);
void cmd_register_cand(int client_fd, char *args);
void cmd_list_cands(int client_fd, char *args);
void cmd_open_voting(int client_fd, char *args);
void cmd_close_voting(int client_fd, char *args);
void cmd_cast_vote(int client_fd, char *args);
void cmd_results(int client_fd, char *args);
void cmd_reset(int client_fd, char *args);
void cmd_quit(int client_fd, char *args);

void dispatch_command(int client_fd, char *cmd_buf);
void handle_session(int client_fd);

static int last_was_quit = 0;

// Helper function to replace underscores with spaces
void replace_underscores(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == '_') {
            str[i] = ' ';
        }
    }
}

// Helper function to replace spaces with underscores for wire format
void replace_spaces(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') {
            str[i] = '_';
        }
    }
}

void cmd_login(int client_fd, char *args) {
    char *token = strtok(args, " ");
    if (!token) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int voter_id = atoi(token);
    char *password = strtok(NULL, " ");
    if (!password) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int result = auth_voter_login(voter_id, password);
    if (result == SUCCESS) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %s", auth_get_voter_name());
        nh_send_line(client_fd, response);
    } else {
        nh_send_line(client_fd, result == ERR_NOT_FOUND ? "ERR_NOT_FOUND" : "ERR_AUTH_FAIL");
    }
}

void cmd_admin_login(int client_fd, char *args) {
    char *username = strtok(args, " ");
    if (!username) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *password = strtok(NULL, " ");
    if (!password) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int result = auth_admin_login(username, password);
    nh_send_line(client_fd, result == SUCCESS ? "OK" : "ERR_AUTH_FAIL");
}

void cmd_logout(int client_fd, char *args) {
    (void)args; // Unused
    auth_logout();
    nh_send_line(client_fd, "OK");
}

void cmd_status(int client_fd, char *args) {
    (void)args; // Unused
    char status[MAX_LINE_LEN];
    int result = admin_get_election_status(status, MAX_LINE_LEN);
    
    if (result == SUCCESS) {
        nh_send_line(client_fd, status);
    } else {
        nh_send_line(client_fd, "ERR_FILE");
    }
}

void cmd_register_voter(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    // Find the space between name and password
    char *space = strchr(args, ' ');
    if (!space) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    *space = '\0';
    char *name = args;
    char *password = space + 1;
    
    replace_underscores(name);
    
    int result = voter_register(name, password);
    if (result >= 0) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", result);
        nh_send_line(client_fd, response);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_line(client_fd, "ERR_DUPLICATE");
                break;
            case ERR_FULL:
                nh_send_line(client_fd, "ERR_FULL");
                break;
            default:
                nh_send_line(client_fd, "ERR_FILE");
                break;
        }
    }
}

void cmd_add_position(int client_fd, char *args) {
    if (!args || strlen(args) == 0) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    replace_underscores(args);
    
    int result = pos_add(args);
    if (result >= 0) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", result);
        nh_send_line(client_fd, response);
    } else {
        nh_send_line(client_fd, result == ERR_DUPLICATE ? "ERR_DUPLICATE" : "ERR_FILE");
    }
}

void cmd_list_positions(int client_fd, char *args) {
    (void)args; // Unused
    Position positions[MAX_POSITIONS];
    int count = pos_get_all(positions, MAX_POSITIONS);
    
    if (count == ERR_FILE || count == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        char name_copy[MAX_NAME_LEN];
        strncpy(name_copy, positions[i].name, MAX_NAME_LEN - 1);
        name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(name_copy);
        
        snprintf(line, CMD_BUF_LEN, "%d %s", positions[i].id, name_copy);
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}

void cmd_register_cand(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    // Find the space between name and position_id
    char *space = strrchr(args, ' '); // Find last space for position_id
    if (!space) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    *space = '\0';
    char *name = args;
    char *position_id_str = space + 1;
    
    replace_underscores(name);
    int position_id = atoi(position_id_str);
    
    int result = cand_register(name, position_id);
    if (result >= 0) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", result);
        nh_send_line(client_fd, response);
    } else {
        switch (result) {
            case ERR_NOT_FOUND:
                nh_send_line(client_fd, "ERR_NOT_FOUND");
                break;
            case ERR_DUPLICATE:
                nh_send_line(client_fd, "ERR_DUPLICATE");
                break;
            default:
                nh_send_line(client_fd, "ERR_FILE");
                break;
        }
    }
}

void cmd_list_cands(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int position_id = atoi(args);
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_for_position(position_id, candidates, MAX_CANDIDATES);
    
    if (count == ERR_FILE || count == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        char name_copy[MAX_NAME_LEN];
        strncpy(name_copy, candidates[i].name, MAX_NAME_LEN - 1);
        name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(name_copy);
        
        snprintf(line, CMD_BUF_LEN, "%d %s", candidates[i].id, name_copy);
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}

void cmd_open_voting(int client_fd, char *args) {
    (void)args; // Unused
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    int result = admin_open_voting();
    nh_send_line(client_fd, result == SUCCESS ? "OK" : "ERR_FILE");
}

void cmd_close_voting(int client_fd, char *args) {
    (void)args; // Unused
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    int result = admin_close_voting();
    nh_send_line(client_fd, result == SUCCESS ? "OK" : "ERR_FILE");
}

void cmd_cast_vote(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *position_id_str = strtok(args, " ");
    char *candidate_id_str = strtok(NULL, " ");
    
    if (!position_id_str || !candidate_id_str) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int position_id = atoi(position_id_str);
    int candidate_id = atoi(candidate_id_str);
    int voter_id = auth_get_voter_id();
    
    if (voter_id < 0) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    int result = voting_cast_vote(voter_id, position_id, candidate_id);
    
    switch (result) {
        case SUCCESS:
            nh_send_line(client_fd, "OK");
            break;
        case ERR_CLOSED:
            nh_send_line(client_fd, "ERR_CLOSED");
            break;
        case ERR_VOTED:
            nh_send_line(client_fd, "ERR_VOTED");
            break;
        case ERR_NOT_FOUND:
            nh_send_line(client_fd, "ERR_NOT_FOUND");
            break;
        default:
            nh_send_line(client_fd, "ERR_FILE");
            break;
    }
}

void cmd_results(int client_fd, char *args) {
    (void)args; // Unused
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    char status[MAX_LINE_LEN];
    admin_get_election_status(status, MAX_LINE_LEN);
    if (strcmp(status, "CLOSED") != 0) {
        nh_send_line(client_fd, "ERR_CLOSED");
        return;
    }
    
    TallyResult results[MAX_CANDIDATES];
    int count = 0;
    
    int result = tally_compute(results, &count);
    if (result != SUCCESS || count == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    // Group by position
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    for (int p = 0; p < pos_count; p++) {
        char line[CMD_BUF_LEN];
        char pos_name_copy[MAX_NAME_LEN];
        strncpy(pos_name_copy, positions[p].name, MAX_NAME_LEN - 1);
        pos_name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(pos_name_copy);
        
        snprintf(line, CMD_BUF_LEN, "POSITION %s", pos_name_copy);
        nh_send_line(client_fd, line);
        
        TallyResult winner;
        winner.vote_count = -1;
        
        for (int i = 0; i < count; i++) {
            if (results[i].position_id == positions[p].id) {
                char cand_name_copy[MAX_NAME_LEN];
                strncpy(cand_name_copy, results[i].candidate_name, MAX_NAME_LEN - 1);
                cand_name_copy[MAX_NAME_LEN - 1] = '\0';
                replace_spaces(cand_name_copy);
                
                snprintf(line, CMD_BUF_LEN, "CANDIDATE %s %d %.2f", 
                        cand_name_copy, results[i].vote_count, results[i].percentage);
                nh_send_line(client_fd, line);
                
                if (results[i].vote_count > winner.vote_count) {
                    winner = results[i];
                }
            }
        }
        
        if (winner.vote_count >= 0) {
            char winner_name_copy[MAX_NAME_LEN];
            strncpy(winner_name_copy, winner.candidate_name, MAX_NAME_LEN - 1);
            winner_name_copy[MAX_NAME_LEN - 1] = '\0';
            replace_spaces(winner_name_copy);
            
            snprintf(line, CMD_BUF_LEN, "WINNER %s %d", winner_name_copy, winner.vote_count);
            nh_send_line(client_fd, line);
        }
    }
    
    // Send turnout
    float turnout = tally_voter_turnout();
    char line[CMD_BUF_LEN];
    snprintf(line, CMD_BUF_LEN, "TURNOUT %.2f", turnout);
    nh_send_line(client_fd, line);
    
    nh_send_line(client_fd, "END");
}

void cmd_reset(int client_fd, char *args) {
    (void)args; // Unused
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    int result = admin_reset_direct();
    nh_send_line(client_fd, result == SUCCESS ? "OK" : "ERR_FILE");
}

void cmd_quit(int client_fd, char *args) {
    (void)args; // Unused
    nh_send_line(client_fd, "OK");
    last_was_quit = 1;
}

void cmd_self_register(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *name = strtok(args, " ");
    char *password = strtok(NULL, " ");
    
    if (!name || !password) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    replace_underscores(name);
    
    int voter_id = voter_next_id();
    if (voter_id == ERR_FILE) {
        nh_send_line(client_fd, "ERR_FILE");
        return;
    }
    
    int result = voter_register(name, password);
    if (result == SUCCESS) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", voter_id);
        nh_send_line(client_fd, response);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_line(client_fd, "ERR_DUPLICATE");
                break;
            case ERR_FULL:
                nh_send_line(client_fd, "ERR_FULL");
                break;
            default:
                nh_send_line(client_fd, "ERR_UNKNOWN");
                break;
        }
    }
}

void cmd_apply_candidate(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    if (!auth_is_logged_in()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    char *pos_id_str = strtok(args, " ");
    if (!pos_id_str) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int position_id = atoi(pos_id_str);
    
    // Get position details
    Position pos;
    if (pos_get_by_id(position_id, &pos) != SUCCESS) {
        nh_send_line(client_fd, "ERR_NOT_FOUND");
        return;
    }
    
    // Get current voter info
    Voter voter;
    if (voter_get_by_id(auth_get_voter_id(), &voter) != SUCCESS) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    // Check if voter already applied
    CandidateApplication existing;
    if (application_get_by_voter(voter.id, &existing) == SUCCESS) {
        nh_send_line(client_fd, "ERR_DUPLICATE");
        return;
    }
    
    // Create application
    int result = application_add(voter.id, voter.name, position_id, pos.name);
    if (result == SUCCESS) {
        nh_send_line(client_fd, "OK");
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_line(client_fd, "ERR_DUPLICATE");
                break;
            default:
                nh_send_line(client_fd, "ERR_UNKNOWN");
                break;
        }
    }
}

void cmd_list_applications(int client_fd, char *args) {
    (void)args; // Unused
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    CandidateApplication apps[MAX_CANDIDATES];
    int count = application_get_pending(apps, MAX_CANDIDATES);
    
    if (count == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        snprintf(line, CMD_BUF_LEN, "%d %s %d %s", 
                 apps[i].id, apps[i].voter_name, apps[i].position_id, apps[i].position_name);
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}

void cmd_approve_application(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    int app_id = atoi(args);
    
    // Get application details
    CandidateApplication apps[MAX_CANDIDATES];
    int count = application_get_all(apps, MAX_CANDIDATES);
    
    CandidateApplication *target_app = NULL;
    for (int i = 0; i < count; i++) {
        if (apps[i].id == app_id && apps[i].status == 0) { // Pending
            target_app = &apps[i];
            break;
        }
    }
    
    if (!target_app) {
        nh_send_line(client_fd, "ERR_NOT_FOUND");
        return;
    }
    
    // Register as candidate
    int result = cand_register(target_app->voter_name, target_app->position_id);
    if (result == SUCCESS) {
        // Update application status
        application_update_status(app_id, 1); // Approved
        nh_send_line(client_fd, "OK");
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_line(client_fd, "ERR_DUPLICATE");
                break;
            case ERR_FULL:
                nh_send_line(client_fd, "ERR_FULL");
                break;
            default:
                nh_send_line(client_fd, "ERR_UNKNOWN");
                break;
        }
    }
}

void cmd_reject_application(int client_fd, char *args) {
    if (!args) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    if (!auth_is_admin()) {
        nh_send_line(client_fd, "ERR_AUTH_FAIL");
        return;
    }
    
    int app_id = atoi(args);
    
    int result = application_update_status(app_id, 2); // Rejected
    if (result == SUCCESS) {
        nh_send_line(client_fd, "OK");
    } else {
        nh_send_line(client_fd, "ERR_NOT_FOUND");
    }
}

void dispatch_command(int client_fd, char *cmd_buf) {
    char cmd_copy[CMD_BUF_LEN];
    strncpy(cmd_copy, cmd_buf, CMD_BUF_LEN - 1);
    cmd_copy[CMD_BUF_LEN - 1] = '\0';
    
    char *verb = strtok(cmd_copy, " ");
    if (!verb) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *args = NULL;
    if (strlen(verb) < strlen(cmd_buf)) {
        args = cmd_buf + strlen(verb) + 1;
    }
    
    last_was_quit = 0;
    
    if (strcmp(verb, "LOGIN") == 0) {
        cmd_login(client_fd, args);
    } else if (strcmp(verb, "ADMIN_LOGIN") == 0) {
        cmd_admin_login(client_fd, args);
    } else if (strcmp(verb, "LOGOUT") == 0) {
        cmd_logout(client_fd, args);
    } else if (strcmp(verb, "STATUS") == 0) {
        cmd_status(client_fd, args);
    } else if (strcmp(verb, "SELF_REGISTER") == 0) {
        cmd_self_register(client_fd, args);
    } else if (strcmp(verb, "APPLY_CANDIDATE") == 0) {
        cmd_apply_candidate(client_fd, args);
    } else if (strcmp(verb, "LIST_APPLICATIONS") == 0) {
        cmd_list_applications(client_fd, args);
    } else if (strcmp(verb, "APPROVE_APPLICATION") == 0) {
        cmd_approve_application(client_fd, args);
    } else if (strcmp(verb, "REJECT_APPLICATION") == 0) {
        cmd_reject_application(client_fd, args);
    } else if (strcmp(verb, "ADD_POSITION") == 0) {
        cmd_add_position(client_fd, args);
    } else if (strcmp(verb, "LIST_POSITIONS") == 0) {
        cmd_list_positions(client_fd, args);
    } else if (strcmp(verb, "REGISTER_CAND") == 0) {
        cmd_register_cand(client_fd, args);
    } else if (strcmp(verb, "LIST_CANDS") == 0) {
        cmd_list_cands(client_fd, args);
    } else if (strcmp(verb, "OPEN_VOTING") == 0) {
        cmd_open_voting(client_fd, args);
    } else if (strcmp(verb, "CLOSE_VOTING") == 0) {
        cmd_close_voting(client_fd, args);
    } else if (strcmp(verb, "CAST_VOTE") == 0) {
        cmd_cast_vote(client_fd, args);
    } else if (strcmp(verb, "RESULTS") == 0) {
        cmd_results(client_fd, args);
    } else if (strcmp(verb, "RESET") == 0) {
        cmd_reset(client_fd, args);
    } else if (strcmp(verb, "QUIT") == 0) {
        cmd_quit(client_fd, args);
    } else {
        nh_send_line(client_fd, "ERR_UNKNOWN");
    }
}

void handle_session(int client_fd) {
    // Reset auth state at start of each session
    auth_logout();
    
    char cmd_buf[CMD_BUF_LEN];
    
    while (1) {
        int result = nh_recv_line(client_fd, cmd_buf, CMD_BUF_LEN);
        if (result == ERR_CONN) {
            printf("Client disconnected.\n");
            break;
        }
        
        printf("CMD: %s\n", cmd_buf);
        dispatch_command(client_fd, cmd_buf);
        
        if (last_was_quit) {
            break;
        }
    }
    
    nh_close(client_fd);
    printf("Session ended.\n");
}

// SIGCHLD handler to prevent zombie slave processes
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void) {
    // Initialize files
    int result = fh_init_files();
    if (result != SUCCESS) {
        printf("Failed to initialize files. Exiting.\n");
        return 1;
    }
    
    // Initialize server
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start server. Exiting.\n");
        return 1;
    }
    
    printf("SONU Voting Server ready. Waiting for connections...\n");
    
    // Install SIGCHLD handler to prevent zombie slave processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);
    
    // Main server loop - fork-based concurrent server
    while (1) {
        int client_fd = nh_server_accept(server_fd);
        if (client_fd == ERR_CONN) {
            continue; // Skip this iteration
        }

        pid_t pid = fork();

        if (pid < 0) {
            // fork failed — close this client and try again
            perror("fork");
            nh_close(client_fd);
            continue;
        }

        if (pid == 0) {
            // SLAVE PROCESS
            // The slave does not need the listening socket — close it
            close(server_fd);
            // Handle the client session fully — auth, commands, voting logic, file saves
            handle_session(client_fd);
            // Slave exits when the session ends
            exit(0);
        }

        // MASTER PROCESS
        // Master does not own this client connection — slave does. Close master's copy.
        nh_close(client_fd);
        // Master loops back immediately to accept the next voter or admin
    }
    
    // This line is unreachable but required by C standard
    return 0;
}
