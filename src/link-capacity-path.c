#include "link-capacity-path.h"
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "skr-utils.h"
#include <transfer-heap.h>
#include <math.h>
#include <transfer-table.h>
#include <time.h>

gint time_compare(gconstpointer a, gconstpointer b);
gboolean TDSP_fixed_size(GHashTable *sats, GHashTable *sat_history, gint hist_len, gdouble data_size, gint start_node, gint end_node, double t_start, double t_end, double time_step);
gdouble get_transfer_time(gint *src, gint *dst, gdouble data_size, GHashTable *sat_history, gint history_len, gdouble time, gdouble t_start, gdouble t_end, gdouble time_step); 
void sat_at_time(sat_t *sat, gdouble time);
gboolean catnr_equal(gconstpointer a, gconstpointer b);
void print_GHashTable_key_value(GHashTable *data, gdouble t_start, gdouble t_end, gdouble time_step);

/**
 * Prints path of satellite transfers that can carry maximum amount of data from
 * first to last savoid.
 * 
 * Runs binary search on size of data it transfers to find max limit for which 
 * a path exists in the time window given.Find a path for a given fixed size by
 * running modified time-dependent Dijkstra.
 * 
 * ToDo:
 *  - Time-Space Optimization 
 *  - Testing with data
 *  - Implement ground stations
 *  - Create user UI for interacting with code
 *  - Write about modified algorithm in code comments. Mention paper and DOI
 *  - Clean up code, keep same style as rest of project.
 *      - add input checking and error handling
 */
void get_max_link_path(
    GHashTable *sats, 
    double t_start, 
    double t_end, 
    double time_step) {

    //GHashtable sats has {key:value} = {gint Catnr : sat_t *satellite}
    int hist_len = 0;
    GHashTable *sat_history = generate_transfer_data(g_hash_table_get_values(sats), &hist_len, t_start, t_end, time_step); 
    
    //print_GHashTable_key_value(sat_history, t_start, t_end, time_step);
    printf("start time %f\n", t_start);

    gdouble low = 0;
    gdouble high = 80000;   //80000 bits = 10 kilobytes
    gdouble mid = -1;

    //within 0.1 kb of right answer. 
    while (low <= high) {
        mid = (low + high) / 2.0;

        printf("trying for data size %f bits\n", mid);
        gboolean attempt = TDSP_fixed_size(sats, sat_history, hist_len, mid, 39466, 27607, t_start, t_end, time_step);
        
        if (!attempt) {
            high = mid - 1;
        } else if (attempt) {
            low = mid + 1;
        }
    } 
   
    if (mid != -1) {
        printf("FOUND MAX CAPACITY TRANSFER: %f\n", mid);
    }

    g_hash_table_destroy(sat_history);
    
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

    //clean up memory 
    free(S);
    free(min);
    g_hash_table_destroy(best);

    return final_answer;
}

