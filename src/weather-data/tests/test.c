#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "../../time-tools.h"
#include "../calc-weather-data.h"

int main() {
    /*
    int retval, ncid;

    // Open the file.
    //if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
    //    ERR(retval);

    
    //if ((retval = get_file_info(ncid)))
    //    return retval;
    

    if ((retval = get_a_value(ncid)))
        return retval; 
    */

    char *vis_filepath = "/home/skadaddle/code/data/singapore/data_sfc.nc";
    char *cn2_filepath = "/home/skadaddle/code/data/singapore/data_plev.nc";
    
    GSList *list = NULL;
    qth_t ogs = {.name="ogs1", .alt=100, .lat=0, .lon=0};
    list = g_slist_append(list, &ogs);

    gdouble start_time = 2461157.898843; // 27/4/2026 around 17:35
    gdouble end_time = 2461159.898843;   // + 2 days
   
    ogs_weather_data *table = load_weather_from_file(vis_filepath, cn2_filepath, start_time, end_time);
    gint t_length = sizeof(*table);

    if (table == NULL) {
        printf("ERROR occured: returned NULL\n");
        return 0;
    }

    for(gint i = 0; i < t_length; i++) {
        printf("hour: %i, vis: %f, rms: %f\n", i, table[i].vis, table[i].rms_wind);
    }
    
    return 0;
}

