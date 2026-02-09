#include <glib/gi18n.h>
#include "../sgpsdp/sgp4sdp4.h"

GHashTable *generate_sat_pos_data(GList *sats_list, guint *sat_hist_len, gdouble start_time, gdouble end_time, gdouble time_step);