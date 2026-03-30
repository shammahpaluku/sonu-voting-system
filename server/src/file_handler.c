#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "config.h"
#include "file_handler.h"

int fh_init_files(void) {
    // Create data directory
    if (mkdir("data", 0755) == -1 && errno != EEXIST) {
        return ERR_FILE;
    }
    
    // List of data files to initialize
    const char *files[] = {VOTERS_FILE, CANDIDATES_FILE, POSITIONS_FILE, VOTES_FILE, STATUS_FILE, APPLICATIONS_FILE};
    const int num_files = 6;
    
    for (int i = 0; i < num_files; i++) {
        if (!fh_file_exists(files[i])) {
            FILE *fp = fopen(files[i], "w");
            if (!fp) {
                return ERR_FILE;
            }
            
            // Initialize election status with PENDING
            if (strcmp(files[i], STATUS_FILE) == 0) {
                fprintf(fp, "PENDING\n");
            }
            
            fclose(fp);
        }
    }
    
    return SUCCESS;
}

int fh_file_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0) ? 1 : 0;
}

int fh_count_records(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return ERR_FILE;
    }
    
    char line[MAX_LINE_LEN];
    int count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        // Check if line has at least one non-whitespace character
        int has_content = 0;
        for (int i = 0; line[i] != '\0' && line[i] != '\n'; i++) {
            if (line[i] != ' ' && line[i] != '\t') {
                has_content = 1;
                break;
            }
        }
        if (has_content) {
            count++;
        }
    }
    
    fclose(fp);
    return count;
}

int fh_read_all(const char *path, char lines[][MAX_LINE_LEN], int max_lines) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return ERR_FILE;
    }
    
    int count = 0;
    char line[MAX_LINE_LEN];
    
    while (count < max_lines && fgets(line, sizeof(line), fp)) {
        // Strip trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        strcpy(lines[count], line);
        count++;
    }
    
    fclose(fp);
    return count;
}

int fh_write_all(const char *path, char lines[][MAX_LINE_LEN], int count) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        return ERR_FILE;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s\n", lines[i]);
    }
    
    fclose(fp);
    return SUCCESS;
}

int fh_append_record(const char *path, const char *record) {
    FILE *fp = fopen(path, "a");
    if (!fp) {
        return ERR_FILE;
    }
    
    fprintf(fp, "%s\n", record);
    fclose(fp);
    return SUCCESS;
}

int fh_update_record(const char *path, int id_field, const char *id_val, const char *new_record) {
    char lines[MAX_VOTES][MAX_LINE_LEN]; // Use MAX_VOTES as maximum possible lines
    int line_count = fh_read_all(path, lines, MAX_VOTES);
    
    if (line_count == ERR_FILE) {
        return ERR_FILE;
    }
    
    int found = 0;
    
    for (int i = 0; i < line_count; i++) {
        char line_copy[MAX_LINE_LEN];
        strcpy(line_copy, lines[i]);
        
        char *token = strtok(line_copy, DELIM);
        int field = 0;
        
        while (token != NULL && field <= id_field) {
            if (field == id_field && strcmp(token, id_val) == 0) {
                strcpy(lines[i], new_record);
                found = 1;
                break;
            }
            token = strtok(NULL, DELIM);
            field++;
        }
    }
    
    if (!found) {
        return ERR_NOT_FOUND;
    }
    
    return fh_write_all(path, lines, line_count);
}

int fh_delete_record(const char *path, int id_field, const char *id_val) {
    char lines[MAX_VOTES][MAX_LINE_LEN]; // Use MAX_VOTES as maximum possible lines
    char new_lines[MAX_VOTES][MAX_LINE_LEN];
    int line_count = fh_read_all(path, lines, MAX_VOTES);
    
    if (line_count == ERR_FILE) {
        return ERR_FILE;
    }
    
    int new_count = 0;
    int found = 0;
    
    for (int i = 0; i < line_count; i++) {
        char line_copy[MAX_LINE_LEN];
        strcpy(line_copy, lines[i]);
        
        char *token = strtok(line_copy, DELIM);
        int field = 0;
        int should_delete = 0;
        
        while (token != NULL && field <= id_field) {
            if (field == id_field && strcmp(token, id_val) == 0) {
                should_delete = 1;
                found = 1;
                break;
            }
            token = strtok(NULL, DELIM);
            field++;
        }
        
        if (!should_delete) {
            strcpy(new_lines[new_count], lines[i]);
            new_count++;
        }
    }
    
    if (!found) {
        return ERR_NOT_FOUND;
    }
    
    return fh_write_all(path, new_lines, new_count);
}
