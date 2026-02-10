#ifndef SAT_PREF_MAX_PATH_VIEW_H
#define SAT_PREF_MAX_PATH_VIEW_H 1

GtkWidget       *sat_pref_max_path_view_create(GKeyFile * cfg);
void            sat_pref_max_path_view_cancel(GKeyFile * cfg);
void            sat_pref_max_path_view_ok(GKeyFile * cfg);


#endif