#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "position.h"
#include "candidate.h"

int cand_next_id(void) {
    int count = fh_count_records(CANDIDATES_FILE);
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    return count + 1;
}

int cand_register(const char *name, int position_id) {
    // Validate name is not empty
    if (!name || strlen(name) == 0) {
        return ERR_FILE;
    }
    
    // Validate position exists
    if (pos_validate_id(position_id) != SUCCESS) {
        return ERR_NOT_FOUND;
    }
    
    // Check for duplicate name in that position
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_for_position(position_id, candidates, MAX_CANDIDATES);
    
    if (count != ERR_FILE) {
        for (int i = 0; i < count; i++) {
            char cand_name_lower[MAX_NAME_LEN];
            char name_lower[MAX_NAME_LEN];
            
            strncpy(cand_name_lower, candidates[i].name, MAX_NAME_LEN - 1);
            cand_name_lower[MAX_NAME_LEN - 1] = '\0';
            utils_to_lowercase(cand_name_lower);
            
            strncpy(name_lower, name, MAX_NAME_LEN - 1);
            name_lower[MAX_NAME_LEN - 1] = '\0';
            utils_to_lowercase(name_lower);
            
            if (strcmp(cand_name_lower, name_lower) == 0) {
                return ERR_DUPLICATE;
            }
        }
    }
    
    // Get new id
    int new_id = cand_next_id();
    if (new_id == ERR_FILE) {
        return ERR_FILE;
    }
    
    // Build record: "id|name|position_id"
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s|%d", new_id, name, position_id);
    
    // Append record
    int result = fh_append_record(CANDIDATES_FILE, record);
    if (result == SUCCESS) {
        return new_id;
    }
    return result;
}

int cand_get_all(Candidate *candidates, int max) {
    char lines[MAX_CANDIDATES][MAX_LINE_LEN];
    int line_count = fh_read_all(CANDIDATES_FILE, lines, MAX_CANDIDATES);
    
    if (line_count == ERR_FILE) {
        return ERR_FILE;
    }
    
    int count = 0;
    for (int i = 0; i < line_count && count < max; i++) {
        char buf[MAX_LINE_LEN];
        strncpy(buf, lines[i], MAX_LINE_LEN - 1);
        buf[MAX_LINE_LEN - 1] = '\0';
        
        char *token = strtok(buf, DELIM);
        if (!token) continue;
        
        candidates[count].id = atoi(token);
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        strncpy(candidates[count].name, token, MAX_NAME_LEN - 1);
        candidates[count].name[MAX_NAME_LEN - 1] = '\0';
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        candidates[count].position_id = atoi(token);
        
        count++;
    }
    
    return count;
}

int cand_get_by_id(int id, Candidate *out) {
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_all(candidates, MAX_CANDIDATES);
    
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    
    for (int i = 0; i < count; i++) {
        if (candidates[i].id == id) {
            *out = candidates[i];
            return SUCCESS;
        }
    }
    
    return ERR_NOT_FOUND;
}

int cand_get_for_position(int position_id, Candidate *out, int max) {
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_all(candidates, MAX_CANDIDATES);
    
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    
    int matching_count = 0;
    for (int i = 0; i < count && matching_count < max; i++) {
        if (candidates[i].position_id == position_id) {
            out[matching_count] = candidates[i];
            matching_count++;
        }
    }
    
    return matching_count;
}

int cand_validate_id(int candidate_id, int position_id) {
    Candidate cand;
    int result = cand_get_by_id(candidate_id, &cand);
    
    if (result == SUCCESS && cand.position_id == position_id) {
        return SUCCESS;
    }
    
    return ERR_NOT_FOUND;
}

void cand_display_all(void) {
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_all(candidates, MAX_CANDIDATES);
    
    if (count == ERR_FILE || count == 0) {
        printf("No candidates registered.\n");
        return;
    }
    
    printf("ID  | Candidate Name        | Position ID\n");
    printf("----|----------------------|------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%-4d| %-22s| %d\n", candidates[i].id, candidates[i].name, candidates[i].position_id);
    }
}

void cand_display_for_position(int position_id) {
    // Get position name
    Position pos;
    if (pos_get_by_id(position_id, &pos) != SUCCESS) {
        printf("Position not found.\n");
        return;
    }
    
    printf("Candidates for: %s\n", pos.name);
    printf("ID  | Candidate Name\n");
    printf("----|------------------\n");
    
    Candidate candidates[MAX_CANDIDATES];
    int count = cand_get_for_position(position_id, candidates, MAX_CANDIDATES);
    
    if (count == ERR_FILE || count == 0) {
        printf("No candidates registered for this position.\n");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        printf("%-4d| %s\n", candidates[i].id, candidates[i].name);
    }
}
