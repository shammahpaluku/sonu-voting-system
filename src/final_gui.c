#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/config.h"
#include "../include/file_handler.h"
#include "../include/auth.h"
#include "../include/voter.h"
#include "../include/candidate.h"
#include "../include/position.h"
#include "../include/voting.h"
#include "../include/tally.h"

/* Function declarations */
void refresh_positions_list(void);
void setup_voting_interface(void);
void display_results(void);
void on_voter_self_register(void);
void on_register_voter(void);
void on_add_candidate(void);
void on_add_candidate_confirm(GtkWidget *button, gpointer user_data);
void on_remove_position(void);
void on_remove_candidate(void);

/* Terminal Mode Functions */
void show_startup_menu(void);
void run_terminal_mode(void);
void terminal_admin_menu(void);
void terminal_voter_menu(void);
void terminal_voter_register(void);
void terminal_voter_login(void);
void terminal_cast_vote(void);
void terminal_view_results(void);
void terminal_add_position(void);
void terminal_add_candidate(void);
void terminal_remove_position(void);
void terminal_remove_candidate(void);
void terminal_open_voting(void);
void terminal_close_voting(void);
void terminal_view_status(void);
void clear_screen(void);

/* Terminal styling functions */
void print_header(const char *title, const char *subtitle);
void print_footer(void);
void print_menu_item(int number, const char *icon, const char *title, const char *description);
void print_section_header(const char *title);
void print_success_message(const char *message);
void print_error_message(const char *message);
void print_warning_message(const char *message);
void print_info_message(const char *message);
void print_separator(void);
void reset_colors(void);

/* Color definitions */
#define COLOR_RESET     "\033[0m"
#define COLOR_BLACK     "\033[0;30m"
#define COLOR_RED       "\033[0;31m"
#define COLOR_GREEN     "\033[0;32m"
#define COLOR_YELLOW    "\033[0;33m"
#define COLOR_BLUE      "\033[0;34m"
#define COLOR_MAGENTA   "\033[0;35m"
#define COLOR_CYAN      "\033[0;36m"
#define COLOR_WHITE     "\033[0;37m"
#define COLOR_BRIGHT_BLACK   "\033[1;30m"
#define COLOR_BRIGHT_RED     "\033[1;31m"
#define COLOR_BRIGHT_GREEN   "\033[1;32m"
#define COLOR_BRIGHT_YELLOW  "\033[1;33m"
#define COLOR_BRIGHT_BLUE    "\033[1;34m"
#define COLOR_BRIGHT_MAGENTA "\033[1;35m"
#define COLOR_BRIGHT_CYAN    "\033[1;36m"
#define COLOR_BRIGHT_WHITE   "\033[1;37m"

/* Background colors */
#define BG_BLACK      "\033[40m"
#define BG_RED        "\033[41m"
#define BG_GREEN      "\033[42m"
#define BG_YELLOW     "\033[43m"
#define BG_BLUE       "\033[44m"
#define BG_MAGENTA    "\033[45m"
#define BG_CYAN       "\033[46m"
#define BG_WHITE      "\033[47m"

/* Styles */
#define STYLE_BOLD    "\033[1m"
#define STYLE_UNDERLINE "\033[4m"
#define STYLE_BLINK   "\033[5m"
#define STYLE_REVERSE "\033[7m"

// #region agent log
static void agent_log(const char *hypothesisId,
                      const char *location,
                      const char *message,
                      const char *dataJson) {
    const char *log_path = "/home/shammah/Documents/voting_system/sonu-voting-system/.cursor/debug-6301fb.log";
    /* Ensure .cursor directory exists so logging does not silently fail */
    mkdir("/home/shammah/Documents/voting_system/sonu-voting-system/.cursor", 0755);
    FILE *fp = fopen(log_path, "a");
    if (!fp) return;
    long ts = (long)time(NULL) * 1000L;
    if (!dataJson) dataJson = "{}";
    fprintf(fp,
            "{\"sessionId\":\"6301fb\",\"id\":\"log_%ld\",\"timestamp\":%ld,"
            "\"runId\":\"initial\",\"hypothesisId\":\"%s\","
            "\"location\":\"%s\",\"message\":\"%s\",\"data\":%s}\n",
            ts, ts, hypothesisId, location, message, dataJson);
    fclose(fp);
}
// #endregion

/* Main window and containers */
static GtkWidget *main_window;
static GtkWidget *main_stack;
static GtkWidget *header_bar;
static GtkWidget *back_button;

/* Login page widgets */
static GtkWidget *login_grid;
static GtkWidget *admin_username_entry;
static GtkWidget *admin_password_entry;
static GtkWidget *voter_id_entry;
static GtkWidget *voter_password_entry;
static GtkWidget *voter_name_entry;
static GtkWidget *voter_register_password_entry;

/* Admin page widgets */
static GtkWidget *admin_grid;
static GtkWidget *status_label;
static GtkWidget *positions_listbox;
static GtkWidget *admin_logout_btn;

/* Voting page widgets */
static GtkWidget *voting_grid;
static GtkWidget *voting_notebook;
static GtkWidget *voter_logout_btn;

/* Results page widgets */
static GtkWidget *results_grid;
static GtkWidget *results_textview;
static GtkWidget *results_logout_btn;

void show_message_dialog(const char *title, const char *message, GtkMessageType type) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           type,
                                           GTK_BUTTONS_OK,
                                           "%s", message);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void update_status_display(void) {
    char status[MAX_LINE_LEN];
    FILE *fp = fopen(STATUS_FILE, "r");
    if (fp) {
        if (fgets(status, sizeof(status), fp)) {
            status[strcspn(status, "\n")] = '\0';
        }
        fclose(fp);
    } else {
        strcpy(status, "Unknown");
    }
    
    char status_text[200];
    snprintf(status_text, sizeof(status_text), 
            "Election Status: <b>%s</b>\n"
            "Total Voters: %d\n"
            "Total Votes: %d",
            status, fh_count_records(VOTERS_FILE), fh_count_records(VOTES_FILE));
    gtk_label_set_markup(GTK_LABEL(status_label), status_text);
}

void on_admin_login(void) {
    const char *username = gtk_entry_get_text(GTK_ENTRY(admin_username_entry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(admin_password_entry));
    
    if (auth_admin_login(username, password) == SUCCESS) {
        gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "admin");
        gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Administrator Panel");
        update_status_display();
        refresh_positions_list();
    } else {
        show_message_dialog("Login Failed", "Invalid administrator credentials", GTK_MESSAGE_ERROR);
    }
}

void on_voter_self_register(void) {
    const char *name = gtk_entry_get_text(GTK_ENTRY(voter_name_entry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(voter_register_password_entry));
    
    if (strlen(name) > 0 && strlen(password) > 0) {
        if (voter_register(name, password) == SUCCESS) {
            Voter registered_voter;
            voter_get_by_id(voter_next_id() - 1, &registered_voter);
            
            char success_msg[200];
            snprintf(success_msg, sizeof(success_msg), 
                    "Voter registered successfully!\nYour Voter ID: %d\nPlease save this ID for login.",
                    registered_voter.id);
            show_message_dialog("Registration Successful", success_msg, GTK_MESSAGE_INFO);
            
            /* Clear registration form */
            gtk_entry_set_text(GTK_ENTRY(voter_name_entry), "");
            gtk_entry_set_text(GTK_ENTRY(voter_register_password_entry), "");
        } else {
            show_message_dialog("Error", "Failed to register voter", GTK_MESSAGE_ERROR);
        }
    } else {
        show_message_dialog("Error", "Please enter both name and password", GTK_MESSAGE_ERROR);
    }
}

void on_voter_login(void) {
    const char *voter_id_str = gtk_entry_get_text(GTK_ENTRY(voter_id_entry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(voter_password_entry));
    
    printf("DEBUG: Voter login attempt with ID: %s\n", voter_id_str);
    // #region agent log
    agent_log("H2", "final_gui.c:on_voter_login", "voter_login_attempt",
              "{\"voter_id_parsed\":0}");
    // #endregion
    
    int voter_id = atoi(voter_id_str);
    if (auth_voter_login(voter_id, password) == SUCCESS) {
        printf("DEBUG: Voter login successful, setting up voting interface\n");
        // #region agent log
        agent_log("H2", "final_gui.c:on_voter_login", "voter_login_success",
                  "{\"voter_id\":0}");
        // #endregion
        gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "voting");
        gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Voting Portal");
        setup_voting_interface();
    } else {
        printf("DEBUG: Voter login failed\n");
        // #region agent log
        agent_log("H2", "final_gui.c:on_voter_login", "voter_login_failed",
                  "{\"voter_id\":0}");
        // #endregion
        show_message_dialog("Login Failed", "Invalid voter ID or password", GTK_MESSAGE_ERROR);
    }
}

void on_back_to_login(void) {
    auth_logout();
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "login");
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "SONU Voting System");
    
    /* Clear login fields */
    gtk_entry_set_text(GTK_ENTRY(admin_username_entry), "");
    gtk_entry_set_text(GTK_ENTRY(admin_password_entry), "");
    gtk_entry_set_text(GTK_ENTRY(voter_id_entry), "");
    gtk_entry_set_text(GTK_ENTRY(voter_password_entry), "");
}

