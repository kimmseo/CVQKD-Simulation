#ifndef __SKR_UTILS_H__
#define __SKR_UTILS_H__

#include <glib.h>
#include "sgpsdp/sgp4sdp4.h"
#include "gtk-sat-data.h"
#include "qth-data.h"

// Functions to calculate and return SKR
gdouble fibre_link(qth_t ground1, qth_t ground2);
gdouble underwater_link(qth_t station1, qth_t station2);
gdouble ground_to_sat_uplink(qth_t ground, sat_t sat);
gdouble sat_to_ground_downlink(sat_t sat, qth_t ground);
gdouble inter_sat_link(sat_t sat1, sat_t sat2);

#endif
