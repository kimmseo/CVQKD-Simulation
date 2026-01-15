#include "link-capacity-path.h"
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "skr-utils.h"
#include <transfer-heap.h>
#include <math.h>

gint time_compare(gconstpointer a, gconstpointer b);
gboolean TDSP_fixed_size(
    GHashTable *sats, 
    gdouble data_size, 
    gint start_node, 
    gint end_node, 
    double t_start, 
    double t_end, 
    double time_step);
gdouble get_transfer_time(
    gint *src, 
    gint *dst, 
    gdouble data_size, 
    GHashTable *sats, 
    gdouble time, 
    gdouble t_end, 
    gdouble time_step);
void sat_at_time(sat_t *sat, gdouble time);
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
 *  - Implement modified Dijkstra
 *      -returns value of shortest path @done
 *      -returns shortest path          @done
 *  - Implement Simpson integration for time need to transfer data @done
 *  - Implement the Binary Search   @done
 *  - Write about modified algorithm in code comments. Mention paper and DOI
 *  - Clean up code, keep same style as rest of project
 */
void get_max_link_path(
    GHashTable *sats, 
    double t_start, 
    double t_end, 
    double time_step) {

    //GHashtable sats has {key:value} = {gint Catnr : sat_t *satellite}

    printf("start time %f\n", t_start);

    gdouble low = 0;
    gdouble high = 80000;   //80000 bits = 10 kilobytes
    gdouble mid = -1;

    //within 0.1 kb of right answer. 
    while (low <= high) {
        mid = (low + high) / 2.0;

        printf("trying for data size %f bits\n", mid);
        gboolean attempt = TDSP_fixed_size(sats, mid, 39466, 27607, t_start, t_end, time_step);
        
        if (!attempt) {
            high = mid - 1;
        } else if (attempt) {
            low = mid + 1;
        }
    } 
   
    if (mid != -1) {
        printf("FOUND MAX CAPACITY TRANSFER: %f\n", mid);
    }
    
    //gdouble data_size = 8000;   // 8000 bits = 1 kilobyte

    //TDSP_fixed_size(sats, data_size, 39466, 27607, t_start, t_end, time_step);
}


// Time-dependent shortest path for a data amount of fixed size. Modified Dijkstra
gboolean TDSP_fixed_size(
    GHashTable *sats, 
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
            
            gdouble a_ij = get_transfer_time(min->cat_nr, i_catnr, data_size, sats, min->time, t_end, time_step);

            
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
        GHashTable *sats, 
        gdouble time, 
        gdouble t_end, 
        gdouble time_step) {
            
    const sat_t *const_src_sat;
    if ((const_src_sat = g_hash_table_lookup(sats, src)) == NULL) return G_MAXDOUBLE;
    sat_t src_sat;
    memcpy(&src_sat, const_src_sat, sizeof(sat_t));

    const sat_t *const_dst_sat;
    if ((const_dst_sat = g_hash_table_lookup(sats, dst)) == NULL) return G_MAXDOUBLE;
    sat_t dst_sat;
    memcpy(&dst_sat, const_dst_sat, sizeof(sat_t));

    int i = 0;

    gdouble accum = 0; 

    gdouble prev_skr = 0;

    sat_at_time(&src_sat, time);
    sat_at_time(&dst_sat, time); 
    gdouble skr = inter_sat_link(&src_sat, &dst_sat);

    gdouble next_time = time + time_step;
    sat_at_time(&src_sat, next_time);
    sat_at_time(&dst_sat, next_time); 
    gdouble next_skr = inter_sat_link(&src_sat, &dst_sat); 
            
    //x_0 + x_1
    gdouble check_term = (time_step / 3) * skr + (time_step / 3) * next_skr;

    while(check_term < data_size) {

        if (next_time >= t_end) return G_MAXDOUBLE;

        //different start for x_0
        if (i == 1) {
            accum += (time_step / 3.0) * prev_skr;

        //if x_i is even then x_i - 1 was odd
        } else if (i%2 == 0) {
            accum += (time_step * 4.0 / 3.0) * prev_skr;

        //if x_i is odd then x_i - 1 was even
        } else {
            accum += (time_step * 2.0 / 3.0) * prev_skr;
        }

        i++;
        prev_skr = skr;
        skr = next_skr;
        
        next_time += time_step;

        sat_at_time(&src_sat, next_time);
        sat_at_time(&dst_sat, next_time);
        next_skr = inter_sat_link(&src_sat, &dst_sat);

        gdouble prev_odd_even_term = ((i-1)%2 == 0) ? 2.0 : 4.0;
        if (i == 1) prev_odd_even_term = 1;

        gdouble odd_even_term = (i%2 == 0) ? 2.0 : 4.0;

        check_term = accum + ((time_step * prev_odd_even_term / 3.0) * prev_skr) 
                            + ((time_step * odd_even_term / 3.0) * skr) 
                            + ((time_step / 3.0) * next_skr);
    }

    // last step
    if (i == 0) {               //case: x_0 + x_1 > data size
        return time + 2.0 * data_size / skr;

    } else if (i % 2 == 0) {    //case: x_i is even. Finish simpson with x_i
        accum += (time_step * 4.0 / 3.0) * prev_skr; 
        accum += (time_step / 3.0) * skr;

        gdouble simpson_bit = next_time - time_step;
        gdouble last_bit = 2 * (data_size - accum) / skr;
        return simpson_bit + last_bit;

    } else {//(i % 2 == 1)      //case: x_i is odd, Finish simpson with x_i-1
        accum += (time_step / 3.0) * prev_skr;

        //trapezoid rule to deal with interval prev_skr to skr
        accum += 0.5 * time_step * fabs(skr - prev_skr);
        gdouble simpson_bit = next_time - time_step;
        
        gdouble last_bit = 2 * (data_size - accum) / skr;
        return simpson_bit + last_bit; 
    }
}

/**
 * Calculate and set position of satellites to given time.
 * Copies move calc from predict-tools.c without Qth calc
 */
void sat_at_time(sat_t *sat, gdouble time) {

    sat->jul_utc = time;
    sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;

    /* call the norad routines according to the deep-space flag */
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4(sat, sat->tsince);
    else
        SGP4(sat, sat->tsince);

    Convert_Sat_State(&sat->pos, &sat->vel);
}

gboolean catnr_equal(gconstpointer a, gconstpointer b) {
    return *(gint *)a == *(gint *)b;
}