#include <glib/gi18n.h>
#include <stdio.h>
#include "link-capacity-path.h"
#include "satellite-history.h"
#include "transfer-heap.h"
#include "transfer-time.h"

gboolean TDSP_fixed_size(GHashTable *sats, GHashTable *sat_history, gint hist_len, gdouble data_size, gint start_node, gint end_node, double t_start, double t_end, double time_step);
gboolean catnr_equal(gconstpointer a, gconstpointer b);
/**
 * Prints path of satellite transfers that can carry maximum amount of data from
 * first to last savoid.
 * 
 * Runs binary search on size of data it transfers to find max limit for which 
 * a path exists in the time window given.Find a path for a given fixed size by
 * running modified time-dependent Dijkstra.
 * 
 * ToDo:
 *  - Testing with data
 *  - Implement ground stations
 *  - Create user UI for interacting with code 
 *  - Clean up code, keep same style as rest of project.
 *      - clean up names (keep it consistent)
 *      - group things together, reduce clutter in parameters 
 *          -t_start, t_end, time_step into time_window struct
 *          -hist_len and sat_history object? 
 *          -sat_history with time_windows struct
 *          -combine pre lerp code with its wrapper?
 *      - Write about modified algorithm in code comments. Mention paper and DOI
 *      - add input checking and error handling
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
    gdouble high = 80000;   //80000 bits = 10 kilobytes
    gdouble mid = -1;

    //within 0.1 kb of right answer. 
    while (low <= high) {
        mid = (low + high) / 2.0;

        printf("trying for data size %f kilobytes\n", mid);
        gboolean attempt = TDSP_fixed_size(sats, sat_history, sat_hist_len, mid, 39466, 27607, t_start, t_end, time_step);
        
        if (!attempt) {
            high = mid - 1;
        } else if (attempt) {
            low = mid + 1;
        }
    } 
   
    if (mid != -1) {
        printf("FOUND MAX CAPACITY TRANSFER: %f (kilobytes)\n", mid);
    }
}


// Time-dependent shortest path for a data amount of fixed size. Modified Dijkstra
gboolean TDSP_fixed_size(
    GHashTable *sats, 
    GHashTable *sat_history,
    gint hist_len,
    gdouble data_size,
    gint start_node,
    gint end_node,
    double t_start,
    double t_end,
    double time_step) {
    
    gboolean final_answer = FALSE;

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
        i->time = G_MAXDOUBLE;

        //time at start node = start time, all other nodes = infinity
        if (*catnr == start_node) {
            i->time = t_start;
        }

        g_hash_table_insert(best, catnr, i);        
        push_tfr(S, i->time, catnr);
    } 
    
    node_tfr *min = malloc(sizeof(node_tfr));
    //main dijkstra loop
    while (S->len != 0) {
        pop_tfr(S, min);

        if (min == NULL) {
            return final_answer;
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
            
            gdouble a_ij = get_transfer_time(min->cat_nr, i_catnr, data_size, sat_history, hist_len, min->time, t_start, t_end, time_step);
            
            if (a_ij < i->time) {
                i->time = a_ij;
                i->prev = *min->cat_nr;
                push_tfr(S, i->time, i_catnr);
            }

        }
    }

    tdsp_node *answer = (tdsp_node *)g_hash_table_lookup(best, &end_node);

    if (answer == NULL) return final_answer;

    //print previous list
    if (answer->time != G_MAXDOUBLE) {
        printf("found path: \n  prev catnr: %i at time %f\n", end_node, answer->time);

        while (answer->prev != -1) {
            tdsp_node *next = (tdsp_node *)g_hash_table_lookup(best, &answer->prev);
            printf("    prev catnr: %i at time %f\n", answer->prev, next->time);

            answer = (tdsp_node *)g_hash_table_lookup(best, &answer->prev);
        }

        final_answer = TRUE;
    }

    free(S);
    free(min);
    g_hash_table_destroy(best);

    return final_answer;
}


gboolean catnr_equal(gconstpointer a, gconstpointer b) {
    return *(gint *)a == *(gint *)b;
}