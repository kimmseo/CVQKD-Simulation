#ifndef __SAT_GRAPH_H__
#define __SAT_GRAPH_H__

#include <glib.h>
#include "sgpsdp/sgp4sdp4.h"
#include "calc-dist-two-sat.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

typedef struct {
    // Key: sat_t* (pointer), Value: GList* of neighbors (sat_t*)
    GHashTable *vertices;
    // Key: "sat1|sat2" string, Value: gdouble* (distance)
    GHashTable *edges;
} SatGraph;

// Graph functions
SatGraph* sat_graph_new(void);
void sat_graph_add_vertex(SatGraph *graph, sat_t *sat);
void sat_graph_add_edge(SatGraph *graph, sat_t *sat1, sat_t *sat2);
void sat_graph_free(SatGraph *graph);

// Shortest path (Dijkstra)
GList* sat_graph_dijkstra(SatGraph *graph, sat_t *start, sat_t *end);

// Minimum Spanning Tree (Prim)
SatGraph* sat_graph_prims(SatGraph *graph);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif // SAT_GRAPH_H
