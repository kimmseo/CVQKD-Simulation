#include <glib/gi18n.h>

typedef struct {
    gdouble time;       //earliest arrival so far
    gint prev;          //cat number of previous node
} tdsp_node;


void get_max_link_path(GHashTable *sats, GHashTable *sat_history, guint sat_hist_len, double t_start, double t_end, double time_step);

gdouble get_transfer_time(gint *src, gint *dst, gdouble data_size, GHashTable *sat_history, gint history_len, gdouble time, gdouble t_start, gdouble t_end, gdouble time_step); 
