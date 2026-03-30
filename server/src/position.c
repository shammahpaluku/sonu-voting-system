#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "position.h"

int pos_next_id(void) {
    int count = fh_count_records(POSITIONS_FILE);
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    return count + 1;
}

int pos_add(const char *name) {
    // Validate name is not empty and contains only alpha/space chars
    if (!name || strlen(name) == 0 || !utils_is_alpha(name)) {
        return ERR_FILE;
    }
    
    // Check for duplicate name (case-insensitive)
    Position positions[MAX_POSITIONS];
    int count = pos_get_all(positions, MAX_POSITIONS);
    
    if (count != ERR_FILE) {
        for (int i = 0; i < count; i++) {
            char pos_name_lower[MAX_NAME_LEN];
            char name_lower[MAX_NAME_LEN];
            
            strncpy(pos_name_lower, positions[i].name, MAX_NAME_LEN - 1);
            pos_name_lower[MAX_NAME_LEN - 1] = '\0';
            utils_to_lowercase(pos_name_lower);
            
            strncpy(name_lower, name, MAX_NAME_LEN - 1);
            name_lower[MAX_NAME_LEN - 1] = '\0';
            utils_to_lowercase(name_lower);
            
            if (strcmp(pos_name_lower, name_lower) == 0) {
                return ERR_DUPLICATE;
            }
        }
    }
    
    // Get new id
    int new_id = pos_next_id();
    if (new_id == ERR_FILE) {
        return ERR_FILE;
    }
    
    // Build record string: "id|name"
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s", new_id, name);
    
    // Append record
    int result = fh_append_record(POSITIONS_FILE, record);
    if (result == SUCCESS) {
        return new_id;
    }
    return result;
}

int pos_get_all(Position *positions, int max) {
    char lines[MAX_POSITIONS][MAX_LINE_LEN];
    int line_count = fh_read_all(POSITIONS_FILE, lines, MAX_POSITIONS);
    
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
        
        positions[count].id = atoi(token);
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        strncpy(positions[count].name, token, MAX_NAME_LEN - 1);
        positions[count].name[MAX_NAME_LEN - 1] = '\0';
        
        count++;
    }
    
    return count;
}

int pos_get_by_id(int id, Position *out) {
    Position positions[MAX_POSITIONS];
    int count = pos_get_all(positions, MAX_POSITIONS);
    
    if (count == ERR_FILE) {
        return ERR_FILE;
    }
    
    for (int i = 0; i < count; i++) {
        if (positions[i].id == id) {
            *out = positions[i];
            return SUCCESS;
        }
    }
    
    return ERR_NOT_FOUND;
}

int pos_validate_id(int id) {
    Position pos;
    return pos_get_by_id(id, &pos);
}

void pos_display_all(void) {
    Position positions[MAX_POSITIONS];
    int count = pos_get_all(positions, MAX_POSITIONS);
    
    if (count == ERR_FILE || count == 0) {
        printf("No positions found.\n");
        return;
    }
    
    printf("ID  | Position Name\n");
    printf("----|------------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%-4d| %s\n", positions[i].id, positions[i].name);
    }
}
