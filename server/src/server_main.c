#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <mqueue.h>
#include "config.h"
#include "net_handler.h"
#include "file_handler.h"
#include "utils.h"
#include "auth.h"
#include "auth_session.h"
#include "admin.h"
#include "voter.h"
#include "position.h"
#include "candidate.h"
#include "voting.h"
#include "tally.h"
#include "application.h"

static int last_was_quit = 0;

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

void cmd_login(int sock, struct sockaddr_in *client_addr, char *args) {
    char *token = strtok(args, " ");
    if (!token) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int voter_id = atoi(token);
    char *password = strtok(NULL, " ");
    if (!password) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int result = auth_voter_login(voter_id, password);
    if (result == SUCCESS) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %s", auth_get_voter_name());
        nh_send_to(sock, response, client_addr);
    } else {
        nh_send_to(sock, result == ERR_NOT_FOUND ? "ERR_NOT_FOUND" : "ERR_AUTH_FAIL", client_addr);
    }
}

void cmd_admin_login(int sock, struct sockaddr_in *client_addr, char *args) {
    printf("[DEBUG] Admin login attempt from %s:%d\n", 
           inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
    printf("[DEBUG] Received args: '%s' (len=%zu)\n", args ? args : "(null)", args ? strlen(args) : 0);
    
    char *username = strtok(args, " ");
    if (!username) {
        printf("[DEBUG] No username found\n");
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *password = strtok(NULL, " ");
    if (!password) {
        printf("[DEBUG] No password found\n");
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    // Remove potential newline from password
    char *newline = strchr(password, '\n');
    if (newline) *newline = '\0';
    newline = strchr(password, '\r');
    if (newline) *newline = '\0';
    
    printf("[DEBUG] Username: '%s' (len=%zu)\n", username, strlen(username));
    printf("[DEBUG] Password: '%s' (len=%zu)\n", password, strlen(password));
    printf("[DEBUG] Expected: user='%s', pass='%s'\n", ADMIN_USER, ADMIN_PASS);
    
    printf("[DEBUG] Username match: %d\n", strcmp(username, ADMIN_USER) == 0);
    printf("[DEBUG] Password match: %d\n", strcmp(password, ADMIN_PASS) == 0);
    
    int result = session_auth_admin(client_addr, username, password);
    printf("[DEBUG] Auth result: %d\n", result);
    
    nh_send_to(sock, result == SUCCESS ? "OK" : "ERR_AUTH_FAIL", client_addr);
}

void cmd_logout(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    session_logout(client_addr);
    nh_send_to(sock, "OK", client_addr);
}

void cmd_status(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    char status[MAX_LINE_LEN];
    int result = admin_get_election_status(status, MAX_LINE_LEN);
    
    if (result == SUCCESS) {
        nh_send_to(sock, status, client_addr);
    } else {
        nh_send_to(sock, "ERR_FILE", client_addr);
    }
}

void cmd_register_voter(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *space = strchr(args, ' ');
    if (!space) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
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
        nh_send_to(sock, response, client_addr);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_to(sock, "ERR_DUPLICATE", client_addr);
                break;
            case ERR_FULL:
                nh_send_to(sock, "ERR_FULL", client_addr);
                break;
            default:
                nh_send_to(sock, "ERR_FILE", client_addr);
                break;
        }
    }
}

void cmd_add_position(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args || strlen(args) == 0) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    replace_underscores(args);
    
    int result = pos_add(args);
    if (result >= 0) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", result);
        nh_send_to(sock, response, client_addr);
    } else {
        nh_send_to(sock, result == ERR_DUPLICATE ? "ERR_DUPLICATE" : "ERR_FILE", client_addr);
    }
}

void cmd_list_positions(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    Position positions[MAX_POSITIONS];
    int count = pos_get_all(positions, MAX_POSITIONS);
    
    if (count == ERR_FILE || count == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        char name_copy[MAX_NAME_LEN];
        strncpy(name_copy, positions[i].name, MAX_NAME_LEN - 1);
        name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(name_copy);
        
        snprintf(line, CMD_BUF_LEN, "%d %s", positions[i].id, name_copy);
        nh_send_to(sock, line, client_addr);
    }
    
    nh_send_to(sock, "END", client_addr);
}

void cmd_register_cand(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *space = strrchr(args, ' ');
    if (!space) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
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
        nh_send_to(sock, response, client_addr);
    } else {
        switch (result) {
            case ERR_NOT_FOUND:
                nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
                break;
            case ERR_DUPLICATE:
                nh_send_to(sock, "ERR_DUPLICATE", client_addr);
                break;
            default:
                nh_send_to(sock, "ERR_FILE", client_addr);
                break;
        }
    }
}

