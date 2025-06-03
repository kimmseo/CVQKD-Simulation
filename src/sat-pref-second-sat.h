#ifndef SAT_PREF_SECOND_SAT_H
#define SAT_PREF_SECOND_SAT_H 1

GtkWidget      *sat_pref_second_sat_create(GKeyFile * cfg);
void            sat_pref_second_sat_cancel(GKeyFile * cfg);
void            sat_pref_second_sat_ok(GKeyFile * cfg);

#endif
