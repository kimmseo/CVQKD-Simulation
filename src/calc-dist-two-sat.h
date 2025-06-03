
// have not been included in makefile, include when ready

#ifndef CALC_DIST_TWO_SAT_H
#define CALC_DIST_TWO_SAT_H 1

#include <glib.h>
#include "gtk-sat-data.h"
#include "sgpsdp/sgp4sdp4.h"

#include <math.h>

/* Constant Declaration */
#define EARTH_RADIUS        6378.137            /* Equatorial Radius */
#define EARTH_RADIUS_POLAR  6356.752            /* Polar Radius */
#define PI                  3.141592653589793   /* Pi */

/* SGP4/SDP4 driver */
// Wrapper function, will pass sat1's pos [x, y, z] and sat2's [x, y, z]
// details to dist_calc_driver()
gdouble dist_calc (sat_t *sat1, sat_t *sat2);
gboolean is_los_clear (sat_t *sat1, sat_t *sat2)



#endif