void on_open_voting(void) {
    FILE *fp = fopen(STATUS_FILE, "w");
    if (fp) {
        fprintf(fp, "%s\n", STATUS_OPEN);
        fclose(fp);
        update_status_display();
        show_message_dialog("Success", "Voting has been opened", GTK_MESSAGE_INFO);
    }
}

void on_close_voting(void) {
    FILE *fp = fopen(STATUS_FILE, "w");
    if (fp) {
        fprintf(fp, "%s\n", STATUS_CLOSED);
        fclose(fp);
        update_status_display();
        show_message_dialog("Success", "Voting has been closed", GTK_MESSAGE_INFO);
    }
}

void on_add_position(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add New Position",
                                                     GTK_WINDOW(main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Add", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget *label = gtk_label_new("Position Name:");
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter position name");
    gtk_widget_set_hexpand(entry, TRUE);
    
    /* Set up focus chain */
    GList *focus_chain = NULL;
    focus_chain = g_list_append(focus_chain, entry);
    gtk_container_set_focus_chain(GTK_CONTAINER(grid), focus_chain);
    g_list_free(focus_chain);
    
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry, 1, 0, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    
    /* Set initial focus to entry */
    gtk_widget_grab_focus(entry);
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (strlen(name) > 0) {
            if (pos_add(name) == SUCCESS) {
                refresh_positions_list();
                show_message_dialog("Success", "Position added successfully", GTK_MESSAGE_INFO);
            } else {
                show_message_dialog("Error", "Failed to add position", GTK_MESSAGE_ERROR);
            }
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_add_candidate(void) {
    GtkWidget *dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dialog), "Add New Candidate");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(main_window));
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(vbox, 20);
    gtk_widget_set_margin_bottom(vbox, 20);
    gtk_widget_set_margin_start(vbox, 20);
    gtk_widget_set_margin_end(vbox, 20);
    
    GtkWidget *name_label = gtk_label_new("Candidate Name:");
    gtk_widget_set_halign(name_label, GTK_ALIGN_START);
    
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Enter candidate name");
    gtk_widget_set_hexpand(name_entry, TRUE);
    
    GtkWidget *pos_label = gtk_label_new("Position:");
    gtk_widget_set_halign(pos_label, GTK_ALIGN_START);
    
    GtkWidget *pos_combo = gtk_combo_box_text_new();
    gtk_widget_set_hexpand(pos_combo, TRUE);
    
    /* Load positions into combo box */
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    if (pos_get_all(positions, &pos_count) == SUCCESS) {
        for (int i = 0; i < pos_count; i++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos_combo), positions[i].name);
            g_object_set_data(G_OBJECT(pos_combo), "pos_id_0", GINT_TO_POINTER(positions[i].id));
        }
        if (pos_count > 0) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(pos_combo), 0);
        }
    }
    
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_END);
    
    GtkWidget *cancel_btn = gtk_button_new_with_label("Cancel");
    GtkWidget *add_btn = gtk_button_new_with_label("Add");
    
    gtk_box_pack_start(GTK_BOX(button_box), cancel_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), add_btn, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), name_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), name_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), pos_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), pos_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(dialog), vbox);
    
    /* Signal handlers */
    g_signal_connect(cancel_btn, "clicked", G_CALLBACK(gtk_widget_destroy), dialog);
    g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_candidate_confirm), dialog);
    g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_show_all(dialog);
    
    /* Set initial focus to name entry */
    gtk_widget_grab_focus(name_entry);
    
    /* Run the dialog */
    gtk_main();
    
    /* Refresh positions list after dialog closes */
    refresh_positions_list();
}

void on_add_candidate_confirm(GtkWidget *button, gpointer user_data) {
    GtkWidget *dialog = GTK_WIDGET(user_data);
    GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(dialog));
    GList *children = gtk_container_get_children(GTK_CONTAINER(vbox));
    
    /* Find the name entry and position combo box */
    GtkWidget *name_entry = NULL;
    GtkWidget *pos_combo = NULL;
    int count = 0;
    
    for (GList *node = children; node != NULL; node = node->next) {
        GtkWidget *child = GTK_WIDGET(node->data);
        if (GTK_IS_ENTRY(child)) {
            name_entry = child;
        } else if (GTK_IS_COMBO_BOX(child)) {
            pos_combo = child;
        }
        count++;
        if (count > 4) break; /* We've found what we need */
    }
    
    g_list_free(children);
    
    if (name_entry && pos_combo) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        int active_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(pos_combo));
        
        if (strlen(name) > 0 && active_pos >= 0) {
            /* Get position ID */
            Position positions[MAX_POSITIONS];
            int pos_count = 0;
            if (pos_get_all(positions, &pos_count) == SUCCESS) {
                int position_id = positions[active_pos].id;
                
                if (cand_register(name, position_id) == SUCCESS) {
                    show_message_dialog("Success", "Candidate added successfully", GTK_MESSAGE_INFO);
                } else {
                    show_message_dialog("Error", "Failed to add candidate", GTK_MESSAGE_ERROR);
                }
            }
        } else {
            show_message_dialog("Error", "Please enter candidate name and select position", GTK_MESSAGE_ERROR);
            return; /* Don't close dialog on error */
        }
    }
    
    gtk_widget_destroy(dialog);
    gtk_main_quit();
}

void on_remove_position(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Remove Position",
                                                     GTK_WINDOW(main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Remove", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget *pos_label = gtk_label_new("Position:");
    GtkWidget *pos_combo = gtk_combo_box_text_new();
    
    /* Load positions into combo box */
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    if (pos_get_all(positions, &pos_count) == SUCCESS) {
        for (int i = 0; i < pos_count; i++) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pos_combo), positions[i].name);
            g_object_set_data(G_OBJECT(pos_combo), "pos_id_0", GINT_TO_POINTER(positions[i].id));
        }
    }
    
    GtkWidget *warning_label = gtk_label_new("WARNING: This will also remove all candidates for this position!");
    gtk_widget_set_sensitive(warning_label, FALSE);
    
    gtk_grid_attach(GTK_GRID(grid), pos_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pos_combo, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), warning_label, 0, 1, 2, 1);
    
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        int active_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(pos_combo));
        
        if (active_pos >= 0) {
            int position_id = positions[active_pos].id;
            
            /* Check if voting is open */
            FILE *fp = fopen(STATUS_FILE, "r");
            if (fp) {
                char status[MAX_LINE_LEN];
                if (fgets(status, sizeof(status), fp)) {
                    status[strcspn(status, "\n")] = '\0';
                    fclose(fp);
                    if (strcmp(status, STATUS_OPEN) == 0) {
                        show_message_dialog("Error", "Cannot remove positions while voting is open", GTK_MESSAGE_ERROR);
                        gtk_widget_destroy(dialog);
                        return;
                    }
                } else {
                    fclose(fp);
                }
            }
            
            /* Remove position and all its candidates */
            if (pos_delete(position_id) == SUCCESS) {
                /* Also remove all candidates for this position */
                Candidate candidates[MAX_CANDIDATES];
                int cand_count = 0;
                if (cand_get_for_position(position_id, candidates, &cand_count) == SUCCESS) {
                    for (int i = 0; i < cand_count; i++) {
                        cand_delete(candidates[i].id);
                    }
                }
                
                refresh_positions_list();
                show_message_dialog("Success", "Position and all its candidates removed successfully", GTK_MESSAGE_INFO);
            } else {
                show_message_dialog("Error", "Failed to remove position", GTK_MESSAGE_ERROR);
            }
        } else {
            show_message_dialog("Error", "Please select a position to remove", GTK_MESSAGE_ERROR);
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_remove_candidate(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Remove Candidate",
                                                     GTK_WINDOW(main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Remove", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget *cand_label = gtk_label_new("Candidate:");
    GtkWidget *cand_combo = gtk_combo_box_text_new();
    
    /* Load all candidates into combo box */
    Candidate candidates[MAX_CANDIDATES];
    int cand_count = 0;
    if (cand_get_all(candidates, &cand_count) == SUCCESS) {
        for (int i = 0; i < cand_count; i++) {
            /* Get position name for display */
            Position pos;
            char display_text[MAX_NAME_LEN * 2];
            if (pos_get_by_id(candidates[i].position_id, &pos) == SUCCESS) {
                snprintf(display_text, sizeof(display_text), "%s (%s)", candidates[i].name, pos.name);
            } else {
                snprintf(display_text, sizeof(display_text), "%s", candidates[i].name);
            }
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cand_combo), display_text);
            g_object_set_data(G_OBJECT(cand_combo), "cand_id_0", GINT_TO_POINTER(candidates[i].id));
        }
    }
    
    gtk_grid_attach(GTK_GRID(grid), cand_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), cand_combo, 1, 0, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        int active_cand = gtk_combo_box_get_active(GTK_COMBO_BOX(cand_combo));
        
        if (active_cand >= 0) {
            int candidate_id = candidates[active_cand].id;
            
            /* Check if voting is open */
            FILE *fp = fopen(STATUS_FILE, "r");
            if (fp) {
                char status[MAX_LINE_LEN];
                if (fgets(status, sizeof(status), fp)) {
                    status[strcspn(status, "\n")] = '\0';
                    fclose(fp);
                    if (strcmp(status, STATUS_OPEN) == 0) {
                        show_message_dialog("Error", "Cannot remove candidates while voting is open", GTK_MESSAGE_ERROR);
                        gtk_widget_destroy(dialog);
                        return;
                    }
                } else {
                    fclose(fp);
                }
            }
            
            if (cand_delete(candidate_id) == SUCCESS) {
                show_message_dialog("Success", "Candidate removed successfully", GTK_MESSAGE_INFO);
            } else {
                show_message_dialog("Error", "Failed to remove candidate", GTK_MESSAGE_ERROR);
            }
        } else {
            show_message_dialog("Error", "Please select a candidate to remove", GTK_MESSAGE_ERROR);
        }
    }
    
    gtk_widget_destroy(dialog);
}

