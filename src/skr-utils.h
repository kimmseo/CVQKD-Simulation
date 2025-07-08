/*
 * skr-utils.h - Secret Key Rate (SKR) calculation utility functions.
 *
 * This header file declares functions for calculating the Secret Key Rate (SKR)
 * for various quantum communication links, including fiber, satellite-to-ground,
 * and inter-satellite links.
 */

#ifndef __SKR_UTILS_H__
#define __SKR_UTILS_H__

#include <glib.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-sat-data.h"
#include "qth-data.h"

/*
 * fibre_link() - Calculates the SKR for a fiber link between two ground stations.
 * @ground1: Pointer to the first ground station data structure.
 * @ground2: Pointer to the second ground station data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble fibre_link(qth_t *ground1, qth_t *ground2);

/*
 * ground_to_sat_uplink() - Calculates the SKR for a ground-to-satellite uplink.
 * @ground: Pointer to the ground station data structure.
 * @sat: Pointer to the satellite data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble ground_to_sat_uplink(qth_t *ground, sat_t *sat);

/*
 * sat_to_ground_downlink() - Calculates SKR for a satellite-to-ground downlink.
 * @sat: Pointer to the satellite data structure.
 * @ground: Pointer to the ground station data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble sat_to_ground_downlink(sat_t *sat, qth_t *ground);

/*
 * inter_sat_link() - Calculates the SKR for an inter-satellite link.
 * @sat1: Pointer to the first satellite data structure.
 * @sat2: Pointer to the second satellite data structure.
 *
 * Return: The calculated SKR in bits per second.
 */
gdouble inter_sat_link(sat_t *sat1, sat_t *sat2);

//gdouble underwater_link(qth_t *station1, qth_t *station2);

#endif /* __SKR_UTILS_H__ */
