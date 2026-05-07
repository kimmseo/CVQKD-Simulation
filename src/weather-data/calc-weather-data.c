#include <stdlib.h>
#include <stdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <netcdf.h>

#include "calc-weather-data.h"
#include "../sat-log.h"
#include "../qth-data.h"
#include "../time-tools.h"


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

#define Num_Plvls 10
#define A0 pow(10, -13)
#define rms 21

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

//gives back reference time of netcdf file in astronomical julian date format.
//return value (gboolean) if error occured or not;
gboolean get_reference_time(gint cd_id, gdouble *return_val) {
    gint retval = 0;
    gint time_ref_id;
    if ((retval = nc_inq_varid(cd_id, "forecast_reference_time", &time_ref_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load dataset start time variable id, returned %d"), 
            __func__, retval);
        return TRUE;
    }

    size_t index_p[4] = {0, 0, 0, 0};
    long long int ref_time_sec;
    if ((retval = nc_get_var1_longlong(cd_id, time_ref_id, index_p, &ref_time_sec))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load data set start time, returned %d"), 
            __func__, retval);
        return TRUE;
    }
    //reference time -> number of seconds since 1970-01-01 in gregorian
    gdouble ref_time_days = ref_time_sec / 86400.0;
    *return_val = ref_time_days + 2440587.5;

    return FALSE;
}

gboolean setup_nc_files(
        gchar *visibility_filepath, gchar *cn2_filepath,
        gint *vis_id, gint *cn2_id, 
        gint *vis_varid, gint *cn2_varid_u, gint *cn2_varid_v) {

    gint retval;
    
    if ((retval = nc_open(visibility_filepath, NC_NOWRITE, vis_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to open %s with nc_open, returned %d"), 
            __func__, visibility_filepath, retval);
        return TRUE;
    }

    if ((retval = nc_open(cn2_filepath, NC_NOWRITE, cn2_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to open %s with nc_open, returned %d"), 
            __func__, cn2_filepath, retval);
        return TRUE;
    }

    if ((retval = nc_inq_varid(*vis_id, "vis", vis_varid))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load visibility variable id, returned %d"), 
            __func__, retval);
        return TRUE;
    }

    if ((retval = nc_inq_varid(*cn2_id, "u", cn2_varid_u))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load u-component wind variable id, returned %d"), 
            __func__, retval);
        return TRUE;
    }

    if ((retval = nc_inq_varid(*cn2_id, "v", cn2_varid_v))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load v-component wind variable id, returned %d"), 
            __func__, retval);
        return TRUE;
    }

    return FALSE;
}