void on_register_voter(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Register New Voter",
                                                     GTK_WINDOW(main_window),
                                                     GTK_DIALOG_MODAL,
                                                     "Cancel", GTK_RESPONSE_CANCEL,
                                                     "Register", GTK_RESPONSE_ACCEPT,
                                                     NULL);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    
    GtkWidget *name_label = gtk_label_new("Full Name:");
    GtkWidget *name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(name_entry), "Enter full name");
    gtk_widget_set_hexpand(name_entry, TRUE);
    
    GtkWidget *pass_label = gtk_label_new("Password:");
    GtkWidget *pass_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Enter password");
    gtk_widget_set_hexpand(pass_entry, TRUE);
    
    /* Set up focus chain */
    GList *focus_chain = NULL;
    focus_chain = g_list_append(focus_chain, name_entry);
    focus_chain = g_list_append(focus_chain, pass_entry);
    gtk_container_set_focus_chain(GTK_CONTAINER(grid), focus_chain);
    g_list_free(focus_chain);
    
    gtk_grid_attach(GTK_GRID(grid), name_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), name_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pass_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), pass_entry, 1, 1, 1, 1);
    
    gtk_container_add(GTK_CONTAINER(content_area), grid);
    gtk_widget_show_all(dialog);
    
    /* Set initial focus to name entry */
    gtk_widget_grab_focus(name_entry);
    
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    
    if (response == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(name_entry));
        const char *password = gtk_entry_get_text(GTK_ENTRY(pass_entry));
        
        if (strlen(name) > 0 && strlen(password) > 0) {
            if (voter_register(name, password) == SUCCESS) {
                Voter registered_voter;
                voter_get_by_id(voter_next_id() - 1, &registered_voter);
                
                char success_msg[200];
                snprintf(success_msg, sizeof(success_msg), 
                        "Voter registered successfully!\nVoter ID: %d\nPlease save this ID for login.",
                        registered_voter.id);
                show_message_dialog("Registration Successful", success_msg, GTK_MESSAGE_INFO);
            } else {
                show_message_dialog("Error", "Failed to register voter", GTK_MESSAGE_ERROR);
            }
        } else {
            show_message_dialog("Error", "Please enter both name and password", GTK_MESSAGE_ERROR);
        }
    }
    
    gtk_widget_destroy(dialog);
}

void refresh_positions_list(void) {
    /* Clear existing items */
    GList *children = gtk_container_get_children(GTK_CONTAINER(positions_listbox));
    for (GList *node = children; node != NULL; node = node->next) {
        gtk_widget_destroy(GTK_WIDGET(node->data));
    }
    g_list_free(children);
    
    /* Add positions */
    Position positions[MAX_POSITIONS];
    int count = 0;
    if (pos_get_all(positions, &count) == SUCCESS) {
        printf("DEBUG: Found %d positions\n", count);
        for (int i = 0; i < count; i++) {
            printf("DEBUG: Position %d: %s\n", positions[i].id, positions[i].name);
            GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
            GtkWidget *label = gtk_label_new(positions[i].name);
            gtk_widget_set_halign(label, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(row), label, TRUE, TRUE, 0);
            gtk_list_box_insert(GTK_LIST_BOX(positions_listbox), row, -1);
        }
        gtk_widget_show_all(positions_listbox);
    } else {
        printf("DEBUG: Failed to get positions\n");
    }
}

void setup_voting_interface(void) {
    /* Clear existing notebook pages */
    while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(voting_notebook)) > 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(voting_notebook), 0);
    }
    
    /* Add positions and candidates */
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    int pos_result = pos_get_all(positions, &pos_count);
    if (pos_result != SUCCESS) {
        printf("DEBUG: Failed to get positions for voting interface\n");
        // #region agent log
        agent_log("H1", "final_gui.c:setup_voting_interface",
                  "pos_get_all_failed",
                  "{\"pos_result\":-1,\"pos_count\":0}");
        // #endregion
        return;
    }

    // #region agent log
    {
        char data[128];
        snprintf(data, sizeof(data),
                 "{\"pos_result\":%d,\"pos_count\":%d}",
                 pos_result, pos_count);
        agent_log("H1", "final_gui.c:setup_voting_interface",
                  "pos_get_all_succeeded", data);
    }
    // #endregion
    
    printf("DEBUG: Setting up voting interface with %d positions\n", pos_count);
    
    if (pos_count == 0) {
        GtkWidget *no_positions_label = gtk_label_new("No positions available for voting.");
        gtk_widget_set_halign(no_positions_label, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(no_positions_label, GTK_ALIGN_CENTER);
        
        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled), no_positions_label);
        gtk_notebook_append_page(GTK_NOTEBOOK(voting_notebook), scrolled, 
                                 gtk_label_new("No Positions"));
        gtk_widget_show_all(voting_notebook);
        return;
    }
    
    for (int i = 0; i < pos_count; i++) {
        GtkWidget *page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
        gtk_widget_set_margin_top(page_vbox, 20);
        gtk_widget_set_margin_bottom(page_vbox, 20);
        gtk_widget_set_margin_start(page_vbox, 20);
        gtk_widget_set_margin_end(page_vbox, 20);
        
        GtkWidget *pos_label = gtk_label_new(positions[i].name);
        gtk_widget_set_name(pos_label, "position-title");
        gtk_widget_set_halign(pos_label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(page_vbox), pos_label, FALSE, FALSE, 0);
        
        /* Candidates for this position */
        Candidate candidates[MAX_CANDIDATES];
        int cand_count = 0;
        cand_get_for_position(positions[i].id, candidates, &cand_count);

        // #region agent log
        {
            char data[160];
            snprintf(data, sizeof(data),
                     "{\"position_id\":%d,\"cand_count\":%d}",
                     positions[i].id, cand_count);
            agent_log("H4", "final_gui.c:setup_voting_interface",
                      "cand_get_for_position_result", data);
        }
        // #endregion
        
        printf("DEBUG: Position '%s' has %d candidates\n", positions[i].name, cand_count);
        
        GtkWidget *cand_frame = gtk_frame_new("Candidates");
        gtk_widget_set_margin_top(cand_frame, 10);
        gtk_widget_set_margin_bottom(cand_frame, 10);
        GtkWidget *cand_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_margin_top(cand_vbox, 15);
        gtk_widget_set_margin_bottom(cand_vbox, 15);
        gtk_widget_set_margin_start(cand_vbox, 15);
        gtk_widget_set_margin_end(cand_vbox, 15);
        
        GSList *radio_group = NULL;
        for (int j = 0; j < cand_count; j++) {
            GtkWidget *cand_radio;
            if (j == 0) {
                cand_radio = gtk_radio_button_new_with_label(NULL, candidates[j].name);
                radio_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(cand_radio));
            } else {
                cand_radio = gtk_radio_button_new_with_label(radio_group, candidates[j].name);
            }
            g_object_set_data(G_OBJECT(cand_radio), "candidate_id", GINT_TO_POINTER(candidates[j].id));
            
            /* Make radio buttons larger and more readable */
            GtkWidget *radio_label = gtk_bin_get_child(GTK_BIN(cand_radio));
            if (GTK_IS_LABEL(radio_label)) {
                PangoFontDescription *font_desc = pango_font_description_from_string("Sans 12");
                gtk_widget_modify_font(radio_label, font_desc);
                pango_font_description_free(font_desc);
            }
            
            gtk_widget_set_size_request(cand_radio, -1, 35);
            gtk_box_pack_start(GTK_BOX(cand_vbox), cand_radio, FALSE, FALSE, 0);
        }
        
        /* Show message if no candidates */
        if (cand_count == 0) {
            GtkWidget *no_candidates_label = gtk_label_new("No candidates registered for this position yet.");
            gtk_widget_set_sensitive(no_candidates_label, FALSE);
            PangoFontDescription *font_desc = pango_font_description_from_string("Sans Italic 12");
            gtk_widget_modify_font(no_candidates_label, font_desc);
            pango_font_description_free(font_desc);
            gtk_box_pack_start(GTK_BOX(cand_vbox), no_candidates_label, FALSE, FALSE, 0);
        }
        
        gtk_container_add(GTK_CONTAINER(cand_frame), cand_vbox);
        gtk_box_pack_start(GTK_BOX(page_vbox), cand_frame, TRUE, TRUE, 0);
        
        /* Create tab label */
        GtkWidget *tab_label = gtk_label_new(positions[i].name);
        PangoFontDescription *tab_font_desc = pango_font_description_from_string("Sans Bold 11");
        gtk_widget_modify_font(tab_label, tab_font_desc);
        pango_font_description_free(tab_font_desc);
        
        /* Add page to notebook */
        GtkWidget *scrolled_page = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_page),
                                     GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled_page), page_vbox);
        
        gtk_notebook_append_page(GTK_NOTEBOOK(voting_notebook), scrolled_page, tab_label);
    }
    
    gtk_widget_show_all(voting_notebook);
}

