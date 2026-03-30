#ifndef APPLICATION_H
#define APPLICATION_H

#include "config.h"

// Application management functions
int application_add(int voter_id, const char *voter_name, int position_id, const char *position_name);
int application_get_by_voter(int voter_id, CandidateApplication *app);
int application_get_all(CandidateApplication *apps, int max_count);
int application_get_pending(CandidateApplication *apps, int max_count);
int application_update_status(int app_id, int status);
int application_delete(int app_id);
void application_display(const CandidateApplication *app);

#endif // APPLICATION_H
