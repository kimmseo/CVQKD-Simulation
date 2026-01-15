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
    gdouble start_time;
} simpson_intg;

void get_all_link_capacities(
    GHashTable *sats, 
    gdouble link_cap_limit, 
    gdouble t_start, 
    gdouble t_end, 
    gdouble time_step);