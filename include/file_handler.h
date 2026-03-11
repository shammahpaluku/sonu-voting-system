#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H
#include "config.h"

int  fh_init_files(void);
int  fh_file_exists(const char *path);
int  fh_count_records(const char *path);
int  fh_read_all(const char *path, char lines[][MAX_LINE_LEN], int *count);
int  fh_write_all(const char *path, char lines[][MAX_LINE_LEN], int count);
int  fh_append_record(const char *path, const char *record);
int  fh_update_record(const char *path, int id_field, const char *id_val,
                      const char *new_record);
int  fh_delete_record(const char *path, int id_field, const char *id_val);

#endif /* FILE_HANDLER_H */
