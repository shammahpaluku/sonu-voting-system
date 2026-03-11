#ifndef POSITION_H
#define POSITION_H
#include "config.h"

typedef struct {
    int  id;
    char name[MAX_NAME_LEN];
} Position;

int  pos_add(const char *name);
int  pos_delete(int id);
int  pos_get_by_id(int id, Position *out);
int  pos_get_all(Position out[], int *count);
int  pos_validate_id(int id);
int  pos_next_id(void);
void pos_display_all(void);

#endif /* POSITION_H */
