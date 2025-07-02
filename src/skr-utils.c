/*
 * This file contains the necessary utility functions for
 * calculating the SKR for different links.
 * This file should be included wherever SKR calculation
 * is required. 
*/

#include <glib.h>
#include <math.h>
#include "qth-data.h"
#include "gtk-sat-data.h"
#include "sgpsdp/sgp4sdp4.h"
#include "skr-utils.h"


/*
    Function for calculating the SKR for ground-to-ground link
    ground1 = First ground station
    ground2 = Second ground station
*/
gdouble fibre_link(qth_t *ground1, qth_t *ground2)
{
    gdouble skr;
    // Do SKR calculation here
    // temporary placeholder
    skr = 0.0;
    return skr;
}

/*
gdouble underwater_link(qth_t *station1, qth_t *station2)
{
    gdouble skr;
    // Do SKR calculation here
    // temporary placeholder
    skr = 0.0;
    return skr;
}
*/

/*
    Function for calculating the SKR for ground-to-sat uplink
    ground = Ground station from which the link starts from
    sat = target satellite
*/
gdouble ground_to_sat_uplink(qth_t *ground, sat_t *sat)
{
    gdouble skr;
    // Do SKR calculation here
    // temporary placeholder
    skr = 0.0;
    return skr;
}

/*
    Function for calculating the SKR for sat-to-ground downlink
    sat = origin satellite
    ground = target ground station
*/
gdouble sat_to_ground_downlink(sat_t *sat, qth_t *ground)
{
    gdouble skr;
    // Do SKR calculation here
    // temporary placeholder
    skr = 0.0;
    return skr;
}

/*
    Function for calculating the SKR for inter-satellite link
    sat1 = origin satellite
    sat2 = target satellite
*/
gdouble inter_sat_link(sat_t *sat1, sat_t *sat2)
{
    gdouble skr;
    // Do SKR calculation here
    // temporary placeholder
    skr = 0.0;
    return skr;
}