#include "sat-graph.h"
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

graph_t *graph_new(void) {
    graph_t *g = g_new(graph_t, 1);
    g->adj_list = g_hash_table_new(g_direct_hash, g_direct_equal);
    return g;
}

void graph_add_vertex(graph_t *g, sat_t *sat) {
    if (!g_hash_table_contains(g->adj_list, sat)) {
        g_hash_table_insert(g->adj_list, sat, NULL);
    }
}

void graph_add_edge(graph_t *g, sat_t *a, sat_t *b, double weight) {
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

void graph_free(graph_t *g) {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, g->adj_list);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        GList *edges = value;
        g_list_free_full(edges, g_free); // Free edge_t*
    }

    g_hash_table_destroy(g->adj_list);
    g_free(g);
}

void graph_print(graph_t *g) {
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, g->adj_list);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        sat_t *sat = (sat_t *)key;
        GList *edges = (GList *)value;

        g_print("Satellite: %s\n", sat->name);
        for (GList *l = edges; l != NULL; l = l->next) {
            edge_t *e = l->data;
            g_print("  -> %s (weight: %.2f)\n", e->target->name, e->weight);
        }
    }
}