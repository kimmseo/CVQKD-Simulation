#ifndef __SAT_GRAPH_H__
#define __SAT_GRAPH_H__

#include <glib.h>
#include "sgpsdp/sgp4sdp4.h"

// Edge between two satellites with a weight
typedef struct {
    sat_t *target;
    double weight;
} edge_t;

// Graph structure
typedef struct {
    // Key: sat_t*, Value: GList* of edge_t*
    GHashTable *adj_list;
} graph_t;

// Graph operations
graph_t *graph_new(void);
void graph_add_vertex(graph_t *g, sat_t *sat);
void graph_add_edge(graph_t *g, sat_t *a, sat_t *b, double weight);
void graph_free(graph_t *g);
void graph_print(graph_t *g);

#endif // SAT_GRAPH_H
