#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"

int utils_get_int(const char *prompt, int min, int max) {
    char buffer[MAX_LINE_LEN];
    int value;
    
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            continue;
        }
        
        if (sscanf(buffer, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            }
        }
        
        printf("Invalid input. Try again.\n");
    }
}

void utils_get_string(const char *prompt, char *buf, int buf_len) {
    printf("%s", prompt);
    fflush(stdout);
    
    if (fgets(buf, buf_len, stdin)) {
        // Strip trailing newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
        
        utils_trim(buf);
    }
}

void utils_get_password(const char *prompt, char *buf, int buf_len) {
    struct termios oldt, newt;
    
    printf("%s", prompt);
    fflush(stdout);
    
    // Disable terminal echo
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    
    if (fgets(buf, buf_len, stdin)) {
        // Strip trailing newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
        }
    }
    
    // Restore terminal echo
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    printf("\n"); // Print newline for clean terminal
}

void utils_trim(char *str) {
    if (!str || *str == '\0') {
        return;
    }
    
    // Remove leading whitespace
    char *start = str;
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    // Remove trailing whitespace
    char *end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    *(end + 1) = '\0';
}

void utils_to_lowercase(char *str) {
    if (!str) {
        return;
    }
    
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

int utils_is_alpha(const char *str) {
    if (!str || *str == '\0') {
        return 0;
    }
    
    for (int i = 0; str[i]; i++) {
        if (!(isalpha(str[i]) || isspace(str[i]))) {
            return 0;
        }
    }
    
    return 1;
}

void utils_hash_password(const char *plain, char *out, int out_len) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *plain++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    snprintf(out, out_len, "%lu", hash);
}

void utils_clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void utils_print_separator(void) {
    printf("------------------------------------------------------------\n");
    fflush(stdout);
}

void utils_pause(void) {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    getchar();
}
