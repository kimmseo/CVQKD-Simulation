#include <glib/gi18n.h>

/**
 * Accumulator for Simpson Integration.
 * To be iniline with the mathematical view of this formula, it starts counting
 * at 0. So the first input would be last = y_0 and last_even = true, 
 * then last = y_1 and last_even = false. 
 */
typedef struct {
    gdouble cumulative;
    gdouble last;
    gboolean last_even;
} simpson_intg;

typedef struct {
    gdouble time;       //earliest arrival so far
    gint prev;          //cat number of previous node
} tdsp_node;

void get_max_link_path(GHashTable *sats, gdouble link_cap_limit, double t_start, double t_end, double time_step);