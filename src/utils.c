#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include "config.h"
#include "utils.h"

int utils_get_int(const char *prompt, int min, int max) {
    char buf[32];
    int val;
    while (1) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) continue;
        if (sscanf(buf, "%d", &val) != 1) {
            printf("Enter a number between %d and %d: ", min, max);
            continue;
        }
        if (val >= min && val <= max) return val;
        printf("Out of range (%d-%d): ", min, max);
    }
}

void utils_get_string(const char *prompt, char *buf, int max_len) {
    printf("%s", prompt);
    fgets(buf, max_len, stdin);
    buf[strcspn(buf, "\n")] = '\0';
    utils_trim(buf);
}

void utils_get_password(const char *prompt, char *buf, int max_len) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO;            /* disable character echo */
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    printf("%s", prompt);
    fgets(buf, max_len, stdin);
    buf[strcspn(buf, "\n")] = '\0';
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);  /* restore echo */
    printf("\n");
}

void utils_hash_password(const char *plain, char *out, int out_len) {
    unsigned long hash = 5381;
    int c;
    const char *p = plain;
    while ((c = *p++)) hash = ((hash << 5) + hash) + c;  /* hash*33 + c */
    snprintf(out, out_len, "%lu", hash);
}

void utils_trim(char *s) {
    int start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;
    if (start > 0) memmove(s, s+start, strlen(s)-start+1);
    int end = (int)strlen(s) - 1;
    while (end >= 0 && (s[end]==' ' || s[end]=='\t')) end--;
    s[end+1] = '\0';
}

void utils_to_lowercase(char *s) {
    for (int i = 0; s[i]; i++) {
        s[i] = tolower(s[i]);
    }
}

int utils_is_alpha(const char *s) {
    for (int i = 0; s[i]; i++) {
        if (!isalpha(s[i]) && s[i] != ' ') {
            return 0;
        }
    }
    return 1;
}

void utils_clear_screen(void) {
    printf("\033[2J\033[H");
}

void utils_print_separator(char c, int n) {
    for (int i = 0; i < n; i++) {
        putchar(c);
    }
    putchar('\n');
}

void utils_pause(void) {
    printf("Press Enter to continue...");
    getchar();
}
