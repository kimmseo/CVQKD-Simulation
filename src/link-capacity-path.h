#include <glib/gi18n.h>

typedef struct {
    gdouble time;       //earliest arrival so far
    gint prev;          //cat number of previous node
} tdsp_node;


void get_max_link_path(GHashTable *sats, double t_start, double t_end, double time_step);