void on_cast_vote(void) {
    /* Check if voting is open */
    FILE *fp = fopen(STATUS_FILE, "r");
    if (fp) {
        char status[MAX_LINE_LEN];
        if (fgets(status, sizeof(status), fp)) {
            status[strcspn(status, "\n")] = '\0';
            if (strcmp(status, STATUS_OPEN) != 0) {
                show_message_dialog("Voting Closed", "Voting is currently closed. Please try again later.", GTK_MESSAGE_WARNING);
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }
    
    int voter_id = auth_get_voter_id();
    if (voter_has_voted(voter_id)) {
        show_message_dialog("Already Voted", "You have already cast your vote.", GTK_MESSAGE_WARNING);
        return;
    }
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    pos_get_all(positions, &pos_count);
    
    int votes_cast = 0;
    for (int i = 0; i < pos_count; i++) {
        GtkWidget *scrolled_page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(voting_notebook), i);
        GtkWidget *page_vbox = NULL;
        
        /* Handle viewport in scrolled window */
        if (GTK_IS_SCROLLED_WINDOW(scrolled_page)) {
            GtkWidget *viewport = gtk_bin_get_child(GTK_BIN(scrolled_page));
            if (GTK_IS_VIEWPORT(viewport)) {
                page_vbox = gtk_bin_get_child(GTK_BIN(viewport));
            } else {
                page_vbox = viewport; /* Direct child if no viewport */
            }
        } else {
            page_vbox = scrolled_page;
        }
        
        if (!page_vbox) {
            continue;
        }
        
        /* The page_vbox should contain: GtkLabel (position title) and GtkFrame (candidates) */
        GList *children = gtk_container_get_children(GTK_CONTAINER(page_vbox));
        GtkWidget *frame = NULL;
        
        /* Find the candidates frame by checking each child */
        for (GList *node = children; node != NULL; node = node->next) {
            GtkWidget *child = GTK_WIDGET(node->data);
            if (GTK_IS_FRAME(child)) {
                const char *label = gtk_frame_get_label(GTK_FRAME(child));
                if (label && strcmp(label, "Candidates") == 0) {
                    frame = child;
                    break;
                }
            }
        }
        
        if (frame) {
            GtkWidget *frame_vbox = gtk_bin_get_child(GTK_BIN(frame));
            GList *radio_buttons = gtk_container_get_children(GTK_CONTAINER(frame_vbox));
            
            int selected_candidate_id = -1;
            for (GList *node = radio_buttons; node != NULL; node = node->next) {
                GtkWidget *radio = GTK_WIDGET(node->data);
                if (GTK_IS_RADIO_BUTTON(radio) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio))) {
                    selected_candidate_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(radio), "candidate_id"));
                    break;
                }
            }
            
            if (selected_candidate_id >= 0) {
                int result = voting_cast_vote(voter_id, positions[i].id, selected_candidate_id);
                if (result == SUCCESS) votes_cast++;
            }
            g_list_free(radio_buttons);
        }
        g_list_free(children);
    }
    
    if (votes_cast == pos_count) {
        voter_mark_voted(voter_id);
        show_message_dialog("Vote Cast", "Your vote has been successfully recorded. Thank you!", GTK_MESSAGE_INFO);
        on_back_to_login();
    } else {
        show_message_dialog("Incomplete Vote", "Please select a candidate for each position.", GTK_MESSAGE_WARNING);
    }
}

void on_view_results(void) {
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "results");
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "Election Results");
    display_results();
}

void display_results(void) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(results_textview));
    gtk_text_buffer_set_text(buffer, "", -1);
    
    Result results[MAX_CANDIDATES];
    int count = 0;
    if (tally_compute(results, &count) != SUCCESS) {
        gtk_text_buffer_insert_at_cursor(buffer, "Error computing results.\n", -1);
        return;
    }
    
    /* Add results to text view */
    char *results_text = g_malloc(2000);
    strcpy(results_text, "=== ELECTION RESULTS ===\n\n");
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    pos_get_all(positions, &pos_count);
    
    for (int p = 0; p < pos_count; p++) {
        char pos_title[100];
        snprintf(pos_title, sizeof(pos_title), "--- Results for: %s ---\n", positions[p].name);
        strcat(results_text, pos_title);
        
        for (int i = 0; i < count; i++) {
            if (results[i].position_id == positions[p].id) {
                char result_line[100];
                snprintf(result_line, sizeof(result_line), 
                        "%-20s %d votes (%.1f%%)\n",
                        results[i].candidate_name,
                        results[i].vote_count,
                        results[i].percentage);
                strcat(results_text, result_line);
            }
        }
        
        Result winner;
        if (tally_get_winner(positions[p].id, &winner) == SUCCESS) {
            char winner_line[100];
            snprintf(winner_line, sizeof(winner_line), 
                    "\nWINNER: %s (%d votes)\n\n",
                    winner.candidate_name, winner.vote_count);
            strcat(results_text, winner_line);
        }
    }
    
    char summary[100];
    snprintf(summary, sizeof(summary), 
            "Total Votes Cast: %d\nVoter Turnout: %.1f%%\n",
            fh_count_records(VOTES_FILE), tally_voter_turnout());
    strcat(results_text, summary);
    
    gtk_text_buffer_set_text(buffer, results_text, -1);
    g_free(results_text);
}

void create_login_page(void) {
    login_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(login_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(login_grid), 20);
    gtk_grid_set_row_homogeneous(GTK_GRID(login_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(login_grid), TRUE);
    
    /* Admin login section */
    GtkWidget *admin_frame = gtk_frame_new("Administrator");
    GtkWidget *admin_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    GtkWidget *admin_user_label = gtk_label_new("Username:");
    admin_username_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(admin_username_entry), "admin");
    
    GtkWidget *admin_pass_label = gtk_label_new("Password:");
    admin_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(admin_password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(admin_password_entry), "admin123");
    
    GtkWidget *admin_login_btn = gtk_button_new_with_label("Login");
    g_signal_connect(admin_login_btn, "clicked", G_CALLBACK(on_admin_login), NULL);
    
    gtk_box_pack_start(GTK_BOX(admin_vbox), admin_user_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(admin_vbox), admin_username_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(admin_vbox), admin_pass_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(admin_vbox), admin_password_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(admin_vbox), admin_login_btn, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(admin_frame), admin_vbox);
    
    /* Voter login section */
    GtkWidget *voter_frame = gtk_frame_new("Voter");
    GtkWidget *voter_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    GtkWidget *voter_id_label = gtk_label_new("Voter ID:");
    voter_id_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(voter_id_entry), "Enter ID");
    
    GtkWidget *voter_pass_label = gtk_label_new("Password:");
    voter_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(voter_password_entry), FALSE);
    
    GtkWidget *voter_login_btn = gtk_button_new_with_label("Login");
    g_signal_connect(voter_login_btn, "clicked", G_CALLBACK(on_voter_login), NULL);
    
    gtk_box_pack_start(GTK_BOX(voter_vbox), voter_id_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(voter_vbox), voter_id_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(voter_vbox), voter_pass_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(voter_vbox), voter_password_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(voter_vbox), voter_login_btn, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(voter_frame), voter_vbox);
    
    /* Voter self-registration section */
    GtkWidget *register_frame = gtk_frame_new("New Voter Registration");
    GtkWidget *register_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    GtkWidget *register_name_label = gtk_label_new("Full Name:");
    voter_name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(voter_name_entry), "Enter your full name");
    
    GtkWidget *register_pass_label = gtk_label_new("Password:");
    voter_register_password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(voter_register_password_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(voter_register_password_entry), "Create a password");
    
    GtkWidget *register_btn = gtk_button_new_with_label("Register");
    g_signal_connect(register_btn, "clicked", G_CALLBACK(on_voter_self_register), NULL);
    
    gtk_box_pack_start(GTK_BOX(register_vbox), register_name_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(register_vbox), voter_name_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(register_vbox), register_pass_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(register_vbox), voter_register_password_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(register_vbox), register_btn, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(register_frame), register_vbox);
    
    /* Add all sections to grid */
    gtk_grid_attach(GTK_GRID(login_grid), admin_frame, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(login_grid), voter_frame, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(login_grid), register_frame, 0, 1, 1, 1);
}

