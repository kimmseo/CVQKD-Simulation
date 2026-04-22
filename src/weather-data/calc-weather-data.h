#include <glib/gi18n.h>
#include "../qth-data.h"

typedef struct {
    size_t len;
    gdouble *cn2;
    gdouble *vis;
} ogs_weather_data;

GHashTable *load_turbulence_data(
        gchar *visibility_filepath,
        gchar *cn2_filepath,
        GSList *OGS_list,
        gdouble start_time,
        gdouble end_time);