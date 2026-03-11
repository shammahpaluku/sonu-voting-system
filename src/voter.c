#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "voter.h"

int voter_get_all(Voter out[], int *count) {
    char lines[MAX_VOTERS][MAX_LINE_LEN];
    if (fh_read_all(VOTERS_FILE, lines, count) != SUCCESS) return ERR_FILE;
    
    for (int i = 0; i < *count; i++) {
        char copy[MAX_LINE_LEN];
        strncpy(copy, lines[i], MAX_LINE_LEN - 1);
        copy[MAX_LINE_LEN - 1] = '\0';
        char *tok = strtok(copy, DELIM);
        out[i].id = tok ? atoi(tok) : 0;
        tok = strtok(NULL, DELIM);
        strncpy(out[i].name, tok ? tok : "", MAX_NAME_LEN - 1);
        out[i].name[MAX_NAME_LEN - 1] = '\0';
        tok = strtok(NULL, DELIM);
        strncpy(out[i].password, tok ? tok : "", MAX_PASS_LEN - 1);
        out[i].password[MAX_PASS_LEN - 1] = '\0';
        tok = strtok(NULL, DELIM);
        out[i].has_voted = tok ? atoi(tok) : 0;
    }
    return SUCCESS;
}

int voter_get_by_id(int id, Voter *out) {
    Voter voters[MAX_VOTERS];
    int count;
    
    if (voter_get_all(voters, &count) != SUCCESS) return ERR_FILE;
    
    for (int i = 0; i < count; i++) {
        if (voters[i].id == id) {
            *out = voters[i];
            return SUCCESS;
        }
    }
    return ERR_NOT_FOUND;
}

int voter_validate_id(int id) {
    Voter v;
    return voter_get_by_id(id, &v) == SUCCESS ? SUCCESS : ERR_NOT_FOUND;
}

int voter_has_voted(int id) {
    Voter v;
    if (voter_get_by_id(id, &v) != SUCCESS) return 0;
    return v.has_voted;
}

int voter_next_id(void) {
    Voter voters[MAX_VOTERS];
    int count;
    
    if (voter_get_all(voters, &count) != SUCCESS) return 1;
    
    int max_id = 0;
    for (int i = 0; i < count; i++) {
        if (voters[i].id > max_id) {
            max_id = voters[i].id;
        }
    }
    return max_id + 1;
}

int voter_register(const char *name, const char *plain_password) {
    int id = voter_next_id();
    char hashed[MAX_PASS_LEN];
    utils_hash_password(plain_password, hashed, MAX_PASS_LEN);
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s|%s|0", id, name, hashed);
    return fh_append_record(VOTERS_FILE, record);
}

int voter_mark_voted(int id) {
    Voter v;
    if (voter_get_by_id(id, &v) != SUCCESS) return ERR_NOT_FOUND;
    v.has_voted = 1;
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s|%s|%d", v.id, v.name, v.password, v.has_voted);
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%d", id);
    return fh_update_record(VOTERS_FILE, 0, id_str, record);
    /* 0 = id_field index, VoterID is first column (index 0) */
}

void voter_display_all(void) {
    Voter voters[MAX_VOTERS];
    int count;
    
    if (voter_get_all(voters, &count) != SUCCESS) {
        printf("Error loading voters.\n");
        return;
    }
    
    printf("\n=== VOTERS ===\n");
    printf("ID\tName\t\t\tStatus\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%d\t%-20s\t%s\n", 
               voters[i].id, voters[i].name,
               voters[i].has_voted ? "Voted" : "Not Voted");
    }
    printf("\nTotal: %d voters\n", count);
}
