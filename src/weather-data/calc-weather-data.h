#include <glib/gi18n.h>
#include "../qth-data.h"

typedef struct {
    gdouble rms_wind;
    gdouble vis;
} ogs_weather_data;

ogs_weather_data *load_weather_from_file(
        gchar *visibility_filepath,
        gchar *cn2_filepath,
        gdouble start_time,
        gdouble end_time
    );

void read_hour_data(gchar *filepath, gchar *var_name);