#include <glib/gi18n.h>

typedef struct {
    gdouble cumulative;
    gdouble last;
    gboolean last_even;
} simpson_intg;

void generate_link_capacities(GHashTable *sats, gdouble link_cap_limit, double t_start, double t_end, double time_step);