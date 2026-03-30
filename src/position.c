#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "position.h"

int pos_get_all(Position out[], int *count) {
    char lines[MAX_POSITIONS][MAX_LINE_LEN];
    if (fh_read_all(POSITIONS_FILE, lines, count) != SUCCESS) return ERR_FILE;
    
    for (int i = 0; i < *count; i++) {
        char copy[MAX_LINE_LEN];
        strncpy(copy, lines[i], MAX_LINE_LEN - 1);
        copy[MAX_LINE_LEN - 1] = '\0';
        char *tok = strtok(copy, DELIM);
        out[i].id = tok ? atoi(tok) : 0;
        tok = strtok(NULL, DELIM);
        strncpy(out[i].name, tok ? tok : "", MAX_NAME_LEN - 1);
        out[i].name[MAX_NAME_LEN - 1] = '\0';
    }
    return SUCCESS;
}

int pos_get_by_id(int id, Position *out) {
    Position positions[MAX_POSITIONS];
    int count;
    
    if (pos_get_all(positions, &count) != SUCCESS) return ERR_FILE;
    
    for (int i = 0; i < count; i++) {
        if (positions[i].id == id) {
            *out = positions[i];
            return SUCCESS;
        }
    }
    return ERR_NOT_FOUND;
}

int pos_validate_id(int id) {
    Position p;
    return pos_get_by_id(id, &p) == SUCCESS ? SUCCESS : ERR_NOT_FOUND;
}

int pos_next_id(void) {
    Position positions[MAX_POSITIONS];
    int count;
    
    if (pos_get_all(positions, &count) != SUCCESS) return 1;
    
    int max_id = 0;
    for (int i = 0; i < count; i++) {
        if (positions[i].id > max_id) {
            max_id = positions[i].id;
        }
    }
    return max_id + 1;
}

int pos_add(const char *name) {
    int id = pos_next_id();
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%s", id, name);
    return fh_append_record(POSITIONS_FILE, record);
}

int pos_delete(int id) {
    char id_str[20];
    snprintf(id_str, sizeof(id_str), "%d", id);
    return fh_delete_record(POSITIONS_FILE, 0, id_str);
}

void pos_display_all(void) {
    Position positions[MAX_POSITIONS];
    int count;
    
    if (pos_get_all(positions, &count) != SUCCESS) {
        printf("Error loading positions.\n");
        return;
    }
    
    printf("\n=== POSITIONS ===\n");
    printf("ID\tName\n");
    printf("------------------------\n");
    
    for (int i = 0; i < count; i++) {
        printf("%d\t%s\n", positions[i].id, positions[i].name);
    }
    printf("\nTotal: %d positions\n", count);
}
