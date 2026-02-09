#include <glib/gi18n.h>
#include "path-util.h"

gdouble get_transfer_time(
        tdsp_node *src, 
        tdsp_node *dst, 
        gdouble data_size, 
        GHashTable *sat_history,
        gint history_len,
        gdouble time,
        gdouble t_start, 
        gdouble t_end,
        gdouble time_step);