void cmd_list_cands(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int position_id = atoi(args);
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_for_position(position_id, candidates, MAX_CANDIDATES);
    
    if (count == ERR_FILE || count == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        char name_copy[MAX_NAME_LEN];
        strncpy(name_copy, candidates[i].name, MAX_NAME_LEN - 1);
        name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(name_copy);
        
        snprintf(line, CMD_BUF_LEN, "%d %s", candidates[i].id, name_copy);
        nh_send_to(sock, line, client_addr);
    }
    
    nh_send_to(sock, "END", client_addr);
}

void cmd_open_voting(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int result = admin_open_voting();
    nh_send_to(sock, result == SUCCESS ? "OK" : "ERR_FILE", client_addr);
}

void cmd_close_voting(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int result = admin_close_voting();
    nh_send_to(sock, result == SUCCESS ? "OK" : "ERR_FILE", client_addr);
}

void cmd_cast_vote(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *position_id_str = strtok(args, " ");
    char *candidate_id_str = strtok(NULL, " ");
    
    if (!position_id_str || !candidate_id_str) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int position_id = atoi(position_id_str);
    int candidate_id = atoi(candidate_id_str);
    int voter_id = auth_get_voter_id();
    
    if (voter_id < 0) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int result = voting_cast_vote(voter_id, position_id, candidate_id);
    
    switch (result) {
        case SUCCESS:
            nh_send_to(sock, "OK", client_addr);
            break;
        case ERR_CLOSED:
            nh_send_to(sock, "ERR_CLOSED", client_addr);
            break;
        case ERR_VOTED:
            nh_send_to(sock, "ERR_VOTED", client_addr);
            break;
        case ERR_NOT_FOUND:
            nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
            break;
        default:
            nh_send_to(sock, "ERR_FILE", client_addr);
            break;
    }
}

void cmd_cast_all_votes(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int voter_id = auth_get_voter_id();
    if (voter_id < 0) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int success_count = 0;
    int error_count = 0;
    
    // Parse position:candidate pairs (format: " pos_id:cand_id pos_id:cand_id ...")
    char *token = strtok(args, " ");
    while (token != NULL) {
        char *colon = strchr(token, ':');
        if (colon) {
            *colon = '\0';
            int position_id = atoi(token);
            int candidate_id = atoi(colon + 1);
            
            int result = voting_cast_vote(voter_id, position_id, candidate_id);
            if (result == SUCCESS) {
                success_count++;
            } else {
                error_count++;
            }
        }
        token = strtok(NULL, " ");
    }
    
    if (success_count > 0 && error_count == 0) {
        nh_send_to(sock, "OK", client_addr);
    } else if (success_count > 0) {
        nh_send_to(sock, "OK PARTIAL", client_addr);
    } else {
        nh_send_to(sock, "ERR_FILE", client_addr);
    }
}

void cmd_results(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    // Status is always "OPEN" now - allow results anytime
    printf("[DEBUG] Admin requested results - status is always OPEN\n");
    
    TallyResult results[MAX_CANDIDATES];
    int count = 0;
    
    int result = tally_compute(results, &count);
    if (result != SUCCESS || count == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    Position positions[MAX_POSITIONS];
    int pos_count = pos_get_all(positions, MAX_POSITIONS);
    
    for (int p = 0; p < pos_count; p++) {
        char line[CMD_BUF_LEN];
        char pos_name_copy[MAX_NAME_LEN];
        strncpy(pos_name_copy, positions[p].name, MAX_NAME_LEN - 1);
        pos_name_copy[MAX_NAME_LEN - 1] = '\0';
        replace_spaces(pos_name_copy);
        
        snprintf(line, CMD_BUF_LEN, "POSITION %s", pos_name_copy);
        nh_send_to(sock, line, client_addr);
        
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
                nh_send_to(sock, line, client_addr);
                
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
            nh_send_to(sock, line, client_addr);
        }
    }
    
    float turnout = tally_voter_turnout();
    char line[CMD_BUF_LEN];
    snprintf(line, CMD_BUF_LEN, "TURNOUT %.2f", turnout);
    nh_send_to(sock, line, client_addr);
    
    nh_send_to(sock, "END", client_addr);
}

void cmd_reset(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int result = admin_reset_direct();
    nh_send_to(sock, result == SUCCESS ? "OK" : "ERR_FILE", client_addr);
}

void cmd_quit(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    nh_send_to(sock, "OK", client_addr);
    last_was_quit = 1;
}

