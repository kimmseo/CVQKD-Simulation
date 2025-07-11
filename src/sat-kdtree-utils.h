#ifndef __SAT_KDTREE_UTILS_H__
#define __SAT_KDTREE_UTILS_H__

#include <glib.h>
#include "kdtree-wrapper.h"
#include "sgpsdp/sgp4sdp4.h"

/**
 * Create a 3D kd-tree for satellites.
 */
Kdtree *sat_kdtree_create(void);

/**
 * Insert a satellite into the kd-tree.
 */
void sat_kdtree_insert(Kdtree *tree, sat_t *sat);

/**
 * Find the nearest satellite to a given satellite (excluding itself).
 */
sat_t *sat_kdtree_find_nearest_other(Kdtree *tree, const sat_t *query_sat);

#endif // SATELLITE_KDTREE_UTILS_H
