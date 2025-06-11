#ifndef SAT_PREF_MULTIPLE_SAT_H
#define SAT_PREF_MULTIPLE_SAT_H 1

GtkWidget       *sat_pref_multiple_sat_create(GKeyFile * cfg);
void            sat_pref_multiple_sat_cancel(GKeyFile * cfg);
void            sat_pref_multiple_sat_ok(GKeyFile * cfg);


#endif