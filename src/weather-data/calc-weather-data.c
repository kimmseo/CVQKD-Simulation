#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <netcdf.h>

#include "../sat-log.h"
#include "../qth-data.h"
#include "calc-weather-data.h"

/**
 * ToDo (func to create):
 * 1 - Load data from file (load days accessible from file and ensure correct format)
 * 2 - take a ground station (lat lon), time period => return visibility and C_n^2 vals table for time
 * 3 - modify skr stuff to take it into account 
 */

#define FILE_NAME "/home/skadaddle/code/data/cloud-data/divergence_stream-oper_daily-mean.nc"

/* Handle errors by printing an error message and exiting with a
 * non-zero status. */

#define N_ValidTime 1
#define N_PressureLevel 1
#define N_Latitude 721
#define N_Longitude 1440

#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); return 2;}

int get_file_info(int ncid){
    int retval; 

    int num_dim;
    int num_vars;
    int num_attr;
    int id_unlimited_dim;
    if ((retval = nc_inq(ncid, &num_dim, &num_vars, &num_attr, &id_unlimited_dim)))
        ERR(retval);

    printf("num_dim: %i, num_vars: %i, num_attr: %i, exist unlim dim: %s\n",
        num_dim, num_vars, num_attr, (id_unlimited_dim == -1 ? "none" : "atleast 1")); 

    //Dimensions
    char *name = calloc(1, sizeof(char) * NC_MAX_NAME);
    size_t len;
    printf("dimensions: \n");
    for (int i = 0; i < num_dim; i++) {
        if ((retval = nc_inq_dim(ncid, i, name, &len)))
            ERR(retval);

        printf("\tname: %s  \tlength: %li\n", name, len);
    }

    //Global Attributes


    //Variables
    int num_dimensions = 0;
    int array_dim_ids[NC_MAX_VAR_DIMS];
    int number_attributes;
    nc_type var_type;
    printf("variables: \n");
    for (int i = 0; i < num_vars; i++) {
        if ((retval = nc_inq_var(ncid, i, name, &var_type, &num_dimensions, array_dim_ids, &number_attributes)))
            ERR(retval);
            
        printf("\tname: %s,\t num_attributes: %i, \tvar type: %i, \tnumber_dimensions: %i\n", 
            name, number_attributes, var_type, num_dimensions);

        for (int j = 0; j < num_dimensions; j++) {
            printf("\t\t dimension id: %i\n", array_dim_ids[j]);
        }

        for (int j = 0; j < number_attributes; j++) {
            //writes to name variable
            if ((retval = nc_inq_attname(ncid, i, j, name)))
                ERR(retval);

            //reads from name variable
            if ((retval = nc_inq_att(ncid, i, name, &var_type, &len)))
                ERR(retval);
            
            printf("\t\t attribute name: %s, \tvar type: %i, \tlen: %li\n",
                name, var_type, len);
        }
    }

    return retval;
}

//get small subset of values
int get_a_value(int ncid) {
    int retval;

    //get varids of latitude and longitude coordinate
    int lat_varid, lon_varid;

    if ((retval = nc_inq_varid(ncid, "latitude", &lat_varid)))
        ERR(retval);

    if ((retval = nc_inq_varid(ncid, "longitude", &lon_varid)))
        ERR(retval);

    //get varid of divergence, the one we want info from
    int diver_varid;
    if ((retval = nc_inq_varid(ncid, "d", &diver_varid)))
        ERR(retval);

    printf("lat: %i, lon: %i, diver: %i\n", lat_varid, lon_varid, diver_varid);

    float value[N_PressureLevel][N_ValidTime][N_Latitude][N_Longitude];
    size_t startp[4];
    size_t countp[4] = {1, 1, N_Latitude, N_Longitude};
    if ((retval = nc_get_vara_float(ncid, diver_varid, startp, countp, &value[0][0][0][0])))
        ERR(retval);

    for (int i_Lat = 0; i_Lat < N_Latitude / 50; i_Lat++) {
        for (int i_Lon = 0; i_Lon < N_Longitude / 50; i_Lon++) {
            /*
            printf("value at Lat: %i Lon: %i: %e\n", i_Lat, i_Lon, 
                value[0][0][i_Lat][i_Lon]);
            */

            printf("%e, ", value[0][0][i_Lat][i_Lon]);
        }
    }

    return 0;
}

/**
 * file name to get data from
 *      don't use ncid. keep opening and closing file in close proximity, self contained
 * ground station (lat, long)
 * time period (start time, end time) in hours to match data
 *      get average rms of wind in vertical direction. 
 *      Otherwise its an extra dimension for no reason
 * 
 * stations within same 1 degree lat lon square will have same data
 *      can't optimize everything. Complicated scheme would be troublesome
 */
GHashTable *load_turbulence_data(
        gchar *visibility_filepath,
        gchar *cn2_filepath,
        GSList *OGS_list,
        gdouble start_time,
        gdouble end_time) {
    int retval = 0;
    int  vis_id, cn2_id;

    if ((retval = nc_open(visibility_filepath, NC_NOWRITE, &vis_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to open %s with nc_open, returned %d"), 
            __func__, visibility_filepath, retval);
        return NULL;
    }

    if ((retval = nc_open(cn2_filepath, NC_NOWRITE, &cn2_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to open %s with nc_open, returned %d"), 
            __func__, cn2_filepath, retval);
        return NULL;
    }

    int vis_varid, cn2_varid_u, cn2_varid_v;

    if ((retval = nc_inq_varid(vis_id, "vis", &vis_varid))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load visibility variable id, returned %d"), 
            __func__, retval);
        return NULL;
    }

    if ((retval = nc_inq_varid(cn2_id, "u", &cn2_varid_u))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load u-component wind variable id, returned %d"), 
            __func__, retval);
        return NULL;
    }

    if ((retval = nc_inq_varid(cn2_id, "v", &cn2_varid_v))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load v-component wind variable id, returned %d"), 
            __func__, retval);
        return NULL;
    }

    //figure out time. hours only. usually only searching 24 hours.
    int start_hour = (int)start_time;
    int end_hour = (int)end_time;

    int time_diff = end_hour - start_hour;

    //load visibility data
    //if (retval = nc_get_vara_double(vis_id, vis_id, ));

    GHashTable *table = g_hash_table_new(g_str_hash, g_str_equal);
    for (GSList *ogs_elm = OGS_list; ogs_elm != NULL; ogs_elm = ogs_elm->next) {
        qth_t *ogs = (qth_t *)ogs_elm->data;

        ogs_weather_data *entries = malloc(sizeof(ogs_weather_data));

        gdouble *vis_data = malloc(time_diff * sizeof(gdouble));

        //pull value for this ogs (lat, lon) in giant chunk,
        // then iterate through to calculate

        for (int time = 0; time < end_hour - start_hour; time++) {

            //get visibility values
            //nc_get_vara_double()
            vis_data[time] = 1.0;

            //get cn2 values
            //nc_get_vara_float()
        }

        entries->vis = vis_data;
        entries->len = time_diff;
        g_hash_table_insert(table, &ogs->name, entries);
    }

    return table;
}