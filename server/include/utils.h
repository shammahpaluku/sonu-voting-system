#ifndef UTILS_H
#define UTILS_H

int   utils_get_int(const char *prompt, int min, int max);
void  utils_get_string(const char *prompt, char *buf, int buf_len);
void  utils_get_password(const char *prompt, char *buf, int buf_len);
void  utils_trim(char *str);
void  utils_to_lowercase(char *str);
int   utils_is_alpha(const char *str);
void  utils_hash_password(const char *plain, char *out, int out_len);
void  utils_clear_screen(void);
void  utils_print_separator(void);
void  utils_pause(void);

#endif // UTILS_H
