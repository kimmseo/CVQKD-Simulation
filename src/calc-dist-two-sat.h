#ifndef __CALC_DIST_TWO_SAT_H__
#define __CALC_DIST_TWO_SAT_H__ 1

#include <glib.h>
#include "gtk-sat-data.h"
#include "sgpsdp/sgp4sdp4.h"
#include "qth-data.h"

#include <math.h>

/* Constant Declaration */
#define EARTH_RADIUS        6378.137            /* Equatorial Radius */
#define EARTH_RADIUS_POLAR  6356.752            /* Polar Radius */
#define PI                  3.141592653589793   /* Pi */
#define DEG_TO_RAD(x)       ((x) * G_PI / 180.0)

/* SGP4/SDP4 driver */
// Wrapper function, will pass sat1's pos [x, y, z] and sat2's [x, y, z]
// details to dist_calc_driver()
gdouble dist_calc (sat_t *sat1, sat_t *sat2);
gdouble dist_calc_driver (gdouble sat1_posx, gdouble sat1_posy, gdouble sat1_posz, gdouble sat2_posx, gdouble sat2_posy, gdouble sat2_posz);
gboolean is_los_clear (sat_t *sat1, sat_t *sat2);
gdouble sat_qth_distance(sat_t *sat, qth_t *qth);



#endif