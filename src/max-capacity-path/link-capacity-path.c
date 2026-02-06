#include <glib/gi18n.h>
#include <stdio.h>
#include "link-capacity-path.h"
#include "satellite-history.h"
#include "transfer-heap.h"
#include "transfer-time.h"

gboolean catnr_equal(gconstpointer a, gconstpointer b);
void tdsp_node_from_table(GArray *tdsp_array, GHashTable *table, tdsp_type type);

/**
 * Prints path of satellite transfers that can carry maximum amount of data from
 * first to last savoid.
 * 
 * Runs binary search on size of data it transfers to find max limit for which 
 * a path exists in the time window given.Find a path for a given fixed size by
 * running modified time-dependent Dijkstra.
 */
void get_max_link_path(
    GHashTable *sats, 
    GHashTable *sat_history,
    guint sat_hist_len,
    GHashTable *ground_stations,
    double t_start, 
    double t_end, 
    double time_step) {

    //GHashtable sats has {key:value} = {gint Catnr : sat_t *satellite}
    
    //print_GHashTable_key_value(sat_history, t_start, t_end, time_step);
    printf("start time %f\n", t_start);

    GArray *nodes = g_array_new(FALSE, TRUE, sizeof(tdsp_node));
    tdsp_node_from_table(nodes, sats, tdsp_SATELLITE);
    tdsp_node_from_table(nodes, ground_stations, tdsp_STATION);

    gdouble low = 0;
    gdouble high = 25000000;
    gdouble mid = -1;

    GList *attempt;

    gdouble end_time = -1;

    while (low <= high) {
        mid = (low + high) / 2.0;

        printf("trying for data size %f kilobytes\n", mid);
        attempt = TDSP_fixed_size(nodes, sat_history, get_transfer_time, sat_hist_len, mid, 39466, 27607, t_start, t_end, time_step);
        
        if (attempt != NULL) {
            printf("    found path\n");
            for (GList *S = attempt; S != NULL; S=S->next) {
                printf("        time: %f, catnr: %i, prev: %p\n", ((tdsp_node *)S->data)->time, ((tdsp_node *)S->data)->id, (void *)((tdsp_node *)S->data)->prev_node);
                end_time = ((tdsp_node *)S->data)->time;
            }    
            
            low = mid + 1;
            g_list_free_full(attempt, free);

        } else if (attempt == NULL) {
            printf("    didn't find path\n");
            high = mid - 1;
        }
    } 
   
    if (mid != -1) {
        printf("FOUND MAX CAPACITY TRANSFER (over %f minutes): %f (gigabytes)\n", (end_time - t_start) * 1440, mid / 1000);
    }

    g_array_free(nodes, TRUE);
}

void tdsp_node_from_table(GArray *tdsp_array, GHashTable *table, tdsp_type type) {
    GList *list = g_hash_table_get_keys(table);

    for (GList *current = list; current != NULL; current = current->next) {
        tdsp_node node = {
            .prev_node = NULL,
            .id = *(gint *)current->data,
            .time = G_MAXDOUBLE,
            .type = type
        };

        g_array_append_val(tdsp_array, node);
    }

    g_list_free(list);
}

// Time-dependent shortest path for a data amount of fixed size. Modified Dijkstra
GList *TDSP_fixed_size(
    GArray *const_tdsp_array,
    GHashTable *sat_history,
    gdouble (*time_func)(gint *, gint *, gdouble, GHashTable *, gint, gdouble, gdouble, gdouble, gdouble),
    gint hist_len,
    gdouble data_size,
    gint start_node,
    gint end_node,
    gdouble t_start,
    gdouble t_end,
    gdouble time_step) {
    
    //priority queue implemented with binary heap
    heap_tfr *S = (heap_tfr *)calloc(1, sizeof(heap_tfr));

    //best arrival time so far is stored in this copy
    GArray *tdsp_array = g_array_copy(const_tdsp_array);

    guint *end_node_index = NULL;

    //initialize distance to all satellites to infinity, except for start node
    for (guint i = 0; i < tdsp_array->len; i++) {
        tdsp_node *n = &g_array_index(tdsp_array, tdsp_node, i);

        //time at start node = start time, all other nodes = infinity
        if (n->id == start_node) {
            n->time = t_start;
        } else if (n->id == end_node) {
            end_node_index = malloc(sizeof(guint));
            *end_node_index = i;
        }

        push_tfr(S, n->time, &n->id, n);
    }

    //end node not included in tdsp_array
    if (end_node_index == NULL) {
        free(S);
        g_array_free(tdsp_array, TRUE);
        return NULL;
    }
    
    node_tfr *min = malloc(sizeof(node_tfr));
    //main dijkstra loop
    while (S->len != 0) {
        pop_tfr(S, min);

        //ToDo: you cannot set min to NULL, fix it inside pop_tfr as well
        if (min == NULL) {
            return NULL;
        }

        //found shortest path to result
        if (*min->cat_nr == end_node) break;

        //ToDo: double check this with unit testing
        //shortes time == G_MAXDOUBLE means won't find any more valid paths
        if (min->time == G_MAXDOUBLE) break;

        for (guint i = 0; i < tdsp_array->len; i++) {
            tdsp_node *node = &g_array_index(tdsp_array, tdsp_node, i);

            if (node->id == *min->cat_nr) {
                continue;
            }

            if (node->id == start_node) {
                continue;
            } 
           
            //get_transfer_time() in file transfer_time.c
            gdouble transfer_time = (*time_func)(min->cat_nr, &node->id, data_size, sat_history, hist_len, min->time, t_start, t_end, time_step);
            
            if (transfer_time < node->time) {
                node->time = transfer_time;
                node->prev_node = min->node;
                push_tfr(S, node->time, &node->id, node);
            }
        }
    }

    GList *path = NULL;
    tdsp_node *end_best = &g_array_index(tdsp_array, tdsp_node, *end_node_index);
    tdsp_node *copy_node;

    //found path
    if (end_best->time != G_MAXDOUBLE) {
        path = calloc(1, sizeof(GList)); 

        copy_node = malloc(sizeof(tdsp_node));
        memcpy(copy_node, end_best, sizeof(tdsp_node));
        path->data = copy_node;
       
        //if found path, only start node should have -1 along path
        for (tdsp_node *b=end_best->prev_node; b != NULL; b = b->prev_node) {

            copy_node = malloc(sizeof(tdsp_node));
            memcpy(copy_node, b, sizeof(tdsp_node));
            path = g_list_append(path, copy_node);
        }

        path = g_list_reverse(path);
    }
    
    free(S);
    free(min);
    free(end_node_index);
    g_array_free(tdsp_array, TRUE);

    return path;
}

gboolean catnr_equal(gconstpointer a, gconstpointer b) {
    return *(gint *)a == *(gint *)b;
}