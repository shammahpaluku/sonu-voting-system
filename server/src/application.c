#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "application.h"
#include "file_handler.h"
#include "utils.h"

int application_add(int voter_id, const char *voter_name, int position_id, const char *position_name) {
    // Check if voter already has an application
    CandidateApplication existing;
    if (application_get_by_voter(voter_id, &existing) == SUCCESS) {
        return ERR_DUPLICATE;
    }
    
    // Get next application ID
    int app_count = fh_count_records(APPLICATIONS_FILE);
    int new_id = app_count + 1;
    
    // Create application record
    char record[MAX_LINE_LEN];
    snprintf(record, MAX_LINE_LEN, "%d|%d|%s|%d|%s|0", 
             new_id, voter_id, voter_name, position_id, position_name);
    
    // Write to file
    if (fh_append_record(APPLICATIONS_FILE, record) != SUCCESS) {
        return ERR_FILE;
    }
    
    return SUCCESS;
}

int application_get_by_voter(int voter_id, CandidateApplication *app) {
    char line[MAX_LINE_LEN];
    FILE *file = fopen(APPLICATIONS_FILE, "r");
    if (!file) return ERR_NOT_FOUND;
    
    while (fgets(line, MAX_LINE_LEN, file)) {
        line[strcspn(line, "\n")] = '\0';
        
        char *token = strtok(line, DELIM);
        if (!token) continue;
        
        app->id = atoi(token);
        
        token = strtok(NULL, DELIM);
        if (!token) continue;
        
        int file_voter_id = atoi(token);
        if (file_voter_id == voter_id) {
            // Parse the rest of the record
            token = strtok(NULL, DELIM);
            strncpy(app->voter_name, token, MAX_NAME_LEN - 1);
            app->voter_name[MAX_NAME_LEN - 1] = '\0';
            
            token = strtok(NULL, DELIM);
            app->position_id = atoi(token);
            
            token = strtok(NULL, DELIM);
            strncpy(app->position_name, token, MAX_NAME_LEN - 1);
            app->position_name[MAX_NAME_LEN - 1] = '\0';
            
            token = strtok(NULL, DELIM);
            app->status = atoi(token);
            
            fclose(file);
            return SUCCESS;
        }
    }
    
    fclose(file);
    return ERR_NOT_FOUND;
}

int application_get_all(CandidateApplication *apps, int max_count) {
    char line[MAX_LINE_LEN];
    FILE *file = fopen(APPLICATIONS_FILE, "r");
    if (!file) return 0;
    
    int count = 0;
    while (fgets(line, MAX_LINE_LEN, file) && count < max_count) {
        line[strcspn(line, "\n")] = '\0';
        
        char *token = strtok(line, DELIM);
        if (!token) continue;
        
        apps[count].id = atoi(token);
        
        token = strtok(NULL, DELIM);
        apps[count].voter_id = atoi(token);
        
        token = strtok(NULL, DELIM);
        strncpy(apps[count].voter_name, token, MAX_NAME_LEN - 1);
        apps[count].voter_name[MAX_NAME_LEN - 1] = '\0';
        
        token = strtok(NULL, DELIM);
        apps[count].position_id = atoi(token);
        
        token = strtok(NULL, DELIM);
        strncpy(apps[count].position_name, token, MAX_NAME_LEN - 1);
        apps[count].position_name[MAX_NAME_LEN - 1] = '\0';
        
        token = strtok(NULL, DELIM);
        apps[count].status = atoi(token);
        
        count++;
    }
    
    fclose(file);
    return count;
}

int application_get_pending(CandidateApplication *apps, int max_count) {
    CandidateApplication all_apps[MAX_CANDIDATES];
    int total_count = application_get_all(all_apps, MAX_CANDIDATES);
    
    int pending_count = 0;
    for (int i = 0; i < total_count && pending_count < max_count; i++) {
        if (all_apps[i].status == 0) { // Pending
            apps[pending_count] = all_apps[i];
            pending_count++;
        }
    }
    
    return pending_count;
}

int application_update_status(int app_id, int status) {
    CandidateApplication apps[MAX_CANDIDATES];
    int count = application_get_all(apps, MAX_CANDIDATES);
    
    // Find and update the application
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (apps[i].id == app_id) {
            apps[i].status = status;
            found = 1;
            break;
        }
    }
    
    if (!found) return ERR_NOT_FOUND;
    
    // Clear the file and rewrite all applications
    char empty_lines[1][MAX_LINE_LEN] = {""};
    fh_write_all(APPLICATIONS_FILE, empty_lines, 0);
    
    for (int i = 0; i < count; i++) {
        char record[MAX_LINE_LEN];
        snprintf(record, MAX_LINE_LEN, "%d|%d|%s|%d|%s|%d",
                 apps[i].id, apps[i].voter_id, apps[i].voter_name,
                 apps[i].position_id, apps[i].position_name, apps[i].status);
        
        fh_append_record(APPLICATIONS_FILE, record);
    }
    
    return SUCCESS;
}

int application_delete(int app_id) {
    CandidateApplication apps[MAX_CANDIDATES];
    int count = application_get_all(apps, MAX_CANDIDATES);
    
    // Find and remove the application
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (apps[i].id == app_id) {
            // Shift remaining applications
            for (int j = i; j < count - 1; j++) {
                apps[j] = apps[j + 1];
            }
            found = 1;
            break;
        }
    }
    
    if (!found) return ERR_NOT_FOUND;
    
    // Clear the file and rewrite remaining applications
    char empty_lines[1][MAX_LINE_LEN] = {""};
    fh_write_all(APPLICATIONS_FILE, empty_lines, 0);
    
    for (int i = 0; i < count - 1; i++) {
        char record[MAX_LINE_LEN];
        snprintf(record, MAX_LINE_LEN, "%d|%d|%s|%d|%s|%d",
                 apps[i].id, apps[i].voter_id, apps[i].voter_name,
                 apps[i].position_id, apps[i].position_name, apps[i].status);
        
        fh_append_record(APPLICATIONS_FILE, record);
    }
    
    return SUCCESS;
}

void application_display(const CandidateApplication *app) {
    const char *status_str;
    switch (app->status) {
        case 0: status_str = "Pending"; break;
        case 1: status_str = "Approved"; break;
        case 2: status_str = "Rejected"; break;
        default: status_str = "Unknown"; break;
    }
    
    printf("ID: %d | Voter: %s (ID: %d) | Position: %s | Status: %s\n",
           app->id, app->voter_name, app->voter_id, app->position_name, status_str);
}
