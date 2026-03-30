#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/config.h"
#include "../include/file_handler.h"
#include "../include/auth.h"
#include "../include/voter.h"
#include "../include/candidate.h"
#include "../include/position.h"
#include "../include/voting.h"
#include "../include/tally.h"

void clear_screen(void);
void show_menu(void);
void voter_login(void);
void voter_registration(void);
void admin_login(void);
void view_results(void);
void view_status(void);

int main(int argc, char *argv[]) {
    /* Initialize system */
    if (fh_init_files() != SUCCESS) {
        printf("Fatal Error: Could not initialize data files\n");
        return 1;
    }
    
    while (1) {
        clear_screen();
        show_menu();
        
        int choice;
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                voter_login();
                break;
            case 2:
                voter_registration();
                break;
            case 3:
                admin_login();
                break;
            case 4:
                view_results();
                break;
            case 5:
                view_status();
                break;
            case 6:
                printf("Thank you for using SONU Voting System. Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
                printf("Press Enter to continue...");
                while (getchar() != '\n');
                getchar();
        }
    }
    
    return 0;
}

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void show_menu(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    SONU VOTING SYSTEM                          ║\n");
    printf("║                     Console Mode                               ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║  1. Voter Login                                               ║\n");
    printf("║  2. Voter Registration                                        ║\n");
    printf("║  3. Admin Login                                               ║\n");
    printf("║  4. View Results                                             ║\n");
    printf("║  5. View Status                                              ║\n");
    printf("║  6. Exit                                                     ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\nEnter your choice (1-6): ");
}

void voter_login(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                        VOTER LOGIN                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    int voter_id;
    char password[20];
    
    printf("\nEnter your Voter ID: ");
    if (scanf("%d", &voter_id) != 1) {
        printf("Invalid ID format.\n");
        printf("Press Enter to continue...");
        while (getchar() != '\n');
        getchar();
        return;
    }
    
    printf("Enter your password: ");
    while (getchar() != '\n');
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    if (auth_voter_login(voter_id, password) == SUCCESS) {
        printf("\n✅ Login successful!\n");
        printf("Press Enter to continue...");
        getchar();
        /* TODO: Implement voting interface */
    } else {
        printf("\n❌ Invalid credentials. Please try again.\n");
        printf("Press Enter to continue...");
        getchar();
    }
}

void voter_registration(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    VOTER REGISTRATION                          ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    char name[100];
    char password[20];
    
    printf("\nEnter your full name: ");
    while (getchar() != '\n');
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    
    printf("Enter password (10-20 chars): ");
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    if (strlen(password) < 10) {
        printf("Password must be at least 10 characters long.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    if (voter_register(name, password) == SUCCESS) {
        printf("\n✅ Registration successful!\n");
        printf("Please save your Voter ID for future login.\n");
    } else {
        printf("\n❌ Registration failed. Please try again.\n");
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}

void admin_login(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                        ADMIN LOGIN                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    char password[20];
    printf("\nEnter admin password: ");
    while (getchar() != '\n');
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    if (strcmp(password, "admin123") == 0) {
        printf("\n✅ Admin login successful!\n");
        printf("Press Enter to continue...");
        getchar();
        /* TODO: Implement admin interface */
    } else {
        printf("\n❌ Invalid admin password.\n");
        printf("Press Enter to continue...");
        getchar();
    }
}

void view_results(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ELECTION RESULTS                           ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    
    if (pos_get_all(positions, &pos_count) != SUCCESS || pos_count == 0) {
        printf("\n❌ No positions available.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    for (int i = 0; i < pos_count; i++) {
        printf("\nPosition: %s\n", positions[i].name);
        printf("─────────────────────────────────────────────────────────────\n");
        
        Candidate candidates[MAX_CANDIDATES];
        int cand_count = 0;
        cand_get_for_position(positions[i].id, candidates, &cand_count);
        
        if (cand_count == 0) {
            printf("No candidates registered for this position.\n");
            continue;
        }
        
        for (int j = 0; j < cand_count; j++) {
            int vote_count = tally_get_votes_for(candidates[j].id, positions[i].id);
            printf("• %s: %d votes\n", candidates[j].name, vote_count);
        }
    }
    
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

void view_status(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                     SYSTEM STATUS                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    FILE *fp = fopen(STATUS_FILE, "r");
    char status[20] = "Unknown";
    if (fp) {
        if (fgets(status, sizeof(status), fp)) {
            status[strcspn(status, "\n")] = '\0';
        }
        fclose(fp);
    }
    
    int total_voters = fh_count_records(VOTERS_FILE);
    int total_votes = fh_count_records(VOTES_FILE);
    
    printf("\nElection Status: %s\n", status);
    printf("Total Voters: %d\n", total_voters);
    printf("Total Votes Cast: %d\n", total_votes);
    printf("Participation Rate: %.1f%%\n", total_voters > 0 ? (float)total_votes / total_voters * 100 : 0);
    
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    getchar();
}
