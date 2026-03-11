#include <sys/stat.h>   /* for mkdir() */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "file_handler.h"

int fh_file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

int fh_count_records(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return ERR_FILE;

    int count = 0;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] != '\0' && line[0] != '\n')
            count++;
    }
    fclose(fp);
    return count;
}

int fh_read_all(const char *path, char lines[][MAX_LINE_LEN], int *count) {
    FILE *fp = fopen(path, "r");
    if (!fp) return ERR_FILE;
    *count = 0;
    
    while (fgets(lines[*count], MAX_LINE_LEN, fp) != NULL) {
        /* Strip trailing newline */
        lines[*count][strcspn(lines[*count], "\n")] = '\0';
        if (strlen(lines[*count]) == 0) continue;  /* skip blank lines */
        (*count)++;
    }
    fclose(fp);
    return SUCCESS;
}

int fh_write_all(const char *path, char lines[][MAX_LINE_LEN], int count) {
    FILE *fp = fopen(path, "w");
    if (!fp) return ERR_FILE;
    
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%s\n", lines[i]);
    }
    fclose(fp);
    return SUCCESS;
}

int fh_append_record(const char *path, const char *record) {
    FILE *fp = fopen(path, "a");
    if (!fp) return ERR_FILE;
    
    fprintf(fp, "%s\n", record);
    fclose(fp);
    return SUCCESS;
}

int fh_update_record(const char *path, int id_field, const char *id_val,
                      const char *new_record) {
    char lines[MAX_VOTERS][MAX_LINE_LEN];
    int count = 0;
    
    if (fh_read_all(path, lines, &count) != SUCCESS) return ERR_FILE;

    FILE *fp = fopen(path, "w");
    if (!fp) return ERR_FILE;

    for (int i = 0; i < count; i++) {
        char copy[MAX_LINE_LEN];
        strncpy(copy, lines[i], MAX_LINE_LEN - 1);
        copy[MAX_LINE_LEN - 1] = '\0';

        /* Advance strtok to the target field */
        char *tok = strtok(copy, DELIM);
        for (int f = 0; f < id_field && tok != NULL; f++)
            tok = strtok(NULL, DELIM);

        /* If this line matches, replace it */
        if (tok && strcmp(tok, id_val) == 0) {
            fprintf(fp, "%s\n", new_record);
        } else {
            fprintf(fp, "%s\n", lines[i]);
        }
    }
    fclose(fp);
    return SUCCESS;
}

int fh_delete_record(const char *path, int id_field, const char *id_val) {
    char lines[MAX_VOTERS][MAX_LINE_LEN];
    int count = 0;
    
    if (fh_read_all(path, lines, &count) != SUCCESS) return ERR_FILE;

    FILE *fp = fopen(path, "w");
    if (!fp) return ERR_FILE;

    for (int i = 0; i < count; i++) {
        char copy[MAX_LINE_LEN];
        strncpy(copy, lines[i], MAX_LINE_LEN - 1);
        copy[MAX_LINE_LEN - 1] = '\0';

        /* Advance strtok to the target field */
        char *tok = strtok(copy, DELIM);
        for (int f = 0; f < id_field && tok != NULL; f++)
            tok = strtok(NULL, DELIM);

        /* If this line matches, skip it (delete) */
        if (tok && strcmp(tok, id_val) == 0) continue;

        fprintf(fp, "%s\n", lines[i]);
    }
    fclose(fp);
    return SUCCESS;
}

int fh_init_files(void) {
    /* Create data/ directory if missing (0755 = rwxr-xr-x) */
    mkdir(DATA_DIR, 0755);

    const char *files[] = {
        VOTERS_FILE, CANDIDATES_FILE, POSITIONS_FILE,
        VOTES_FILE, STATUS_FILE, NULL
    };
    
    for (int i = 0; files[i] != NULL; i++) {
        if (!fh_file_exists(files[i])) {
            FILE *fp = fopen(files[i], "w");
            if (!fp) return ERR_FILE;
            /* Write PENDING to election_status.txt on first creation */
            if (strcmp(files[i], STATUS_FILE) == 0)
                fprintf(fp, "%s\n", STATUS_PENDING);
            fclose(fp);
        }
    }
    return SUCCESS;
}
