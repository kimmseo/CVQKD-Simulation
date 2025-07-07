#include "sat-graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

GHashTable* dijkstra(graph_t *g, sat_t *source);
GList* prim(graph_t *g, sat_t *start);
gint edge_weight_compare(gconstpointer a, gconstpointer b);

graph_t *graph_new(void)
{
    graph_t *g = g_new(graph_t, 1);
    g->adj_list = g_hash_table_new(g_direct_hash, g_direct_equal);
    return g;
}

void graph_add_vertex(graph_t *g, sat_t *sat)
{
    if (!g_hash_table_contains(g->adj_list, sat))
    {
        g_hash_table_insert(g->adj_list, sat, NULL);
    }
}

void graph_add_edge(graph_t *g, sat_t *a, sat_t *b, double weight)
{
    graph_add_vertex(g, a);
    graph_add_vertex(g, b);

    edge_t *edge_ab = g_new(edge_t, 1);
    edge_ab->target = b;
    edge_ab->weight = weight;

    edge_t *edge_ba = g_new(edge_t, 1);
    edge_ba->target = a;
    edge_ba->weight = weight;

    GList *list_a = g_hash_table_lookup(g->adj_list, a);
    list_a = g_list_prepend(list_a, edge_ab);
    g_hash_table_replace(g->adj_list, a, list_a);

    GList *list_b = g_hash_table_lookup(g->adj_list, b);
    list_b = g_list_prepend(list_b, edge_ba);
    g_hash_table_replace(g->adj_list, b, list_b);
}

void graph_free(graph_t *g)
{
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, g->adj_list);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        GList *edges = value;
        g_list_free_full(edges, g_free); // Free edge_t*
    }

    g_hash_table_destroy(g->adj_list);
    g_free(g);
}

void graph_print(graph_t *g)
{
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, g->adj_list);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        sat_t *sat = (sat_t *)key;
        GList *edges = (GList *)value;

        g_print("Satellite: %s\n", sat->name);
        for (GList *l = edges; l != NULL; l = l->next)
        {
            edge_t *e = l->data;
            g_print("  -> %s (weight: %.2f)\n", e->target->name, e->weight);
        }
    }
}


// Return value is GHashTable, must be freed by caller
GHashTable* dijkstra(graph_t *g, sat_t *source)
{
    GHashTable *distances = g_hash_table_new(g_direct_hash, g_direct_equal);
    GHashTable *visited = g_hash_table_new(g_direct_hash, g_direct_equal);
    GQueue *queue = g_queue_new();

    GHashTableIter iter;
    gpointer key, value;

    // Initialize distances to infinity
    g_hash_table_iter_init(&iter, g->adj_list);
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        g_hash_table_insert(distances, key, GDOUBLE_TO_POINTER(G_MAXDOUBLE));
    }

    g_hash_table_replace(distances, source, GDOUBLE_TO_POINTER(0.0));
    g_queue_push_tail(queue, source);

    while (!g_queue_is_empty(queue))
    {
        sat_t *u = g_queue_pop_head(queue);
        if (g_hash_table_contains(visited, u)) continue;

        g_hash_table_add(visited, u);
        double dist_u = GPOINTER_TO_DOUBLE(g_hash_table_lookup(distances, u));
        GList *edges = g_hash_table_lookup(g->adj_list, u);

        for (GList *l = edges; l != NULL; l = l->next)
        {
            edge_t *edge = (edge_t *)l->data;
            sat_t *v = edge->target;
            double alt = dist_u + edge->weight;
            double curr_dist = GPOINTER_TO_DOUBLE(g_hash_table_lookup(distances, v));
            if (alt < curr_dist)
            {
                g_hash_table_replace(distances, v, GDOUBLE_TO_POINTER(alt));
                g_queue_push_tail(queue, v);
            }
        }
    }

    g_hash_table_destroy(visited);
    g_queue_free(queue);
    return distances;
}


// Helpers for prim's algorithm
typedef struct {
    sat_t *from;
    edge_t *edge;
} pq_entry_t;

gint edge_weight_compare(gconstpointer a, gconstpointer b) {
    const pq_entry_t *entry_a = a;
    const pq_entry_t *entry_b = b;

    if (entry_a->edge->weight < entry_b->edge->weight)
        return -1;
    else if (entry_a->edge->weight > entry_b->edge->weight)
        return 1;
    else
        return 0;
}


// Prim's algorithm for MST, for multiple stations. Returns GList, must be
// freed by the caller
GList* prim(graph_t *g, sat_t *start)
{
    GHashTable *visited = g_hash_table_new(g_direct_hash, g_direct_equal);
    GList *mst_edges = NULL;

    GList *pq = NULL;

    g_hash_table_add(visited, start);
    GList *edges = g_hash_table_lookup(g->adj_list, start);
    for (GList *l = edges; l != NULL; l = l->next)
    {
        pq_entry_t *entry = g_new(pq_entry_t, 1);
        entry->from = start;
        entry->edge = l->data;
        pq = g_list_insert_sorted(pq, entry, edge_weight_compare);
    }

    while (pq != NULL)
    {
        pq_entry_t *min = pq->data;
        pq = g_list_delete_link(pq, pq);

        sat_t *v = min->edge->target;
        if (!g_hash_table_contains(visited, v))
        {
            g_hash_table_add(visited, v);
            // Add edge to MST
            mst_edges = g_list_prepend(mst_edges, min->edge);

            // Add edges of `v` to priority queue
            GList *v_edges = g_hash_table_lookup(g->adj_list, v);
            for (GList *l = v_edges; l != NULL; l = l->next)
            {
                edge_t *e = l->data;
                if (!g_hash_table_contains(visited, e->target))
                {
                    pq_entry_t *entry = g_new(pq_entry_t, 1);
                    entry->from = v;
                    entry->edge = e;
                    pq = g_list_insert_sorted(pq, entry, edge_weight_compare);
                }
            }
        }

        g_free(min);
    }

    g_hash_table_destroy(visited);
    return mst_edges;
}
