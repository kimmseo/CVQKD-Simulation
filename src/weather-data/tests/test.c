#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "../../qth-data.h"
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
    char *vis_filepath = "/home/skadaddle/code/data/vis-cn2/data_sfc.nc";
    char *cn2_filepath = "/home/skadaddle/code/data/vis-cn2/data_plev.nc";
    
    GSList *list = NULL;
    qth_t ogs1 = {.name="ogs1", .alt=100, .lat=0, .lon=0};
    qth_t ogs2 = {.name="ogs2", .alt=0, .lat=20, .lon=20};
    qth_t ogs3 = {.name="ogs3", .alt=25, .lat=-30, .lon=-10};
    list = g_slist_append(list, &ogs1);
    list = g_slist_append(list, &ogs2);
    list = g_slist_append(list, &ogs3);
    
    GHashTable *table = load_turbulence_data(vis_filepath, cn2_filepath, list, 0, 12);

    if (table == NULL) {
        printf("ERROR occured: returned NULL\n");
        return 0;
    }
    
    GList *keys = g_hash_table_get_keys(table);
    for(GList *k = keys; k != NULL; k = k->next) {
        ogs_weather_data *data = (ogs_weather_data *)g_hash_table_lookup(table, k->data);
        printf("ogs: %s\n", ((qth_t *)k->data)->name);

        for (size_t i = 0; i < data->len; i++) {
            printf("\tvis: %f\n", data->vis[i]);
        }
    }
    
    return 0;
}

