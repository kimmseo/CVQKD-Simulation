/*
    Utility functions to calculate distance between two satellites
    Positions of each satellite are given by sat->pos
    sat->pos is vector of [x, y, z] obtained from SGP4 SDP4 algorithms
    Find distance between two points in a sphere where locations are known
*/


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
#include "qth-data.h"


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
// This is done by checking whether the straight line between them intersects
// the Earth
gboolean is_los_clear(sat_t *sat1, sat_t *sat2)
{
    gdouble a, b, c, discriminant;
    vector_t A, B, d;
    
    A.x = sat1->pos.x;
    A.y = sat1->pos.y;
    A.z = sat1->pos.z;

    B.x = sat2->pos.x;
    B.y = sat2->pos.y;
    B.z = sat2->pos.z;

    d.x = B.x - A.x;
    d.y = B.y - A.y;
    d.z = B.z - A.z;

    a = dot_product(&d, &d);
    b = 2 * dot_product(&A, &d);
    c = dot_product(&A, &A) - (EARTH_RADIUS + 20)*(EARTH_RADIUS + 20);

    discriminant = b*b - 4*a*c;

    if (discriminant < 0)
        return TRUE; // no intersection: LOS is clear

    // intersection points
    gdouble t1 = (-b - sqrt(discriminant)) / (2*a);
    gdouble t2 = (-b + sqrt(discriminant)) / (2*a);

    if ((t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0))
    {
        // One of the intersections lies between sat1 and sat2
        return FALSE; // LOS is blocked
    }
    else
    {
        return TRUE; // LOS is clear
    }
}

// same as is_los_clear() but vector_t pointers as input
gboolean is_pos_los_clear(vector_t *sat1, vector_t *sat2)
{
    gdouble a, b, c, discriminant;
    vector_t A, B, d;
    
    A.x = sat1->x;
    A.y = sat1->y;
    A.z = sat1->z;

    B.x = sat2->x;
    B.y = sat2->y;
    B.z = sat2->z;

    d.x = B.x - A.x;
    d.y = B.y - A.y;
    d.z = B.z - A.z;

    a = dot_product(&d, &d);
    b = 2 * dot_product(&A, &d);
    c = dot_product(&A, &A) - (EARTH_RADIUS + 20)*(EARTH_RADIUS + 20);

    discriminant = b*b - 4*a*c;

    if (discriminant < 0)
        return TRUE; // no intersection: LOS is clear

    // intersection points
    gdouble t1 = (-b - sqrt(discriminant)) / (2*a);
    gdouble t2 = (-b + sqrt(discriminant)) / (2*a);

    if ((t1 >= 0.0 && t1 <= 1.0) || (t2 >= 0.0 && t2 <= 1.0))
    {
        // One of the intersections lies between sat1 and sat2
        return FALSE; // LOS is blocked
    }
    else
    {
        return TRUE; // LOS is clear
    }
}

gdouble sat_qth_distance(sat_t *sat, qth_t *qth)
{
    if (!sat || !qth)
        return DBL_MAX;

    // Extract satellite's sub-point and altitude
    gdouble sat_lat_rad = DEG_TO_RAD(sat->ssplat);
    gdouble sat_lon_rad = DEG_TO_RAD(sat->ssplon);
    gdouble sat_alt_km = sat->alt;

    // Ground station position
    gdouble qth_lat_rad = DEG_TO_RAD(qth->lat);
    gdouble qth_lon_rad = DEG_TO_RAD(qth->lon);
    gdouble qth_alt_km = qth->alt / 1000.0;

    // Earth radius in km
    const gdouble R = 6371.0;
    gdouble r1 = R + sat_alt_km;
    gdouble r2 = R + qth_alt_km;

    // Convert to 3D Cartesian coordinates
    gdouble x1 = r1 * cos(sat_lat_rad) * cos(sat_lon_rad);
    gdouble y1 = r1 * cos(sat_lat_rad) * sin(sat_lon_rad);
    gdouble z1 = r1 * sin(sat_lat_rad);

    gdouble x2 = r2 * cos(qth_lat_rad) * cos(qth_lon_rad);
    gdouble y2 = r2 * cos(qth_lat_rad) * sin(qth_lon_rad);
    gdouble z2 = r2 * sin(qth_lat_rad);

    // Euclidean distance in 3D space
    gdouble dx = x1 - x2;
    gdouble dy = y1 - y2;
    gdouble dz = z1 - z2;

    return sqrt(dx * dx + dy * dy + dz * dz); // in kilometers
}