void create_admin_page(void) {
    admin_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(admin_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(admin_grid), 20);
    
    /* Status display */
    status_label = gtk_label_new("Loading status...");
    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_widget_set_valign(status_label, GTK_ALIGN_START);
    gtk_label_set_markup(GTK_LABEL(status_label), "<b>Loading...</b>");
    
    /* Positions list */
    GtkWidget *positions_frame = gtk_frame_new("Positions");
    positions_listbox = gtk_list_box_new();
    GtkWidget *positions_scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(positions_scrolled),
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(positions_scrolled, 300, 200);
    gtk_container_add(GTK_CONTAINER(positions_scrolled), positions_listbox);
    gtk_container_add(GTK_CONTAINER(positions_frame), positions_scrolled);
    
    /* Control buttons */
    GtkWidget *controls_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    
    GtkWidget *add_pos_btn = gtk_button_new_with_label("Add Position");
    g_signal_connect(add_pos_btn, "clicked", G_CALLBACK(on_add_position), NULL);
    
    GtkWidget *register_voter_btn = gtk_button_new_with_label("Register Voter");
    g_signal_connect(register_voter_btn, "clicked", G_CALLBACK(on_register_voter), NULL);
    
    GtkWidget *open_voting_btn = gtk_button_new_with_label("Open Voting");
    g_signal_connect(open_voting_btn, "clicked", G_CALLBACK(on_open_voting), NULL);
    
    GtkWidget *close_voting_btn = gtk_button_new_with_label("Close Voting");
    g_signal_connect(close_voting_btn, "clicked", G_CALLBACK(on_close_voting), NULL);
    
    GtkWidget *results_btn = gtk_button_new_with_label("View Results");
    g_signal_connect(results_btn, "clicked", G_CALLBACK(on_view_results), NULL);
    
    GtkWidget *add_candidate_btn = gtk_button_new_with_label("Add Candidate");
    g_signal_connect(add_candidate_btn, "clicked", G_CALLBACK(on_add_candidate), NULL);
    
    GtkWidget *remove_pos_btn = gtk_button_new_with_label("Remove Position");
    g_signal_connect(remove_pos_btn, "clicked", G_CALLBACK(on_remove_position), NULL);
    
    GtkWidget *remove_candidate_btn = gtk_button_new_with_label("Remove Candidate");
    g_signal_connect(remove_candidate_btn, "clicked", G_CALLBACK(on_remove_candidate), NULL);
    
    gtk_box_pack_start(GTK_BOX(controls_vbox), add_pos_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), remove_pos_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), add_candidate_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), remove_candidate_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), register_voter_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), open_voting_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), close_voting_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(controls_vbox), results_btn, FALSE, FALSE, 0);
    
    admin_logout_btn = gtk_button_new_with_label("Logout");
    g_signal_connect(admin_logout_btn, "clicked", G_CALLBACK(on_back_to_login), NULL);
    gtk_box_pack_start(GTK_BOX(controls_vbox), admin_logout_btn, FALSE, FALSE, 0);
    
    /* Layout */
    gtk_grid_attach(GTK_GRID(admin_grid), status_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(admin_grid), positions_frame, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(admin_grid), controls_vbox, 1, 0, 1, 2);
}

void create_voting_page(void) {
    voting_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(voting_grid), 20);
    gtk_grid_set_column_spacing(GTK_GRID(voting_grid), 20);
    
    voting_notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(voting_notebook), GTK_POS_TOP);
    gtk_widget_set_hexpand(voting_notebook, TRUE);
    gtk_widget_set_vexpand(voting_notebook, TRUE);
    
    /* Create a larger scrolled window for the voting area */
    GtkWidget *scrolled_notebook = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_notebook),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_notebook, 700, 400);
    gtk_container_add(GTK_CONTAINER(scrolled_notebook), voting_notebook);
    
    /* Button container */
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    
    GtkWidget *vote_btn = gtk_button_new_with_label("Cast Vote");
    gtk_widget_set_size_request(vote_btn, 120, 40);
    g_signal_connect(vote_btn, "clicked", G_CALLBACK(on_cast_vote), NULL);
    
    voter_logout_btn = gtk_button_new_with_label("Logout");
    gtk_widget_set_size_request(voter_logout_btn, 120, 40);
    g_signal_connect(voter_logout_btn, "clicked", G_CALLBACK(on_back_to_login), NULL);
    
    gtk_box_pack_start(GTK_BOX(button_box), vote_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), voter_logout_btn, FALSE, FALSE, 0);
    
    /* Layout with better spacing */
    gtk_grid_attach(GTK_GRID(voting_grid), scrolled_notebook, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(voting_grid), button_box, 0, 1, 1, 1);
}

void create_results_page(void) {
    results_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(results_grid), 20);
    
    results_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(results_textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(results_textview), GTK_WRAP_WORD);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(results_textview), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(results_textview), 10);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(results_textview), 10);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(results_textview), 10);
    
    GtkWidget *scrolled_results = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_results),
                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_results, 600, 400);
    
    gtk_container_add(GTK_CONTAINER(scrolled_results), results_textview);
    gtk_grid_attach(GTK_GRID(results_grid), scrolled_results, 0, 0, 1, 1);
    
    results_logout_btn = gtk_button_new_with_label("Back to Login");
    g_signal_connect(results_logout_btn, "clicked", G_CALLBACK(on_back_to_login), NULL);
    
    gtk_grid_attach(GTK_GRID(results_grid), results_logout_btn, 0, 1, 1, 1);
}

