/*
    Utility functions to calculate distance between two satellites
    Positions of each satellite are given by sat->pos
    sat->pos is vector of [x, y, z] obtained from SGP4 SDP4 algorithms
    Find distance between two points in a sphere where locations are known
*/


// have not been included in makefile, include when ready

#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif


#include <glib.h>
#include <glib/gi18n.h>

#include <math.h>

#include "gtk-sat-data.h"
#include "orbit-tools.h"
#include "calc-dist-two-sat.h"
#include "sat-cfg.h"
#include "sat-log.h"
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"


// Driver function for calculating distance between two satellites
// Euclidean distance - need verification
// If not correct, will need to use Haversine formula
gdouble dist_calc_driver (gdouble sat1_posx, gdouble sat1_posy, gdouble sat1_posz,
                          gdouble sat2_posx, gdouble sat2_posy, gdouble sat2_posz)
{
    gdouble dist;

    dist = sqrt( pow((sat2_posx - sat1_posx), 2)
                + pow((sat2_posy - sat1_posy), 2)
                + pow((sat2_posz - sat1_posz), 2) );
    
    return dist;
}

// Warpper function for dist_calc_driver()
gdouble dist_calc (sat_t *sat1, sat_t *sat2)
{
    gdouble sat1_posx, sat1_posy, sat1_posz;
    gdouble sat2_posx, sat2_posy, sat2_posz;

    sat1_posx = sat1->pos.x;
    sat1_posy = sat1->pos.y;
    sat1_posz = sat1->pos.z;

    sat2_posx = sat2->pos.x;
    sat2_posy = sat2->pos.y;
    sat2_posz = sat2->pos.z;

    return dist_calc_driver(sat1_posx, sat1_posy, sat1_posz,
                            sat2_posx, sat2_posy, sat2_posz);
    
    // Use below if Haversine formula is needed instead of Euclidean distance
    /*
    gdouble sat1_lat, sat1_lon, sat2_lat, sat2_lon;

    sat1_lat = sat1->ssplat;
    sat1_lon = sat1->ssplon;
    sat2_lat = sat2->ssplat;
    sat2_lon = sat2->ssplon;

    return haversine_dist_calc_driver(sat1_lat, sat1_lon, sat2_lat, sat2_lon);
    */
}


// Haversine forumla
// Use if Euclidean distance is not correct
gdouble haversine_dist_calc_driver(gdouble sat1_lat, gdouble sat1_lon,
                                   gdouble sat2_lat, gdouble sat2_lon)
{
    gdouble dist;

    // distance betwen lat and lon
    gdouble dLat = (sat2_lat - sat1_lat) * PI / 180.0;
    gdouble dLon = (sat2_lon - sat1_lon) * PI / 180.0;

    // convert to rad
    sat1_lat = (sat1_lat) * PI / 180.0;
    sat2_lat = (sat2_lat) * PI / 180.0;

    // apply the formula
    gdouble a = pow (sin(dLat / 2), 2) +
                pow (sin(dLon / 2), 2) +
                cos(sat1_lat) * cos(sat2_lat);   
    gdouble c = 2 * asin(sqrt(a));
    dist = EARTH_RADIUS * c;

    return dist;
}

// Helper functions
void compute_magnitude(vector_t *v)
{
    v->w = sqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

vector_t cross_product(const vector_t *a, const vector_t *b)
{
    vector_t result;
    result.x = a->y * b->z - a->z * b->y;
    result.y = a->z * b->x - a->x * b->z;
    result.z = a->x * b->y - a->y * b->x;
    compute_magnitude(&result);
    return result;
}

gdouble dot_product(const vector_t *a, const vector_t *b)
{
    double result;
    result = (a->x * b->x) + (a->y * b->y) + (a->z * b->z);
    return result;
}

// Check if the straight line between two satellites collide with Earth
// We assume radius of Earth + 20km of atmosphere is the zone the laser
// cannot pass through.
// Return value true means los is clear (sat to sat link can be established)
// Return value false means los is not clear (sat to sat link blocked by Earth)
gboolean is_los_clear(sat_t *sat1, sat_t *sat2)
{
    vector_t sat1_vector = {sat1->pos.x, sat1->pos.y, sat1->pos.z, sat1->pos.w};
    vector_t sat2_vector = {sat2->pos.x, sat2->pos.y, sat2->pos.z, sat2->pos.w};

    vector_t a_to_b;
    a_to_b.x = sat2_vector.x - sat1_vector.x;
    a_to_b.y = sat2_vector.y - sat1_vector.y;
    a_to_b.z = sat2_vector.z - sat1_vector.z;
    a_to_b.w = 0.0;

    gdouble a = dot_product(&a_to_b, &a_to_b);
    gdouble b = 2 * dot_product(&sat1_vector, &a_to_b);
    gdouble c = dot_product(&sat1_vector, &sat1_vector) -
                (EARTH_RADIUS + 20) * (EARTH_RADIUS + 20);

    gdouble discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
    {
        return TRUE; // No intersection with Earth
    }
    else
    {
        double sqrt_disc = sqrt(discriminant);
        double t1 = (-b - sqrt_disc) / (2 * a);
        double t2 = (-b + sqrt_disc) / (2 * a);

        if ((t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0))
            return FALSE; // LOS blocked
        else
            return TRUE; // LOS clear
    }
}