void cmd_self_register(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *name = strtok(args, " ");
    char *password = strtok(NULL, " ");
    
    if (!name || !password) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    replace_underscores(name);
    
    int voter_id = voter_next_id();
    if (voter_id == ERR_FILE) {
        nh_send_to(sock, "ERR_FILE", client_addr);
        return;
    }
    
    int result = voter_register(name, password);
    if (result == SUCCESS) {
        char response[CMD_BUF_LEN];
        snprintf(response, CMD_BUF_LEN, "OK %d", voter_id);
        nh_send_to(sock, response, client_addr);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_to(sock, "ERR_DUPLICATE", client_addr);
                break;
            case ERR_FULL:
                nh_send_to(sock, "ERR_FULL", client_addr);
                break;
            default:
                nh_send_to(sock, "ERR_UNKNOWN", client_addr);
                break;
        }
    }
}

void cmd_apply_candidate(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    if (!auth_is_logged_in()) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    char *pos_id_str = strtok(args, " ");
    if (!pos_id_str) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int position_id = atoi(pos_id_str);
    
    Position pos;
    if (pos_get_by_id(position_id, &pos) != SUCCESS) {
        nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
        return;
    }
    
    Voter voter;
    if (voter_get_by_id(auth_get_voter_id(), &voter) != SUCCESS) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    CandidateApplication existing;
    if (application_get_by_voter(voter.id, &existing) == SUCCESS) {
        nh_send_to(sock, "ERR_DUPLICATE", client_addr);
        return;
    }
    
    int result = application_add(voter.id, voter.name, position_id, pos.name);
    if (result == SUCCESS) {
        nh_send_to(sock, "OK", client_addr);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_to(sock, "ERR_DUPLICATE", client_addr);
                break;
            default:
                nh_send_to(sock, "ERR_UNKNOWN", client_addr);
                break;
        }
    }
}

void cmd_list_applications(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    CandidateApplication apps[MAX_CANDIDATES];
    int count = application_get_pending(apps, MAX_CANDIDATES);
    
    if (count == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        char line[CMD_BUF_LEN];
        snprintf(line, CMD_BUF_LEN, "%d %s %d %s", 
                 apps[i].id, apps[i].voter_name, apps[i].position_id, apps[i].position_name);
        nh_send_to(sock, line, client_addr);
    }
    
    nh_send_to(sock, "END", client_addr);
}

void cmd_approve_application(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int app_id = atoi(args);
    
    CandidateApplication apps[MAX_CANDIDATES];
    int count = application_get_all(apps, MAX_CANDIDATES);
    
    CandidateApplication *target_app = NULL;
    for (int i = 0; i < count; i++) {
        if (apps[i].id == app_id && apps[i].status == 0) {
            target_app = &apps[i];
            break;
        }
    }
    
    if (!target_app) {
        nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
        return;
    }
    
    int result = cand_register(target_app->voter_name, target_app->position_id);
    if (result == SUCCESS) {
        application_update_status(app_id, 1);
        nh_send_to(sock, "OK", client_addr);
    } else {
        switch (result) {
            case ERR_DUPLICATE:
                nh_send_to(sock, "ERR_DUPLICATE", client_addr);
                break;
            case ERR_FULL:
                nh_send_to(sock, "ERR_FULL", client_addr);
                break;
            default:
                nh_send_to(sock, "ERR_UNKNOWN", client_addr);
                break;
        }
    }
}

void cmd_reject_application(int sock, struct sockaddr_in *client_addr, char *args) {
    if (!args) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    if (!session_is_admin(client_addr)) {
        nh_send_to(sock, "ERR_AUTH_FAIL", client_addr);
        return;
    }
    
    int app_id = atoi(args);
    
    int result = application_update_status(app_id, 2);
    if (result == SUCCESS) {
        nh_send_to(sock, "OK", client_addr);
    } else {
        nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
    }
}

