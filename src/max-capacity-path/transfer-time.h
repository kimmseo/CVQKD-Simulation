#include <glib/gi18n.h>

gdouble get_transfer_time(
        gint *src, 
        gint *dst, 
        gdouble data_size, 
        GHashTable *sat_history,
        gint history_len,
        gdouble time,
        gdouble t_start, 
        gdouble t_end,
        gdouble time_step);