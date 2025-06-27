#include <math.h>
#include "sat-kdtree-utils.h"
#include "kdtree-wrapper.h"
#include "sgpsdp/sgp4sdp4.h"


/* Usage example
Kdtree *tree = sat_kdtree_create();
sat_kdtree_insert_all(tree, satellites, num_sats);

sat_t *target = satellites[0];
sat_t *nearest = sat_kdtree_find_nearest_other(tree, target);

if (nearest) {
    g_print("Closest to %s is %s\n", target->name, nearest->name);
} else {
    g_print("No other satellite found.\n");
}

kdtree_free(tree);
*/

// Assume vector_t has x/y/z as double
struct _sat_t {
    char *name;
    vector_t pos;
    // ... other fields ...
};

Kdtree *sat_kdtree_create(void) {
    return kdtree_new(3);
}

void sat_kdtree_insert(Kdtree *tree, sat_t *sat) {
    kdtree_insert3(tree, sat->pos.x, sat->pos.y, sat->pos.z, sat);
}

void sat_kdtree_insert_all(Kdtree *tree, sat_t **sats, gsize count) {
    for (gsize i = 0; i < count; ++i) {
        sat_kdtree_insert(tree, sats[i]);
    }
}

sat_t *sat_kdtree_find_nearest_other(Kdtree *tree, const sat_t *query_sat) {
    gdouble x = query_sat->pos.x;
    gdouble y = query_sat->pos.y;
    gdouble z = query_sat->pos.z;

    struct kdres *res = kd_nearest_range3(tree->tree, x, y, z, 1e6); // use large range

    sat_t *nearest = NULL;
    gdouble min_dist = G_MAXDOUBLE;

    while (!kd_res_end(res)) {
        gdouble px, py, pz;
        sat_t *candidate = (sat_t *)kd_res_item3(res, &px, &py, &pz);
        if (candidate != query_sat) {
            gdouble dx = px - x;
            gdouble dy = py - y;
            gdouble dz = pz - z;
            gdouble dist = sqrt(dx * dx + dy * dy + dz * dz);
            if (dist < min_dist) {
                min_dist = dist;
                nearest = candidate;
            }
        }
        kd_res_next(res);
    }

    kd_res_free(res);
    return nearest;
}
