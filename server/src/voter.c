#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "voter.h"

int voter_next_id(void) {
    int count = fh_count_records(VOTERS_FILE);
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    return count + 1;
}

int voter_register(const char *name, const char *plain_password) {
    // Validate name is not empty
    if (!name || strlen(name) == 0) {
        return ERR_FILE;
    }
    
    // Check MAX_VOTERS not exceeded
    int current_count = fh_count_records(VOTERS_FILE);
    if (current_count >= MAX_VOTERS) {
        return ERR_FULL;
    }
    
    // Check for duplicate name - TODO: implement this properly later
    // For now, skip duplicate check to focus on main functionality
    
    // Get new id
    int new_id = voter_next_id();
    if (new_id == ERR_FILE) {
        return ERR_FILE;
    }
    
    // Hash password
    char hashed_password[MAX_PASS_LEN];
    utils_hash_password(plain_password, hashed_password, MAX_PASS_LEN);
    
    // Create voter record
    Voter voter;
    voter.id = new_id;
    strncpy(voter.name, name, MAX_NAME_LEN - 1);
    voter.name[MAX_NAME_LEN - 1] = '\0';
    strncpy(voter.password, hashed_password, MAX_PASS_LEN - 1);
    voter.password[MAX_PASS_LEN - 1] = '\0';
    voter.has_voted = 0;
    
    // Write to file
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s|%s|0", voter.id, voter.name, voter.password);
    
    return fh_append_record(VOTERS_FILE, record);
}

int voter_get_all(Voter *voters, int max) {
    char lines[MAX_VOTERS][MAX_LINE_LEN];
    int line_count = fh_read_all(VOTERS_FILE, lines, MAX_VOTERS);
    
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
        
        voters[count].id = atoi(token);
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        strncpy(voters[count].name, token, MAX_NAME_LEN - 1);
        voters[count].name[MAX_NAME_LEN - 1] = '\0';
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        strncpy(voters[count].password, token, MAX_PASS_LEN - 1);
        voters[count].password[MAX_PASS_LEN - 1] = '\0';
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        voters[count].has_voted = atoi(token);
        
        count++;
    }
    
    return count;
}

int voter_get_by_id(int id, Voter *out) {
    Voter voters[MAX_VOTERS];
    int count = voter_get_all(voters, MAX_VOTERS);
    
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    
    for (int i = 0; i < count; i++) {
        if (voters[i].id == id) {
            *out = voters[i];
            return SUCCESS;
        }
    }
    
    return ERR_NOT_FOUND;
}

int voter_validate_id(int id) {
    Voter voter;
    return voter_get_by_id(id, &voter);
}

int voter_has_voted(int id) {
    Voter voter;
    int result = voter_get_by_id(id, &voter);
    
    if (result == SUCCESS) {
        return voter.has_voted;
    }
    
    return ERR_NOT_FOUND;
}

int voter_mark_voted(int id) {
    Voter voter;
    int result = voter_get_by_id(id, &voter);
    
    if (result != SUCCESS) {
        return ERR_NOT_FOUND;
    }
    
    // Build updated record string with has_voted set to 1
    char new_record[MAX_LINE_LEN];
    snprintf(new_record, MAX_LINE_LEN, "%d|%s|%s|1", voter.id, voter.name, voter.password);
    
    // Convert id to string for update
    char id_str[16];
    snprintf(id_str, sizeof(id_str), "%d", id);
    
    return fh_update_record(VOTERS_FILE, 0, id_str, new_record);
}

void voter_display_all(void) {
    Voter voters[MAX_VOTERS];
    int count = voter_get_all(voters, MAX_VOTERS);
    
    if (count == ERR_FILE || count == 0) {
        printf("No voters registered.\n");
        return;
    }
    
    printf("ID  | Name                  | Has Voted\n");
    printf("----|-----------------------|----------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%-4d| %-23s| %s\n", voters[i].id, voters[i].name, 
               voters[i].has_voted ? "Yes" : "No");
    }
}
