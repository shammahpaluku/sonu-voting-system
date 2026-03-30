#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "position.h"
#include "candidate.h"

int cand_get_all(Candidate out[], int *count) {
    char lines[MAX_CANDIDATES][MAX_LINE_LEN];
    if (fh_read_all(CANDIDATES_FILE, lines, count) != SUCCESS) return ERR_FILE;
    
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
        out[i].position_id = tok ? atoi(tok) : 0;
    }
    return SUCCESS;
}

int cand_get_by_id(int id, Candidate *out) {
    Candidate candidates[MAX_CANDIDATES];
    int count;
    
    if (cand_get_all(candidates, &count) != SUCCESS) return ERR_FILE;
    
    for (int i = 0; i < count; i++) {
        if (candidates[i].id == id) {
            *out = candidates[i];
            return SUCCESS;
        }
    }
    return ERR_NOT_FOUND;
}

int cand_get_for_position(int position_id, Candidate out[], int *count) {
    Candidate all_candidates[MAX_CANDIDATES];
    int all_count;
    
    if (cand_get_all(all_candidates, &all_count) != SUCCESS) return ERR_FILE;
    
    *count = 0;
    for (int i = 0; i < all_count; i++) {
        if (all_candidates[i].position_id == position_id) {
            out[*count] = all_candidates[i];
            (*count)++;
        }
    }
    return SUCCESS;
}

int cand_validate_id(int id, int position_id) {
    Candidate c;
    if (cand_get_by_id(id, &c) != SUCCESS) return ERR_NOT_FOUND;
    return (c.position_id == position_id) ? SUCCESS : ERR_NOT_FOUND;
}

int cand_next_id(void) {
    Candidate candidates[MAX_CANDIDATES];
    int count;
    
    if (cand_get_all(candidates, &count) != SUCCESS) return 1;
    
    int max_id = 0;
    for (int i = 0; i < count; i++) {
        if (candidates[i].id > max_id) {
            max_id = candidates[i].id;
        }
    }
    return max_id + 1;
}

int cand_register(const char *name, int position_id) {
    if (pos_validate_id(position_id) != SUCCESS) return ERR_NOT_FOUND;
    
    int id = cand_next_id();
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s|%d", id, name, position_id);
    return fh_append_record(CANDIDATES_FILE, record);
}

int cand_delete(int id) {
    char id_str[20];
    snprintf(id_str, sizeof(id_str), "%d", id);
    return fh_delete_record(CANDIDATES_FILE, 0, id_str);
}

void cand_display_all(void) {
    Candidate candidates[MAX_CANDIDATES];
    int count;
    
    if (cand_get_all(candidates, &count) != SUCCESS) {
        printf("Error loading candidates.\n");
        return;
    }
    
    printf("\n=== CANDIDATES ===\n");
    printf("ID\tName\t\t\tPosition\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        Position pos;
        char pos_name[MAX_NAME_LEN] = "Unknown";
        if (pos_get_by_id(candidates[i].position_id, &pos) == SUCCESS) {
            strcpy(pos_name, pos.name);
        }
        printf("%d\t%-20s\t%d (%s)\n", 
               candidates[i].id, candidates[i].name, 
               candidates[i].position_id, pos_name);
    }
    printf("\nTotal: %d candidates\n", count);
}

void cand_display_for_position(int position_id) {
    Candidate candidates[MAX_CANDIDATES];
    int count;
    
    if (cand_get_for_position(position_id, candidates, &count) != SUCCESS) {
        printf("Error loading candidates.\n");
        return;
    }
    
    Position pos;
    char pos_name[MAX_NAME_LEN] = "Unknown";
    if (pos_get_by_id(position_id, &pos) == SUCCESS) {
        strcpy(pos_name, pos.name);
    }
    
    printf("\n=== CANDIDATES FOR: %s ===\n", pos_name);
    printf("ID\tName\n");
    printf("------------------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%d\t%s\n", candidates[i].id, candidates[i].name);
    }
    printf("\nTotal: %d candidates\n", count);
}