//amount of time it takes to transmit data from source -> i ++ i to j at time j->time
gdouble get_transfer_time(
        gint *src, 
        gint *dst, 
        gdouble data_size, 
        GHashTable *sat_history,
        gint history_len,
        gdouble time,
        gdouble t_start, 
        gdouble t_end, 
        gdouble time_step) {
            
    vector_t *src_sat_history = g_hash_table_lookup(sat_history, src);
    if (src_sat_history == NULL) return G_MAXDOUBLE;

    vector_t *dst_sat_history = g_hash_table_lookup(sat_history, dst);
    if (dst_sat_history == NULL) return G_MAXDOUBLE;

    gint start_i = ceil((time - t_start) / time_step);   

    //ToDo: replace with interp start between (start - 1, start)
    //ToDo: also check if accum by itself already bigger than data
    // pre_x_0 -> x_0
    gdouble accum = 0;

    gdouble prev_skr = 0;

    //i = 0
    gint i = start_i;
    if (i >= history_len) return G_MAXDOUBLE;
    gdouble skr = inter_sat_pos_link(&src_sat_history[i], &dst_sat_history[i]);
    
    //i = 1
    gint next_i = start_i + 1;
    if (next_i >= history_len) return G_MAXDOUBLE;
    gdouble next_skr = inter_sat_pos_link(&src_sat_history[next_i], &dst_sat_history[next_i]);

    //pre_x_0 -> x_0 -> x_1
    gdouble check_term = accum + (time_step / 3) * skr + (time_step / 3) * next_skr;

    //accum + x_prev + x_i + x_next < data_size
    while(check_term < data_size) { 
        
        //next_i is already at the last spot in history
        if (next_i >= history_len - 1) return G_MAXDOUBLE;

        //accumulate x_0 properly
        if (i == start_i + 1) {
            accum += (time_step / 3.0) * prev_skr;

        //if x_i is even then x_i - 1 was odd
        } else if ((i - start_i)%2 == 0) {
            accum += (time_step * 4.0 / 3.0) * prev_skr;

        //if x_i is odd then x_i - 1 was even
        } else {
            accum += (time_step * 2.0 / 3.0) * prev_skr;
        }

        prev_skr = skr;

        i = next_i;
        skr = next_skr;
       
        //array access safeguarded by if statement above
        next_i++;
        next_skr = inter_sat_pos_link(&src_sat_history[next_i], &dst_sat_history[next_i]);
        
        gdouble prev_odd_even_term = (((i - start_i)-1)%2 == 0) ? 2.0 : 4.0;
        if ((i - start_i) == 1) prev_odd_even_term = 1;

        gdouble odd_even_term = ((i - start_i)%2 == 0) ? 2.0 : 4.0;

        check_term = accum + ((time_step * prev_odd_even_term / 3.0) * prev_skr) 
                            + ((time_step * odd_even_term / 3.0) * skr) 
                            + ((time_step / 3.0) * next_skr);

    }

    // last step
        //case: pre_x_i --> x_0 --> x_1 > data size
    if (i == start_i) {
        return time + 2.0 * data_size / skr;

        //case: x_i is even. Finish simpson with x_i
    } else if ((i - start_i) % 2 == 0) {    
        accum += (time_step * 4.0 / 3.0) * prev_skr; 
        accum += (time_step / 3.0) * skr; 

        //case: x_i is odd, Finish simpson with x_i-1
    } else { //(i % 2 == 1)      
        accum += (time_step / 3.0) * prev_skr;

        //trapezoid rule to deal with interval prev_skr to skr
        accum += 0.5 * time_step * fabs(skr - prev_skr);
    }

    gdouble simpson_bit = (i * time_step) + t_start;
    gdouble last_bit = 2 * (data_size - accum) / skr;
    return simpson_bit + last_bit;
}

/**
 * Calculate and set position of satellites to given time.
 * Copies move calc from predict-tools.c without Qth calc
 */
/*
void sat_at_time(sat_t *sat, gdouble time) {

    sat->jul_utc = time;
    sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;

    //call the norad routines according to the deep-space flag 
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4(sat, sat->tsince);
    else
        SGP4(sat, sat->tsince);

    Convert_Sat_State(&sat->pos, &sat->vel);
}
*/

gboolean catnr_equal(gconstpointer a, gconstpointer b) {
    return *(gint *)a == *(gint *)b;
}

void print_GHashTable_key_value(GHashTable *data, gdouble t_start, gdouble t_end, gdouble time_step) {
    GList *keys = g_hash_table_get_keys(data);

    gdouble num_entries = ceil((t_end - t_start) / time_step); 

    for (GList *cur = keys; cur != NULL; cur = cur->next) {
        gint *catnr = (gint *)cur->data;
        vector_t *pos_array = g_hash_table_lookup(data, catnr);

        printf("for catnr; %i\n", *catnr);

        for (gint i = 0; i < num_entries; i++) {
            printf("    (%f, %f, %f)\n", pos_array[i].x, pos_array[i].y, pos_array[i].z);
        }
    }
}