void apply_css_styling(void) {
    GtkCssProvider *css_provider = gtk_css_provider_new();
    const char *css = 
        "window {"
        "  background-color: #f8f9fa;"
        "}"
        "headerbar {"
        "  background-color: #343a40;"
        "  color: white;"
        "  padding: 10px;"
        "}"
        "headerbar label {"
        "  font-weight: bold;"
        "  font-size: 16px;"
        "}"
        "frame {"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 8px;"
        "  padding: 15px;"
        "  background-color: white;"
        "}"
        "button {"
        "  background-color: #007bff;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "}"
        "button:hover {"
        "  background-color: #0056b3;"
        "}"
        "entry {"
        "  border: 1px solid #ced4da;"
        "  border-radius: 4px;"
        "  padding: 8px;"
        "  margin: 2px 0;"
        "}"
        "entry:focus {"
        "  border-color: #007bff;"
        "  outline: none;"
        "}"
        "listbox {"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "  background-color: white;"
        "}"
        "listbox row {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #f8f9fa;"
        "}"
        "listbox row:hover {"
        "  background-color: #f8f9fa;"
        "}"
        "notebook {"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "  background-color: white;"
        "}"
        "notebook tab {"
        "  background-color: #e9ecef;"
        "  border: 1px solid #dee2e6;"
        "  border-bottom: none;"
        "  padding: 8px 12px;"
        "}"
        "notebook tab:hover {"
        "  background-color: #dee2e6;"
        "}"
        "textview {"
        "  font-family: monospace;"
        "  font-size: 12px;"
        "  background-color: white;"
        "  border: 1px solid #dee2e6;"
        "  border-radius: 4px;"
        "}"
        "#position-title {"
        "  font-weight: bold;"
        "  font-size: 14px;"
        "  color: #495057;"
        "  margin-bottom: 10px;"
        "}";
    
    gtk_css_provider_load_from_data(css_provider, css, -1, NULL);
    GdkScreen *screen = gdk_screen_get_default();
    gtk_style_context_add_provider_for_screen(screen,
                                          GTK_STYLE_PROVIDER(css_provider),
                                          GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

/* Terminal Mode Implementation */
void reset_colors(void) {
    printf(COLOR_RESET);
}

void print_header(const char *title, const char *subtitle) {
    printf("\n" COLOR_BRIGHT_CYAN);
    printf("╔════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║%*s%s%*s║\n", 50, "", title, 50, "");
    printf("║%*s%s%*s║\n", 50, "", subtitle, 50, "");
    printf("╚════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    reset_colors();
}

void print_footer(void) {
    printf(COLOR_BRIGHT_CYAN);
    printf("╚════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    reset_colors();
}

void print_menu_item(int number, const char *icon, const char *title, const char *description) {
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW "  %d. %s " COLOR_BRIGHT_WHITE "%-25s" COLOR_WHITE " %s%*s║\n", 
           number, icon, title, description, 35 - (int)strlen(title) - (int)strlen(description), "");
    reset_colors();
}

void print_section_header(const char *title) {
    printf(COLOR_BRIGHT_BLUE);
    printf("╠════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    printf("║%*s%s%*s║\n", 50, "", title, 50, "");
    printf("╠════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");
    reset_colors();
}

void print_success_message(const char *message) {
    printf(COLOR_BRIGHT_GREEN "✅ %s" COLOR_RESET "\n", message);
}

void print_error_message(const char *message) {
    printf(COLOR_BRIGHT_RED "❌ %s" COLOR_RESET "\n", message);
}

void print_warning_message(const char *message) {
    printf(COLOR_BRIGHT_YELLOW "⚠️  %s" COLOR_RESET "\n", message);
}

void print_info_message(const char *message) {
    printf(COLOR_BRIGHT_CYAN "ℹ️  %s" COLOR_RESET "\n", message);
}

void print_separator(void) {
    printf(COLOR_BRIGHT_CYAN "├────────────────────────────────────────────────────────────────────────────────────────────┤" COLOR_RESET "\n");
}

void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void show_startup_menu(void) {
    clear_screen();
    
    print_header(COLOR_BRIGHT_WHITE "SONU VOTING SYSTEM" COLOR_RESET, COLOR_BRIGHT_CYAN "Professional Dual-Mode Interface" COLOR_RESET);
    
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW "  🎯" COLOR_WHITE " Choose your preferred interface:" COLOR_WHITE " %*s║\n", 95, "");
    print_separator();
    
    printf(COLOR_WHITE "║");
    printf(COLOR_BRIGHT_GREEN "  1. 🖥️  Graphical User Interface (GUI)" COLOR_WHITE " %*s║\n", 60, "");
    printf(COLOR_WHITE "║     " COLOR_CYAN "• Modern, user-friendly interface" COLOR_WHITE " %*s║\n", 65, "");
    printf(COLOR_WHITE "║     " COLOR_CYAN "• Click-based navigation" COLOR_WHITE " %*s║\n", 71, "");
    printf(COLOR_WHITE "║     " COLOR_CYAN "• Visual candidate selection" COLOR_WHITE " %*s║\n", 65, "");
    print_separator();
    
    printf(COLOR_WHITE "║");
    printf(COLOR_BRIGHT_BLUE "  2. 💻  Terminal/Command Line Interface" COLOR_WHITE " %*s║\n", 55, "");
    printf(COLOR_WHITE "║     " COLOR_CYAN "• Keyboard-based navigation" COLOR_WHITE " %*s║\n", 68, "");
    printf(COLOR_WHITE "║     " COLOR_CYAN "• Lightweight and fast" COLOR_WHITE " %*s║\n", 75, "");
    printf(COLOR_WHITE "║     " COLOR_CYAN "• Works on any terminal" COLOR_WHITE " %*s║\n", 72, "");
    print_separator();
    
    printf(COLOR_WHITE "║");
    printf(COLOR_BRIGHT_RED "  3. ❌  Exit Program" COLOR_WHITE " %*s║\n", 82, "");
    printf(COLOR_WHITE "║" COLOR_CYAN " %*s║\n", 118, "");
    print_footer();
    
    printf(COLOR_BRIGHT_YELLOW "\n  → Enter your choice (1-3): " COLOR_RESET);
}

void run_terminal_mode(void) {
    int choice;
    
    while (1) {
        clear_screen();
        print_header(COLOR_BRIGHT_WHITE "SONU VOTING SYSTEM" COLOR_RESET, COLOR_BRIGHT_CYAN "Terminal Mode v2.0 - Professional Edition" COLOR_RESET);
        
        printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 🌟" COLOR_WHITE " Main Menu:" COLOR_WHITE " %*s║\n", 97, "");
        print_separator();
        
        print_menu_item(1, "👤", "Voter Login", "Cast your vote securely");
        print_menu_item(2, "📝", "Voter Registration", "Register as a new voter");
        print_menu_item(3, "🔑", "Admin Login", "Access admin controls");
        print_menu_item(4, "📊", "View Election Results", "See current voting results");
        print_menu_item(5, "ℹ️", "View System Status", "Check election status and statistics");
        print_menu_item(6, "🏠", "Return to Interface Selection", "Switch between GUI and Terminal");
        print_menu_item(7, "❌", "Exit Program", "Close the application");
        
        printf(COLOR_WHITE "║" COLOR_CYAN " %*s║\n", 118, "");
        print_footer();
        
        printf(COLOR_BRIGHT_YELLOW "\n  → Enter your choice (1-7): " COLOR_RESET);
        
        if (scanf("%d", &choice) != 1) {
            print_error_message("Invalid input. Please enter a number.");
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                terminal_voter_login();
                break;
            case 2:
                terminal_voter_register();
                break;
            case 3:
                terminal_admin_menu();
                break;
            case 4:
                terminal_view_results();
                break;
            case 5:
                terminal_view_status();
                break;
            case 6:
                return; /* Return to interface selection */
            case 7:
                print_success_message("Thank you for using SONU Voting System. Goodbye!");
                exit(0);
            default:
                print_error_message("Invalid choice. Please try again.");
                printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
                while (getchar() != '\n');
                getchar();
        }
    }
}

void terminal_voter_register(void) {
    clear_screen();
    print_header(COLOR_BRIGHT_WHITE "VOTER REGISTRATION" COLOR_RESET, COLOR_BRIGHT_CYAN "Create Your Voting Account" COLOR_RESET);
    
    char name[100];
    char password[20];
    
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 📋" COLOR_WHITE " Please provide your details:" COLOR_WHITE " %*s║\n", 85, "");
    print_separator();
    
    printf(COLOR_CYAN "\n  🎯 Enter your full name: " COLOR_RESET);
    while (getchar() != '\n');
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    
    printf(COLOR_CYAN "  🔐 Enter password (10-20 chars): " COLOR_RESET);
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    if (strlen(password) < 10) {
        print_error_message("Password must be at least 10 characters long.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        getchar();
        return;
    }
    
    Voter voter;
    strncpy(voter.name, name, sizeof(voter.name) - 1);
    strncpy(voter.password, password, sizeof(voter.password) - 1);
    voter.has_voted = 0;
    
    if (voter_register(name, password) == SUCCESS) {
        print_success_message("Registration successful!");
        printf(COLOR_BRIGHT_GREEN "\n  🎉 Your Voter ID: %d" COLOR_RESET, voter.id);
        printf(COLOR_CYAN "\n  💡 Please save your ID for future login." COLOR_RESET);
    } else {
        print_error_message("Registration failed. Please try again.");
    }
    
    printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
    getchar();
}

void terminal_voter_login(void) {
    clear_screen();
    print_header(COLOR_BRIGHT_WHITE "VOTER LOGIN" COLOR_RESET, COLOR_BRIGHT_CYAN "Access Your Voting Account" COLOR_RESET);
    
    int voter_id;
    char password[20];
    
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 🔐" COLOR_WHITE " Enter your credentials:" COLOR_WHITE " %*s║\n", 85, "");
    print_separator();
    
    printf(COLOR_CYAN "\n  🆔 Enter your Voter ID: " COLOR_RESET);
    if (scanf("%d", &voter_id) != 1) {
        print_error_message("Invalid ID format.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        while (getchar() != '\n');
        getchar();
        return;
    }
    
    printf(COLOR_CYAN "  🔑 Enter your password: " COLOR_RESET);
    while (getchar() != '\n');
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    if (auth_voter_login(voter_id, password) == SUCCESS) {
        print_success_message("Login successful!");
        printf(COLOR_CYAN "\n  Press Enter to continue to voting..." COLOR_RESET);
        getchar();
        terminal_cast_vote();
    } else {
        print_error_message("Invalid credentials. Please try again.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        getchar();
    }
}

void terminal_cast_vote(void) {
    clear_screen();
    print_header(COLOR_BRIGHT_WHITE "CAST YOUR VOTE" COLOR_RESET, COLOR_BRIGHT_CYAN "Make Your Voice Heard" COLOR_RESET);
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    
    if (pos_get_all(positions, &pos_count) != SUCCESS || pos_count == 0) {
        print_error_message("No positions available for voting.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        getchar();
        return;
    }
    
    int voter_id = auth_get_voter_id();
    if (voter_has_voted(voter_id)) {
        print_error_message("You have already cast your vote.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        getchar();
        return;
    }
    
    int votes[MAX_POSITIONS];
    int valid_votes = 0;
    
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 🗳️" COLOR_WHITE " Select your candidates:" COLOR_WHITE " %*s║\n", 85, "");
    print_separator();
    
    for (int i = 0; i < pos_count; i++) {
        printf(COLOR_BRIGHT_BLUE "\n  📋 Position %d: %s" COLOR_RESET, i + 1, positions[i].name);
        printf(COLOR_BRIGHT_CYAN "\n  ┌─────────────────────────────────────────────────────────────┐" COLOR_RESET);
        
        Candidate candidates[MAX_CANDIDATES];
        int cand_count = 0;
        cand_get_for_position(positions[i].id, candidates, &cand_count);
        
        if (cand_count == 0) {
            printf(COLOR_CYAN "  │ No candidates available for this position." COLOR_RESET);
            printf(COLOR_BRIGHT_CYAN "\n  └─────────────────────────────────────────────────────────────┘" COLOR_RESET);
            continue;
        }
        
        for (int j = 0; j < cand_count; j++) {
            printf(COLOR_CYAN "  │ %d. %s", j + 1, candidates[j].name);
            printf("%*s│", 50 - (j + 1) - (int)strlen(candidates[j].name), "");
        }
        printf(COLOR_BRIGHT_CYAN "  ├─────────────────────────────────────────────────────────────┤" COLOR_RESET);
        printf(COLOR_CYAN "  │ Enter your choice (1-%d): ", cand_count);
        printf("%*s│", 37, "");
        
        int choice;
        if (scanf("%d", &choice) != 1 || choice < 1 || choice > cand_count) {
            printf(COLOR_RED "  │ Invalid choice. Skipping this position." COLOR_RESET);
            printf(COLOR_BRIGHT_CYAN "  └─────────────────────────────────────────────────────────────┘" COLOR_RESET);
            while (getchar() != '\n');
            continue;
        }
        
        votes[i] = candidates[choice - 1].id;
        valid_votes++;
        printf(COLOR_GREEN "  │ Selected: %s", candidates[choice - 1].name);
        printf("%*s│", 37 - (int)strlen(candidates[choice - 1].name), "");
        printf(COLOR_BRIGHT_CYAN "  └─────────────────────────────────────────────────────────────┘" COLOR_RESET);
        while (getchar() != '\n');
    }
    
    if (valid_votes == pos_count) {
        printf(COLOR_BRIGHT_YELLOW "\n  ⚠️  Confirm your vote? (y/n): " COLOR_RESET);
        char confirm;
        scanf(" %c", &confirm);
        
        if (confirm == 'y' || confirm == 'Y') {
            int success_count = 0;
            for (int i = 0; i < pos_count; i++) {
                if (voting_cast_vote(voter_id, positions[i].id, votes[i]) == SUCCESS) {
                    success_count++;
                }
            }
            
            if (success_count == pos_count) {
                voter_mark_voted(voter_id);
                print_success_message("Your vote has been successfully recorded!");
                printf(COLOR_BRIGHT_GREEN "\n  🎉 Thank you for participating in the election." COLOR_RESET);
            } else {
                print_error_message("Error recording vote. Please try again.");
            }
        } else {
            print_warning_message("Vote cancelled.");
        }
    } else {
        print_error_message("Please vote for all positions to complete your vote.");
    }
    
    printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
    while (getchar() != '\n');
    getchar();
}

void terminal_view_results(void) {
    clear_screen();
    print_header(COLOR_BRIGHT_WHITE "ELECTION RESULTS" COLOR_RESET, COLOR_BRIGHT_CYAN "Live Voting Statistics" COLOR_RESET);
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    
    if (pos_get_all(positions, &pos_count) != SUCCESS || pos_count == 0) {
        print_error_message("No positions available.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        getchar();
        return;
    }
    
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 📊" COLOR_WHITE " Current Results:" COLOR_WHITE " %*s║\n", 85, "");
    print_separator();
    
    for (int i = 0; i < pos_count; i++) {
        printf(COLOR_BRIGHT_BLUE "\n  🏆 Position: %s" COLOR_RESET, positions[i].name);
        printf(COLOR_BRIGHT_CYAN "\n  ┌─────────────────────────────────────────────────────────────┐" COLOR_RESET);
        
        Candidate candidates[MAX_CANDIDATES];
        int cand_count = 0;
        cand_get_for_position(positions[i].id, candidates, &cand_count);
        
        if (cand_count == 0) {
            printf(COLOR_CYAN "  │ No candidates registered for this position." COLOR_RESET);
            printf(COLOR_BRIGHT_CYAN "\n  └─────────────────────────────────────────────────────────────┘" COLOR_RESET);
            continue;
        }
        
        for (int j = 0; j < cand_count; j++) {
            int vote_count = tally_get_votes_for(candidates[j].id, positions[i].id);
            printf(COLOR_CYAN "  │ %s", candidates[j].name);
            printf("%*s│", 50 - (int)strlen(candidates[j].name), "");
            printf(COLOR_BRIGHT_GREEN "  │ Votes: %d", vote_count);
            printf("%*s│", 35 - (vote_count > 99 ? 3 : vote_count > 9 ? 2 : 1), "");
        }
        printf(COLOR_BRIGHT_CYAN "  └─────────────────────────────────────────────────────────────┘" COLOR_RESET);
    }
    
    printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
    while (getchar() != '\n');
    getchar();
}

void terminal_view_status(void) {
    clear_screen();
    print_header(COLOR_BRIGHT_WHITE "SYSTEM STATUS" COLOR_RESET, COLOR_BRIGHT_CYAN "Election Overview & Statistics" COLOR_RESET);
    
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
    
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 📈" COLOR_WHITE " Current Statistics:" COLOR_WHITE " %*s║\n", 85, "");
    print_separator();
    
    printf(COLOR_BRIGHT_CYAN "\n  ┌─────────────────────────────────────────────────────────────┐" COLOR_RESET);
    printf(COLOR_CYAN "  │ Election Status: ");
    if (strcmp(status, "open") == 0) {
        printf(COLOR_BRIGHT_GREEN "%s", status);
    } else if (strcmp(status, "closed") == 0) {
        printf(COLOR_BRIGHT_RED "%s", status);
    } else {
        printf(COLOR_BRIGHT_YELLOW "%s", status);
    }
    printf("%*s│", 35 - (int)strlen(status), "");
    
    printf(COLOR_CYAN "  │ Total Voters: %d", total_voters);
    printf("%*s│", 35 - (total_voters > 99 ? 3 : total_voters > 9 ? 2 : 1), "");
    
    printf(COLOR_CYAN "  │ Total Votes Cast: %d", total_votes);
    printf("%*s│", 30 - (total_votes > 99 ? 3 : total_votes > 9 ? 2 : 1), "");
    
    printf(COLOR_CYAN "  │ Participation Rate: %.1f%%", total_voters > 0 ? (float)total_votes / total_voters * 100 : 0);
    printf("%*s│", 25, "");
    printf(COLOR_BRIGHT_CYAN "  └─────────────────────────────────────────────────────────────┘" COLOR_RESET);
    
    printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
    while (getchar() != '\n');
    getchar();
}

void terminal_admin_menu(void) {
    clear_screen();
    print_header(COLOR_BRIGHT_WHITE "ADMIN PANEL" COLOR_RESET, COLOR_BRIGHT_CYAN "System Control Center" COLOR_RESET);
    
    char password[20];
    printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 🔐" COLOR_WHITE " Authentication Required:" COLOR_WHITE " %*s║\n", 85, "");
    print_separator();
    
    printf(COLOR_CYAN "\n  🔑 Enter admin password: " COLOR_RESET);
    while (getchar() != '\n');
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = '\0';
    
    if (strcmp(password, "admin123") != 0) {
        print_error_message("Invalid admin password.");
        printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
        getchar();
        return;
    }
    
    int choice;
    while (1) {
        clear_screen();
        print_header(COLOR_BRIGHT_WHITE "ADMIN CONTROL PANEL" COLOR_RESET, COLOR_BRIGHT_CYAN "Election Management System" COLOR_RESET);
        
        printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " ⚙️" COLOR_WHITE " Election Management:" COLOR_WHITE " %*s║\n", 85, "");
        print_separator();
        
        print_menu_item(1, "🗳️", "Open Voting", "Enable voter participation");
        print_menu_item(2, "🔒", "Close Voting", "Disable voting system");
        print_menu_item(3, "📊", "View Results", "Check election statistics");
        
        printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 📋" COLOR_WHITE " Position Management:" COLOR_WHITE " %*s║\n", 85, "");
        print_separator();
        
        print_menu_item(4, "➕", "Add Position", "Create new voting position");
        print_menu_item(5, "➖", "Remove Position", "Delete existing position");
        
        printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 👥" COLOR_WHITE " Candidate Management:" COLOR_WHITE " %*s║\n", 85, "");
        print_separator();
        
        print_menu_item(6, "�", "Add Candidate", "Register new candidate");
        print_menu_item(7, "❌", "Remove Candidate", "Delete existing candidate");
        
        printf(COLOR_WHITE "║" COLOR_BRIGHT_YELLOW " 🏠" COLOR_WHITE " Navigation:" COLOR_WHITE " %*s║\n", 85, "");
        print_separator();
        
        print_menu_item(8, "🏠", "Return to Main Menu", "Go back to main interface");
        print_menu_item(9, "❌", "Exit Admin Panel", "Close admin session");
        
        printf(COLOR_WHITE "║" COLOR_CYAN " %*s║\n", 118, "");
        print_footer();
        
        printf(COLOR_BRIGHT_YELLOW "\n  → Enter your choice (1-9): " COLOR_RESET);
        
        if (scanf("%d", &choice) != 1) {
            print_error_message("Invalid input. Please enter a number.");
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                terminal_open_voting();
                break;
            case 2:
                terminal_close_voting();
                break;
            case 3:
                terminal_view_results();
                break;
            case 4:
                terminal_add_position();
                break;
            case 5:
                terminal_remove_position();
                break;
            case 6:
                terminal_add_candidate();
                break;
            case 7:
                terminal_remove_candidate();
                break;
            case 8:
                return;
            case 9:
                print_info_message("Exiting admin panel...");
                exit(0);
            default:
                print_error_message("Invalid choice. Please try again.");
                printf(COLOR_CYAN "\n  Press Enter to continue..." COLOR_RESET);
                while (getchar() != '\n');
                getchar();
        }
    }
}

void terminal_add_position(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                     ADD POSITION                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    char name[100];
    printf("\nEnter position name: ");
    while (getchar() != '\n');
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    
    if (strlen(name) == 0) {
        printf("Position name cannot be empty.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    if (pos_add(name) == SUCCESS) {
        printf("\n✅ Position added successfully!\n");
    } else {
        printf("\n❌ Failed to add position.\n");
    }
    
    printf("Press Enter to continue...");
    getchar();
}

void terminal_add_candidate(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                    ADD CANDIDATE                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    char name[100];
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    
    if (pos_get_all(positions, &pos_count) != SUCCESS || pos_count == 0) {
        printf("\n❌ No positions available. Please add positions first.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("\nAvailable positions:\n");
    for (int i = 0; i < pos_count; i++) {
        printf("%d. %s\n", i + 1, positions[i].name);
    }
    
    int pos_choice;
    printf("\nSelect position (1-%d): ", pos_count);
    while (getchar() != '\n');
    if (scanf("%d", &pos_choice) != 1 || pos_choice < 1 || pos_choice > pos_count) {
        printf("Invalid position selection.\n");
        printf("Press Enter to continue...");
        while (getchar() != '\n');
        getchar();
        return;
    }
    
    printf("Enter candidate name: ");
    while (getchar() != '\n');
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = '\0';
    
    if (strlen(name) == 0) {
        printf("Candidate name cannot be empty.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    if (cand_register(name, positions[pos_choice - 1].id) == SUCCESS) {
        printf("\n✅ Candidate added successfully!\n");
    } else {
        printf("\n❌ Failed to add candidate.\n");
    }
    
    printf("Press Enter to continue...");
    getchar();
}

void terminal_remove_position(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                   REMOVE POSITION                             ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    Position positions[MAX_POSITIONS];
    int pos_count = 0;
    
    if (pos_get_all(positions, &pos_count) != SUCCESS || pos_count == 0) {
        printf("\n❌ No positions available.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("\nAvailable positions:\n");
    for (int i = 0; i < pos_count; i++) {
        printf("%d. %s\n", i + 1, positions[i].name);
    }
    
    int pos_choice;
    printf("\nSelect position to remove (1-%d): ", pos_count);
    while (getchar() != '\n');
    if (scanf("%d", &pos_choice) != 1 || pos_choice < 1 || pos_choice > pos_count) {
        printf("Invalid position selection.\n");
        printf("Press Enter to continue...");
        while (getchar() != '\n');
        getchar();
        return;
    }
    
    printf("\n⚠️  Warning: This will remove the position and all associated candidates.\n");
    printf("Confirm removal? (y/n): ");
    char confirm;
    scanf(" %c", &confirm);
    
    if (confirm == 'y' || confirm == 'Y') {
        if (pos_delete(positions[pos_choice - 1].id) == SUCCESS) {
            printf("\n✅ Position removed successfully!\n");
        } else {
            printf("\n❌ Failed to remove position.\n");
        }
    } else {
        printf("\nOperation cancelled.\n");
    }
    
    printf("Press Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

void terminal_remove_candidate(void) {
    clear_screen();
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                  REMOVE CANDIDATE                            ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    Candidate candidates[MAX_CANDIDATES];
    int cand_count = 0;
    
    if (cand_get_all(candidates, &cand_count) != SUCCESS || cand_count == 0) {
        printf("\n❌ No candidates available.\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("\nAvailable candidates:\n");
    for (int i = 0; i < cand_count; i++) {
        Position pos;
        pos_get_by_id(candidates[i].position_id, &pos);
        printf("%d. %s (Position: %s)\n", i + 1, candidates[i].name, pos.name);
    }
    
    int cand_choice;
    printf("\nSelect candidate to remove (1-%d): ", cand_count);
    while (getchar() != '\n');
    if (scanf("%d", &cand_choice) != 1 || cand_choice < 1 || cand_choice > cand_count) {
        printf("Invalid candidate selection.\n");
        printf("Press Enter to continue...");
        while (getchar() != '\n');
        getchar();
        return;
    }
    
    printf("\n⚠️  Warning: This will remove the candidate permanently.\n");
    printf("Confirm removal? (y/n): ");
    char confirm;
    scanf(" %c", &confirm);
    
    if (confirm == 'y' || confirm == 'Y') {
        if (cand_delete(candidates[cand_choice - 1].id) == SUCCESS) {
            printf("\n✅ Candidate removed successfully!\n");
        } else {
            printf("\n❌ Failed to remove candidate.\n");
        }
    } else {
        printf("\nOperation cancelled.\n");
    }
    
    printf("Press Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

void terminal_open_voting(void) {
    FILE *fp = fopen(STATUS_FILE, "w");
    if (fp) {
        fprintf(fp, "open");
        fclose(fp);
        printf("\n✅ Voting opened successfully!\n");
    } else {
        printf("\n❌ Failed to open voting.\n");
    }
    printf("Press Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

void terminal_close_voting(void) {
    FILE *fp = fopen(STATUS_FILE, "w");
    if (fp) {
        fprintf(fp, "closed");
        fclose(fp);
        printf("\n✅ Voting closed successfully!\n");
    } else {
        printf("\n❌ Failed to close voting.\n");
    }
    printf("Press Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

int main(int argc, char *argv[]) {
    /* Check if we should run in terminal mode or show startup menu */
    if (argc > 1 && strcmp(argv[1], "--terminal") == 0) {
        /* Initialize system for terminal mode */
        if (fh_init_files() != SUCCESS) {
            printf("Fatal Error: Could not initialize data files\n");
            return 1;
        }
        run_terminal_mode();
        return 0;
    }
    
    /* Show startup menu to choose interface */
    int choice;
    while (1) {
        show_startup_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                /* GUI Mode */
                gtk_init(&argc, &argv);
                break;
            case 2:
                /* Terminal Mode */
                if (fh_init_files() != SUCCESS) {
                    printf("Fatal Error: Could not initialize data files\n");
                    return 1;
                }
                run_terminal_mode();
                return 0;
            case 3:
                printf("Thank you for using SONU Voting System. Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
                while (getchar() != '\n');
        }
        
        if (choice == 1) break; /* Exit loop when GUI is chosen */
    }
    
    /* GUI Mode Initialization */
    /* Initialize system */
    if (fh_init_files() != SUCCESS) {
        show_message_dialog("Fatal Error", "Could not initialize data files", GTK_MESSAGE_ERROR);
        return 1;
    }
    
    /* Create main window */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "SONU Voting System");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 900, 650);
    gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    /* Create header bar */
    header_bar = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "SONU Voting System");
    
    back_button = gtk_button_new_with_label("Back to Login");
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_to_login), NULL);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), back_button);
    
    /* Create main stack for different pages */
    main_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(main_stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    
    /* Create pages */
    create_login_page();
    create_admin_page();
    create_voting_page();
    create_results_page();
    
    /* Add pages to stack */
    gtk_stack_add_titled(GTK_STACK(main_stack), login_grid, "login", "Login");
    gtk_stack_add_titled(GTK_STACK(main_stack), admin_grid, "admin", "Admin");
    gtk_stack_add_titled(GTK_STACK(main_stack), voting_grid, "voting", "Voting");
    gtk_stack_add_titled(GTK_STACK(main_stack), results_grid, "results", "Results");
    
    /* Set up window */
    gtk_window_set_titlebar(GTK_WINDOW(main_window), header_bar);
    gtk_container_add(GTK_CONTAINER(main_window), main_stack);
    
    /* Apply styling */
    apply_css_styling();
    
    /* Show everything */
    gtk_widget_show_all(main_window);
    
    /* Start with login page */
    gtk_stack_set_visible_child_name(GTK_STACK(main_stack), "login");
    gtk_widget_set_visible(back_button, FALSE);
    
    /* Show/hide back button based on current page */
    g_signal_connect(main_stack, "notify::visible-child", 
                   G_CALLBACK(gtk_widget_set_visible), back_button);
    
    gtk_main();
    return 0;
}