void close_nc_file(gint vis_id, gint cn2_id) {
    int retval;
    if ((retval = nc_close(vis_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to close visibility file, returned %d"), 
            __func__, retval);
    }

    if ((retval = nc_close(cn2_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to close cn2 file, returned %d"), 
            __func__, retval);
    } 
}

void read_hour_data(gchar *filepath, gchar *var_name) {
    int retval; 
    int file_id;
    int var_id;

    if ((retval = nc_open(filepath, NC_NOWRITE, &file_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to open %s with nc_open, returned %d"), 
            __func__, filepath, retval);
        return;
    }

    if ((retval = nc_inq_varid(file_id, var_name, &var_id))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR, 
            _("%s: Failed to load visibility variable id, returned %d"), 
            __func__, retval);
        return;
    }

    //{forecast_period, forecast_reference_time, latitude, longitude}
    //start index for each dimension
    size_t vis_startp[1] = {0};
    //edge length in each dimension of data values to read
    const int dimension = 41;
    size_t vis_countp[1] =  {dimension};
    //array to load the data into
    double vis_values[dimension];

    if ((retval = nc_get_vara_double(file_id, var_id, vis_startp, vis_countp, &vis_values[0]))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
        _("%s: Failed to load visibility data from file %s, returned %d"),
        __func__, filepath, retval);
        return;
    }

    for (int i = 0; i < dimension; i++) {
        printf("\tread value: %f\n", vis_values[i]);
    }
}

gdouble calc_rms(float u_val[], float v_val[]) {
    gdouble sum = 0; 
    
    for (gint i = 0; i < Num_Plvls; i++) {
        sum += pow(u_val[i], 2) + pow(v_val[i], 2);
    }

    return sqrt( sum / Num_Plvls);
}

/**
 * file name to get data from
 *      don't use ncid. keep opening and closing file in close proximity, self contained
 * ground station (lat, long)
 * time period (start time, end time) in hours to match data
 *      get average rms of wind in vertical direction. 
 *      Otherwise its an extra dimension for no reason
 */
ogs_weather_data *load_turbulence_data(
        gchar *visibility_filepath,
        gchar *cn2_filepath,
        gdouble start_time,
        gdouble end_time
    ) {
    gint retval;
    gint  vis_id, cn2_id;
    gint vis_varid, cn2_varid_u, cn2_varid_v;

    if (setup_nc_files(visibility_filepath, cn2_filepath, &vis_id, &cn2_id, 
                        &vis_varid, &cn2_varid_u, &cn2_varid_v)) {
        return NULL;
    }

    //ToDo: add safety check
    gdouble ref_time;
    get_reference_time(vis_id, &ref_time);

    //convert to hours
    //maybe somechecks that falls within range of values provided by file
    gint start_index = (gint)((start_time - ref_time) * 24);
    gint end_index = (gint)((end_time - ref_time) * 24);
    printf("start_index: %i, end_index; %i\n", start_index, end_index);

    gint time_diff = end_index - start_index;

    char ref_time_str[20];
    daynum_to_str(ref_time_str, 20, "%d/%m/%G - %H:%M\n", ref_time);
    printf("reference time: %s", ref_time_str);

    ogs_weather_data *entries = malloc(time_diff * sizeof(ogs_weather_data));

    // === Visibility ===
    //{forecast_period, forecast_reference_time, latitude, longitude}
    //start index for each dimension
    size_t vis_startp[4] = {0, 0, 0, 0};
    //edge length in each dimension of data values to read
    size_t vis_countp[4] = {time_diff, 1, 1, 1};
    //array to load the data into
    float vis_values[time_diff];

    if ((retval = nc_get_vara_float(vis_id, vis_varid, vis_startp, vis_countp, vis_values))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
        _("%s: Failed to load visibility data from file %s, returned %d"),
        __func__, visibility_filepath, retval);
        return NULL;
    } 

    // === CN2 ===
    //{forecast_period, forecast_reference_time, pressure_level, latitude, longitude}
    gint diff_divided = (int)(time_diff / 3);

    printf("time diff: %i, diff_divided: %i\n", time_diff, diff_divided);

    size_t cn2_startp[5] = {0, 0, 0, 0, 0};
    size_t cn2_countp[5] = {diff_divided, 1, Num_Plvls, 1, 1};

    float *uwind_values = malloc(sizeof(float) * diff_divided * Num_Plvls);
    float *vwind_values = malloc(sizeof(float) * diff_divided * Num_Plvls);

    if ((retval = nc_get_vara_float(cn2_id, cn2_varid_u, cn2_startp, cn2_countp, uwind_values))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
        _("%s: Failed to load u-wind data from file %s, returned %d"),
        __func__, cn2_filepath, retval);
        return NULL;
    }

    if ((retval = nc_get_vara_float(cn2_id, cn2_varid_v, cn2_startp, cn2_countp, vwind_values))) {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
        _("%s: Failed to load v-wind data from file %s, returned %d"),
        __func__, cn2_filepath, retval);
        return NULL;
    }

    gdouble rms_value = 0;
    // === Transfer to output === 
    for (int index = 0; index < time_diff; index++) {

        //get visibility values
        entries[index].vis = vis_values[index];

        //get cn2 values
        if (index % 3 == 0) {
            rms_value = calc_rms(&uwind_values[index * Num_Plvls], &vwind_values[index * Num_Plvls]);
        }

        entries[index].rms_wind = rms_value;
    }

    close_nc_file(vis_id, cn2_id);

    return entries;
}