#include <glib/gi18n.h>
#include <stdio.h>
#include "link-capacity-path.h"
#include "satellite-history.h"
#include "transfer-heap.h"
#include "transfer-time.h"

gboolean catnr_equal(gconstpointer a, gconstpointer b);

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
    double t_start, 
    double t_end, 
    double time_step) {

    //GHashtable sats has {key:value} = {gint Catnr : sat_t *satellite}
    
    //print_GHashTable_key_value(sat_history, t_start, t_end, time_step);
    printf("start time %f\n", t_start);

    gdouble low = 0;
    gdouble high = 25000000;
    gdouble mid = -1;

    GList *attempt;

    gdouble end_time = -1;

    //within 0.1 kb of right answer. 
    while (low <= high) {
        mid = (low + high) / 2.0;

        printf("trying for data size %f kilobytes\n", mid);
        attempt = TDSP_fixed_size(sats, sat_history, get_transfer_time, sat_hist_len, mid, 39466, 27607, t_start, t_end, time_step);
        
        if (attempt != NULL) {
            printf("    found path\n");
            for (GList *S = attempt; S != NULL; S=S->next) {
                printf("        time: %f, catnr: %i, prev: %i\n", ((tdsp_node *)S->data)->time, ((tdsp_node *)S->data)->current, ((tdsp_node *)S->data)->prev);
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
}


// Time-dependent shortest path for a data amount of fixed size. Modified Dijkstra
GList *TDSP_fixed_size(
    GHashTable *sats,                   //--
    GHashTable *sat_history,
    gdouble (*time_func)(gint *, gint *, gdouble, GHashTable *, gint, gdouble, gdouble, gdouble, gdouble),
    gint hist_len,
    gdouble data_size,
    gint start_node,                    //--
    gint end_node,                      //--
    gdouble t_start,                    //--
    gdouble t_end,
    gdouble time_step) {
    
    //best arrival times so far
    //{key : value} = {gint catnr : tdsp_node i}
    GHashTable *best = g_hash_table_new_full(g_int_hash, catnr_equal, NULL, free);        

    //priority queue implemented with binary heap
    heap_tfr *S = (heap_tfr *)calloc(1, sizeof(heap_tfr));

    //initialize distance to all satellites to infinity, except for start node
    GList *cats_list = g_hash_table_get_keys(sats); 
    for (GList *sat_cat = cats_list; sat_cat != NULL; sat_cat = sat_cat->next) {

        gint *catnr = (gint *)sat_cat->data;

        tdsp_node *i = malloc(sizeof(tdsp_node));
        i->prev = -1;
        i->current = *catnr;
        i->time = G_MAXDOUBLE;

        //time at start node = start time, all other nodes = infinity
        if (*catnr == start_node) i->time = t_start;

        g_hash_table_insert(best, catnr, i);        
        push_tfr(S, i->time, catnr);
    } 
    
    node_tfr *min = malloc(sizeof(node_tfr));
    //main dijkstra loop
    while (S->len != 0) {
        pop_tfr(S, min);

        if (min == NULL) {
            return NULL;
        }

        //found shortest path to result
        if (*min->cat_nr == end_node) break;

        //ToDo: double check this with unit testing
        if (min->time == G_MAXDOUBLE) break;

        for (GList *check = cats_list; check != NULL; check = check->next) {
            gint *i_catnr = (gint *)check->data;
            tdsp_node *i = (tdsp_node *)g_hash_table_lookup(best, i_catnr); 

            if (*i_catnr == *min->cat_nr) {
                continue;
            }

            if (*i_catnr == start_node) {
                continue;
            } 
            
            gdouble a_ij = (*time_func)(min->cat_nr, i_catnr, data_size, sat_history, hist_len, min->time, t_start, t_end, time_step);
            
            if (a_ij < i->time) {
                i->time = a_ij;
                i->prev = *min->cat_nr;
                push_tfr(S, i->time, i_catnr);
            }
        }
    }

    GList *path = NULL;
    tdsp_node *end_best = g_hash_table_lookup(best, &end_node);
    tdsp_node *copy_node;
   
    //found path
    if (end_best->time != G_MAXDOUBLE) {
        path = calloc(1, sizeof(GList)); 

        copy_node = malloc(sizeof(tdsp_node));
        memcpy(copy_node, end_best, sizeof(tdsp_node));
        path->data = copy_node;
       
        //if found path, only start node should have -1 along path
        for (tdsp_node *b=g_hash_table_lookup(best, &end_best->prev); b != NULL; b = g_hash_table_lookup(best, &b->prev)) {

            copy_node = malloc(sizeof(tdsp_node));
            memcpy(copy_node, b, sizeof(tdsp_node));
            path = g_list_append(path, copy_node);
        }

        path = g_list_reverse(path);
    }
    
    free(S);
    free(min);
    g_hash_table_destroy(best);

    return path;
}

gboolean catnr_equal(gconstpointer a, gconstpointer b) {
    return *(gint *)a == *(gint *)b;
}