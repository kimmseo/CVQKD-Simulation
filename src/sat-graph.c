#include "sat-graph.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

static gchar* make_edge_key(sat_t *a, sat_t *b)
{
    // Use pointer values for uniqueness
    return g_strdup_printf("%p|%p", a < b ? a : b, a < b ? b : a);
}

SatGraph* sat_graph_new(void)
{
    SatGraph *graph = g_new0(SatGraph, 1);
    graph->vertices = g_hash_table_new(g_direct_hash, g_direct_equal);
    graph->edges = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    return graph;
}

void sat_graph_add_vertex(SatGraph *graph, sat_t *sat)
{
    if (!g_hash_table_contains(graph->vertices, sat))
    {
        g_hash_table_insert(graph->vertices, sat, NULL);
    }
}

void sat_graph_add_edge(SatGraph *graph, sat_t *sat1, sat_t *sat2)
{
    if (!sat1 || !sat2 || sat1 == sat2) return;

    gdouble *dist = g_new(gdouble, 1);
    *dist = dist_calc(sat1, sat2);
    gchar *key = make_edge_key(sat1, sat2);
    g_hash_table_insert(graph->edges, key, dist);

    GList *neighbors1 = g_hash_table_lookup(graph->vertices, sat1);
    GList *neighbors2 = g_hash_table_lookup(graph->vertices, sat2);

    neighbors1 = g_list_prepend(neighbors1, sat2);
    neighbors2 = g_list_prepend(neighbors2, sat1);

    g_hash_table_insert(graph->vertices, sat1, neighbors1);
    g_hash_table_insert(graph->vertices, sat2, neighbors2);
}

void sat_graph_free(SatGraph *graph)
{
    g_hash_table_destroy(graph->vertices);
    g_hash_table_destroy(graph->edges);
    g_free(graph);
}

// Dijkstra's Algorithm
GList* sat_graph_dijkstra(SatGraph *graph, sat_t *start, sat_t *end)
{
    GHashTable *dist = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *prev = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *visited = g_hash_table_new(g_direct_hash, g_direct_equal);
    GQueue *queue = g_queue_new();

    GList *keys = g_hash_table_get_keys(graph->vertices);
    for (GList *l = keys; l != NULL; l = l->next)
    {
        sat_t *sat = l->data;
        g_hash_table_insert(dist, sat, GDOUBLE_TO_POINTER(DBL_MAX));
        g_hash_table_insert(prev, sat, NULL);
    }
    g_list_free(keys);

    g_hash_table_replace(dist, start, GDOUBLE_TO_POINTER(0.0));
    g_queue_push_tail(queue, start);

    while (!g_queue_is_empty(queue))
    {
        // Find unvisited vertex with smallest distance
        sat_t *u = NULL;
        gdouble min_dist = DBL_MAX;
        for (GList *l = g_queue_peek_head_link(queue); l; l = l->next)
        {
            sat_t *s = l->data;
            if (!g_hash_table_contains(visited, s))
            {
                gdouble d = GPOINTER_TO_DOUBLE(g_hash_table_lookup(dist, s));
                if (d < min_dist)
                {
                    u = s;
                    min_dist = d;
                }
            }
        }

        if (!u) break;
        g_hash_table_insert(visited, u, GINT_TO_POINTER(TRUE));
        g_queue_remove(queue, u);

        GList *neighbors = g_hash_table_lookup(graph->vertices, u);
        for (GList *l = neighbors; l != NULL; l = l->next) {
            sat_t *v = l->data;
            if (g_hash_table_contains(visited, v)) continue;

            gchar *edge_key = make_edge_key(u, v);
            gdouble *w = g_hash_table_lookup(graph->edges, edge_key);
            g_free(edge_key);
            gdouble alt = GPOINTER_TO_DOUBLE(g_hash_table_lookup(dist, u)) + *w;

            if (alt < GPOINTER_TO_DOUBLE(g_hash_table_lookup(dist, v))) {
                g_hash_table_replace(dist, v, GDOUBLE_TO_POINTER(alt));
                g_hash_table_replace(prev, v, u);
                g_queue_push_tail(queue, v);
            }
        }
    }

    // Reconstruct path
    GList *path = NULL;
    for (sat_t *at = end; at != NULL; at = g_hash_table_lookup(prev, at))
    {
        path = g_list_prepend(path, at);
        if (at == start) break;
    }

    if (path == NULL || path->data != start)
    {
        g_list_free(path);
        // No path found
        path = NULL;
    }

    g_hash_table_destroy(dist);
    g_hash_table_destroy(prev);
    g_hash_table_destroy(visited);
    g_queue_free(queue);
    return path;
}

// Prim's Algorithm
SatGraph* sat_graph_prims(SatGraph *graph)
{
    SatGraph *mst = sat_graph_new();
    GHashTable *visited = g_hash_table_new(g_direct_hash, g_direct_equal);

    GList *vertices = g_hash_table_get_keys(graph->vertices);
    if (!vertices) return mst;

    sat_t *start = vertices->data;
    g_hash_table_insert(visited, start, GINT_TO_POINTER(TRUE));
    sat_graph_add_vertex(mst, start);

    while (g_hash_table_size(visited) < g_hash_table_size(graph->vertices))
    {
        gdouble min_weight = DBL_MAX;
        sat_t *from = NULL, *to = NULL;

        GList *visited_keys = g_hash_table_get_keys(visited);
        for (GList *l = visited_keys; l != NULL; l = l->next)
        {
            sat_t *u = l->data;
            GList *neighbors = g_hash_table_lookup(graph->vertices, u);
            for (GList *n = neighbors; n != NULL; n = n->next)
            {
                sat_t *v = n->data;
                if (g_hash_table_contains(visited, v)) continue;

                gchar *key = make_edge_key(u, v);
                gdouble *weight = g_hash_table_lookup(graph->edges, key);
                g_free(key);
                if (*weight < min_weight)
                {
                    min_weight = *weight;
                    from = u;
                    to = v;
                }
            }
        }
        g_list_free(visited_keys);

        if (to)
        {
            sat_graph_add_vertex(mst, to);
            sat_graph_add_edge(mst, from, to);
            g_hash_table_insert(visited, to, GINT_TO_POINTER(TRUE));
        } else
        {
            break;  // No more reachable vertices
        }
    }

    g_list_free(vertices);
    g_hash_table_destroy(visited);
    return mst;
}
