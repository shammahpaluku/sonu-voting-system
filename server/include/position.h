#ifndef POSITION_H
#define POSITION_H

int  pos_add(const char *name);
int  pos_get_all(Position *positions, int max);
int  pos_get_by_id(int id, Position *out);
int  pos_validate_id(int id);
int  pos_next_id(void);
void pos_display_all(void);

#endif // POSITION_H
