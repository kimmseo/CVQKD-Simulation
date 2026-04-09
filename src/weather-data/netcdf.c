#include <stdlib.h>
#include <stdio.h>
#include <netcdf.h>

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

//get value of last row
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

int main() {
    int retval, ncid;

    /* Open the file. */
    if ((retval = nc_open(FILE_NAME, NC_NOWRITE, &ncid)))
        ERR(retval);

    /*
    if ((retval = get_file_info(ncid)))
        return retval;
    */

    if ((retval = get_a_value(ncid)))
        return retval; 

}