#include <glib/gi18n.h>
#include "../sgpsdp/sgp4sdp4.h"

#ifndef PATH_UTIL_H
#define PATH_UTIL_H

typedef enum {
    path_STATION,
    path_SATELLITE
} path_type;

typedef struct path_node{
    gdouble time;           //earliest arrival so far
    gint id;                //for satellites id is catnr, for ground stations, assigned negative number ids
    path_type type;
    void *obj;              //points to original objects, for satellites sat_t, for gound stations qth_t
} path_node;

typedef struct tdsp_node {
    struct tdsp_node *prev_node;
    path_node node;
} tdsp_node;              //wrapper type keeps track of node with best path to it
                    //including prev_node into path_node is troublesome 
                    //when returning result (GList of copies)

/**
 * \brief Light weight satellite data structure
 * for max link capacity calculation
 */
typedef struct {
    vector_t pos;
    vector_t vel;
    gdouble jul_utc;
} lw_sat_t;

#endif