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
gdouble accum_pre_start(gdouble x_i_mid, gint start_i, gdouble t_start, gdouble time_step, vector_t *src_hist, vector_t *dst_hist);
gdouble accum_post_end(gdouble data_size, gint start_i, gdouble t_start, gdouble t_end, gdouble time_step, vector_t *src_hist, vector_t *dst_hist);
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
 *  - Clean up code, keep same style as rest of project.
 *      - clean up names (keep it consistent)
 *      - group things together, reduce clutter in parameters 
 *          -t_start, t_end, time_step into time_window struct
 *          -hist_len and sat_history object? 
 *          -combine pre lerp code with its wrapper?
 *      - Write about modified algorithm in code comments. Mention paper and DOI
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

    gint start_i = (gint) ceil((time - t_start) / time_step); 
    if (start_i < 0 || start_i >= history_len - 1) return G_MAXDOUBLE;
    // ================ pre_x_0 --> x_0 ========================================================
    gdouble accum = accum_pre_start(time, start_i, t_start, time_step, src_sat_history, dst_sat_history);
   
    //within a timestep its already more than enough to transmit all data
    //limit to how accurate we can be, so just return smallest time we know is enough
    if (accum >= data_size) return t_start + (start_i * time_step);

    // ================ x_0 --> x_n =============================================================
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

    // ================ x_n --> post_x_n ========================================================

    // last step
        //case: pre_x_i --> x_0 --> x_1 > data size
    if (i == start_i) {
        gdouble data_left = data_size - accum;

        gdouble final_result = accum_post_end(data_left, i, t_start, t_end, time_step, src_sat_history, dst_sat_history);

        return final_result;

        //case: x_i is even. Finish simpson with x_i
    } else if ((i - start_i) % 2 == 0) {
        accum += (time_step * 4.0 / 3.0) * prev_skr; 
        accum += (time_step / 3.0) * skr;

        //case: x_i is odd, Finish simpson with x_i-1
    } else {
        accum += (time_step / 3.0) * prev_skr;

        //trapezoid rule to deal with interval prev_skr to skr
        accum += 0.5 * time_step * (skr + prev_skr);
    }

    gdouble data_left = data_size - accum;
    gdouble total = accum_post_end(data_left, i, t_start, t_end, time_step, src_sat_history, dst_sat_history);

    return total; 
}

/**
 * Performs estimation of amount data transferred during actual transfer start
 * time to the next closest index (recorded positions).
 * @param x_i_mid   actual start time
 * @param start_i   next closest index to actual start time
 * @param t_start   start of time window we are searching for optimal path
 * @param time_step time increment between successive index in history
 * @param src_hist  array of source satellite positions
 * @param dst_hist  array of destination satellite positions
 * @return amount of data transmitted during window actual_start_time to next closest index
 */
gdouble accum_pre_start(gdouble x_i_mid, gint start_i, gdouble t_start, gdouble time_step, vector_t *src_hist, vector_t *dst_hist) {
    if (start_i <= 0) return 0;

    gdouble x_i = t_start + (start_i * time_step);
    gdouble y_i = inter_sat_pos_link(&src_hist[start_i], &dst_hist[start_i]);
    gdouble x_i_prev = x_i - time_step;
    gdouble y_i_prev = inter_sat_pos_link(&src_hist[start_i - 1], &dst_hist[start_i - 1]);

    //lerp to get rate transfer at start_time
    gdouble y_start = y_i_prev + (x_i_mid - x_i_prev) * ((y_i_prev - y_i) / (x_i_prev - x_i));

    //apply trapezoid rule to get area (data amount)
    gdouble data = 0.5 * (x_i - x_i_mid) * (y_start + y_i);

    return data;
}

/**
 * Estimate how much time it would take to transmit last bit of data, 
 * returns time at which it would stop.
 * @param data_size     the data left to be transmitted (bits)
 * @param start_i       last index we did simpson integration for
 * @param t_start       start of time window for position measurments
 * @param t_end         end of time window for position measurments (inclusive)
 * @param time_step     amount of time between i and i+1
 * @param src_hist      position history of source satellite
 * @param dst_hist      position history of destination satellite
 * @return time as which it would stop after transmitting data
 */
gdouble accum_post_end(gdouble data_size, gint start_i, gdouble t_start, gdouble t_end, gdouble time_step, vector_t *src_hist, vector_t *dst_hist) {
    
    //start_i has checks. Always < history_len - 1. 
    //So theres always a start_i and start_i + 1
    gdouble x_i = t_start  + (start_i * time_step);
    gdouble y_i = inter_sat_pos_link(&src_hist[start_i], &dst_hist[start_i]);
    gdouble y_i_nxt = inter_sat_pos_link(&src_hist[start_i+1], &dst_hist[start_i+1]);

    //Simplify calculation: set x_i = 0, x_i_nxt = time_step
    //Trapezoid rule:   Area = 0.5 * (X_i_mid - 0) * (y_i + y_i_mid)
    //Lerp:     y_i_mid = y_i + (X_i_mid - 0) * ((y_i_nxt - y_i) / (time_step - 0))

    //combine and solve for x_i_mid, get quadratic, solve for quadratic
    gdouble a = 0.5 * ((y_i_nxt + y_i) / time_step);
    gdouble b = y_i;
    gdouble c = -1 * data_size;

    gdouble answer = x_i + ( (-1 * b) + sqrt(pow(b, 2.0) - (4 * a * c)) ) / (2 * a);

    if (answer > t_end) return G_MAXDOUBLE;
    
    return answer;
}

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