#include <glib/gi18n.h>

typedef struct {
    gdouble time;       //earliest arrival so far
    gint current;
    gint prev;          //cat number of previous node
} tdsp_node;


void get_max_link_path(GHashTable *sats, GHashTable *sat_history, guint sat_hist_len, double t_start, double t_end, double time_step);

GList *TDSP_fixed_size(
    GHashTable *sats,
    GHashTable *sat_history,
    gdouble (*time_func)(gint *, gint *, gdouble, GHashTable *, gint, gdouble, gdouble, gdouble, gdouble),
    gint hist_len,
    gdouble data_size,
    gint start_node,
    gint end_node,
    gdouble t_start,
    gdouble t_end,
    gdouble time_step);