void dispatch_command(int sock, struct sockaddr_in *client_addr, char *cmd_buf) {
    printf("[DEBUG] Received command from %s:%d: '%s'\n", 
           inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), cmd_buf);
    
    char cmd_copy[CMD_BUF_LEN];
    strncpy(cmd_copy, cmd_buf, CMD_BUF_LEN - 1);
    cmd_copy[CMD_BUF_LEN - 1] = '\0';
    
    char *verb = strtok(cmd_copy, " ");
    if (!verb) {
        printf("[DEBUG] No verb found in command\n");
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *args = NULL;
    if (strlen(verb) < strlen(cmd_buf)) {
        args = cmd_buf + strlen(verb) + 1;
    }
    
    printf("[DEBUG] Verb: '%s', Args: '%s'\n", verb, args ? args : "(none)");
    
    last_was_quit = 0;
    
    if (strcmp(verb, "LOGIN") == 0) {
        cmd_login(sock, client_addr, args);
    } else if (strcmp(verb, "ADMIN_LOGIN") == 0) {
        cmd_admin_login(sock, client_addr, args);
    } else if (strcmp(verb, "LOGOUT") == 0) {
        cmd_logout(sock, client_addr, args);
    } else if (strcmp(verb, "STATUS") == 0) {
        cmd_status(sock, client_addr, args);
    } else if (strcmp(verb, "SELF_REGISTER") == 0) {
        cmd_self_register(sock, client_addr, args);
    } else if (strcmp(verb, "APPLY_CANDIDATE") == 0) {
        cmd_apply_candidate(sock, client_addr, args);
    } else if (strcmp(verb, "LIST_APPLICATIONS") == 0) {
        cmd_list_applications(sock, client_addr, args);
    } else if (strcmp(verb, "APPROVE_APPLICATION") == 0) {
        cmd_approve_application(sock, client_addr, args);
    } else if (strcmp(verb, "REJECT_APPLICATION") == 0) {
        cmd_reject_application(sock, client_addr, args);
    } else if (strcmp(verb, "ADD_POSITION") == 0) {
        cmd_add_position(sock, client_addr, args);
    } else if (strcmp(verb, "LIST_POSITIONS") == 0) {
        cmd_list_positions(sock, client_addr, args);
    } else if (strcmp(verb, "REGISTER_CAND") == 0) {
        cmd_register_cand(sock, client_addr, args);
    } else if (strcmp(verb, "LIST_CANDS") == 0) {
        cmd_list_cands(sock, client_addr, args);
    } else if (strcmp(verb, "OPEN_VOTING") == 0) {
        cmd_open_voting(sock, client_addr, args);
    } else if (strcmp(verb, "CLOSE_VOTING") == 0) {
        cmd_close_voting(sock, client_addr, args);
    } else if (strcmp(verb, "CAST_VOTE") == 0) {
        cmd_cast_vote(sock, client_addr, args);
    } else if (strcmp(verb, "CAST_ALL_VOTES") == 0) {
        cmd_cast_all_votes(sock, client_addr, args);
    } else if (strcmp(verb, "RESULTS") == 0) {
        cmd_results(sock, client_addr, args);
    } else if (strcmp(verb, "RESET") == 0) {
        cmd_reset(sock, client_addr, args);
    } else if (strcmp(verb, "QUIT") == 0) {
        cmd_quit(sock, client_addr, args);
    } else {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
    }
    
    printf("CMD [%s] from %s:%d\n", verb,
           inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
}

// Message struct for master-slave communication via POSIX message queue
typedef struct {
    char cmd_buf[CMD_BUF_LEN];
    struct sockaddr_in client_addr;
} DgramMsg;

// SIGCHLD handler to prevent zombie slave processes
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(void) {
    // Initialize session management
    session_init();
    
    int result = fh_init_files();
    if (result != SUCCESS) {
        printf("Failed to initialize files. Exiting.\n");
        return 1;
    }
    
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start UDP server. Exiting.\n");
        return 1;
    }
    
    printf("SONU Voting UDP Server ready. Waiting for datagrams...\n");

    // Install SIGCHLD handler to prevent zombie slave processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);

    // Open POSIX message queue for master-slave communication
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAX_MSG;
    attr.mq_msgsize = sizeof(DgramMsg);
    attr.mq_curmsgs = 0;
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    // Main loop - master process receives datagrams and forks slaves
    while (1) {
        DgramMsg msg;

        // MASTER receives the datagram — command text and sender address
        int r = nh_recv_from(server_fd, msg.cmd_buf, CMD_BUF_LEN, &msg.client_addr);
        if (r == ERR_CONN) continue;

        // MASTER places the datagram into the message queue for the slave to pick up
        if (mq_send(mq, (char *)&msg, sizeof(DgramMsg), 0) == -1) {
            perror("mq_send");
            continue;
        }

        // MASTER forks a slave to handle this datagram
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // SLAVE PROCESS
            // Slave pulls the datagram from the queue
            DgramMsg slave_msg;
            if (mq_receive(mq, (char *)&slave_msg, sizeof(DgramMsg), NULL) == -1) {
                perror("mq_receive");
                exit(1);
            }

            // Slave processes the voting command — login, cast_vote, results, etc.
            // It uses the client address from the message to send the reply back
            last_was_quit = 0;
            dispatch_command(server_fd, &slave_msg.client_addr, slave_msg.cmd_buf);

            // Slave exits after handling exactly one datagram
            mq_close(mq);
            exit(0);
        }

        // MASTER loops back immediately to receive the next datagram
    }

    // Cleanup (unreachable in infinite loop but good practice)
    mq_close(mq);
    mq_unlink(MQ_NAME);

    nh_close(server_fd);
    return 0;
}
