#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include "config.h"
#include "file_handler.h"
#include "utils.h"
#include "auth.h"
#include "position.h"
#include "voter.h"
#include "admin.h"
#include "gui.h"

void admin_manage_positions(void) {
    /* GUI position management is handled by on_add_position_clicked callback */
    /* This function is kept for console compatibility */
}

void admin_open_voting(void) {
    FILE *fp = fopen(STATUS_FILE, "w");
    if (!fp) { 
        printf("Error opening status file.\n"); 
        return; 
    }
    fprintf(fp, "%s\n", STATUS_OPEN);
    fclose(fp);
    gui_show_message("Success", "Voting is now OPEN");
}

void admin_close_voting(void) {
    FILE *fp = fopen(STATUS_FILE, "w");
    if (!fp) { 
        printf("Error opening status file.\n"); 
        return; 
    }
    fprintf(fp, "%s\n", STATUS_CLOSED);
    fclose(fp);
    gui_show_message("Success", "Voting is now CLOSED");
}

int admin_get_election_status(char *out_status) {
    char lines[1][MAX_LINE_LEN];
    int count = 0;
    if (fh_read_all(STATUS_FILE, lines, &count) != SUCCESS) return ERR_FILE;
    if (count == 0) return ERR_FILE;
    
    strncpy(out_status, lines[0], MAX_LINE_LEN - 1);
    out_status[MAX_LINE_LEN - 1] = '\0';
    return SUCCESS;
}

void admin_view_status(void) {
    char status[MAX_LINE_LEN];
    if (admin_get_election_status(status) != SUCCESS) {
        gui_show_error("Error", "Error reading election status");
        return;
    }
    
    char status_text[200];
    snprintf(status_text, sizeof(status_text), 
             "=== ELECTION STATUS ===\n"
             "Current Status: %s\n"
             "Total Voters: %d\n"
             "Total Votes Cast: %d",
             status, fh_count_records(VOTERS_FILE), fh_count_records(VOTES_FILE));
    
    gui_show_message("Election Status", status_text);
}

void admin_reset_system(void) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(admin_window),
                                           GTK_DIALOG_MODAL,
                                           GTK_MESSAGE_WARNING,
                                           "Cancel", GTK_RESPONSE_CANCEL,
                                           "Reset", GTK_RESPONSE_ACCEPT,
                                           NULL);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
        "This will permanently delete all voter and vote data!\n"
        "This action cannot be undone.");
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dialog),
        "<b>RESET SYSTEM</b>");
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        /* Truncate files */
        FILE *fp;
        fp = fopen(VOTERS_FILE, "w"); if (fp) fclose(fp);
        fp = fopen(VOTES_FILE, "w"); if (fp) fclose(fp);
        
        /* Reset status to PENDING */
        fp = fopen(STATUS_FILE, "w");
        if (fp) {
            fprintf(fp, "%s\n", STATUS_PENDING);
            fclose(fp);
        }
        
        gui_show_message("Success", "System has been reset successfully");
    } else {
        gui_show_message("Cancelled", "System reset cancelled");
    }
    
    gtk_widget_destroy(dialog);
}

void admin_menu(void) {
    /* GUI admin menu is handled by gui.c */
    /* This function is kept for console compatibility */
}
