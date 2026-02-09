#include <glib/gi18n.h>
#include "path-util.h"

void get_max_link_path(
    GHashTable *sats, 
    GHashTable *sat_history,
    guint sat_hist_len,
    GHashTable *ground_stations,
    double t_start, 
    double t_end, 
    double time_step);

GList *TDSP_fixed_size(
    GArray *const_tdsp_array,
    GHashTable *sat_history,
    gdouble (*time_func)(tdsp_node *, tdsp_node *, gdouble, GHashTable *, gint, gdouble, gdouble, gdouble, gdouble),
    gint hist_len,
    gdouble data_size,
    gint start_node,
    gint end_node,
    gdouble t_start,
    gdouble t_end,
    gdouble time_step);