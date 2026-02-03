#include <glib/gi18n.h>
#include "../skr-utils.h"
#include "../sgpsdp/sgp4sdp4.h"


gdouble accum_pre_start(gdouble x_i_mid, gint start_i, gdouble t_start, gdouble time_step, vector_t *src_hist, vector_t *dst_hist);
gdouble accum_post_end(gdouble data_size, gint start_i, gdouble t_start, gdouble t_end, gdouble time_step, vector_t *src_hist, vector_t *dst_hist);

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
    gdouble skr = scaled_inter_sat_link(&src_sat_history[i], &dst_sat_history[i]); 
    
    //i = 1
    gint next_i = start_i + 1;
    if (next_i >= history_len) return G_MAXDOUBLE;
    gdouble next_skr = scaled_inter_sat_link(&src_sat_history[next_i], &dst_sat_history[next_i]);

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
        next_skr = scaled_inter_sat_link(&src_sat_history[next_i], &dst_sat_history[next_i]);

        /*
        gdouble prev_odd_even_term = (((i - start_i)-1)%2 == 0) ? 2.0 : 4.0;
        if ((i - start_i) == 1) prev_odd_even_term = 1;

        gdouble odd_even_term = ((i - start_i)%2 == 0) ? 2.0 : 4.0;

        check_term = accum + ((time_step * prev_odd_even_term / 3.0) * prev_skr) 
                            + ((time_step * odd_even_term / 3.0) * skr) 
                            + ((time_step / 3.0) * next_skr);
        */
        if ((i - start_i) == 1) {
            check_term = accum + ((time_step / 3.0) * prev_skr) 
                            + ((time_step * 4.0 / 3.0) * skr) 
                            + ((time_step / 3.0) * next_skr);
        }
        //case: next_i is even. Finish simpson with next_i
        else if ((next_i - start_i) % 2 == 0) {
            check_term = accum + ((time_step * 2.0 / 3.0) * prev_skr)
                               + ((time_step * 4.0 / 3.0) * skr) 
                               + ((time_step / 3.0) * next_skr);
        //case: next_i is odd. Finish simpson with i and trapezoid to next_i
        } else {
            check_term = accum + ((time_step * 4.0 / 3.0) * prev_skr)
                               + ((time_step / 3.0) * skr)
                               + 0.5 * time_step * (skr + next_skr);
        }

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

        //trapezoid rule to deal with x_i-1 to x_i
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
    gdouble y_i = scaled_inter_sat_link(&src_hist[start_i], &dst_hist[start_i]);
    gdouble x_i_prev = x_i - time_step;
    gdouble y_i_prev = scaled_inter_sat_link(&src_hist[start_i - 1], &dst_hist[start_i - 1]);

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
    gdouble y_i = scaled_inter_sat_link(&src_hist[start_i], &dst_hist[start_i]);
    gdouble y_i_nxt = scaled_inter_sat_link(&src_hist[start_i+1], &dst_hist[start_i+1]);

    //Simplify calculation: set x_i = 0, x_i_nxt = time_step
    //Trapezoid rule:   Area = 0.5 * (X_i_mid - 0) * (y_i + y_i_mid)
    //Lerp:     y_i_mid = y_i + (X_i_mid - 0) * ((y_i_nxt - y_i) / (time_step - 0))

    //combine and solve for x_i_mid, get quadratic, solve for quadratic
    gdouble a = (y_i_nxt - y_i) / time_step;
    gdouble b = 2 * y_i;
    gdouble c = -2 * data_size;

    // "a" becomes zero when y_i_nxt and y_i are equal
    if (fabs(a) < 0.000000000000001)
        return x_i + (data_size / y_i);

    gdouble x_mid = ( (-1 * b) + sqrt(pow(b, 2.0) - (4 * a * c)) ) / (2 * a);
    gdouble answer = x_i + x_mid;

    if (answer > t_end) return G_MAXDOUBLE;
    
    return answer;
}