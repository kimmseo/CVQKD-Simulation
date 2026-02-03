#include <glib/gi18n.h>
#include "../transfer-time.h"
#include "../../sgpsdp/sgp4sdp4.h"
#include "../../skr-utils.h"

#include "test-headers.h"

//transfer-time.c   test cases
void t_time_simple_setup(sat_hist *S, gconstpointer user_data) {
    UNUSED(user_data); 

    S->values[0] = malloc(10 * sizeof(vector_t));
    S->values[1] = malloc(10 * sizeof(vector_t));

    memcpy(S->values[0], &(vector_t[10]){
            {.x=1, .y=0, .z=0, .w=0},
            {.x=2, .y=0, .z=0, .w=0},
            {.x=3, .y=0, .z=0, .w=0},
            {.x=4, .y=0, .z=0, .w=0},
            {.x=5, .y=0, .z=0, .w=0},
            {.x=6, .y=0, .z=0, .w=0},
            {.x=7, .y=0, .z=0, .w=0},
            {.x=8, .y=0, .z=0, .w=0},
            {.x=9, .y=0, .z=0, .w=0},
            {.x=10, .y=0, .z=0, .w=0}
        }, 10 * sizeof(vector_t));

    memcpy(S->values[1], &(vector_t[10]){
            {.x=2, .y=0, .z=0, .w=0},
            {.x=3, .y=0, .z=0, .w=0},
            {.x=4, .y=0, .z=0, .w=0},
            {.x=5, .y=0, .z=0, .w=0},
            {.x=6, .y=0, .z=0, .w=0},
            {.x=7, .y=0, .z=0, .w=0},
            {.x=8, .y=0, .z=0, .w=0},
            {.x=9, .y=0, .z=0, .w=0},
            {.x=10, .y=0, .z=0, .w=0},
            {.x=11, .y=0, .z=0, .w=0}
        }, 10 * sizeof(vector_t));
    
    memcpy(&S->catnrs, &(gint[]){1, 2}, 2 * sizeof(gint));

    S->sat_hist = g_hash_table_new(g_int_hash, g_int_equal);
        
    g_hash_table_insert(S->sat_hist, &S->catnrs[0], S->values[0]);
    g_hash_table_insert(S->sat_hist, &S->catnrs[1], S->values[1]);

    S->hist_len = 10;

    S->w_step = 0.0006944444444;      //1 minutes

    S->w_start = 0;
    S->w_end = S->w_start + (9 * S->w_step);
}

void t_time_teardown(sat_hist *S, gconstpointer user_data) {
    UNUSED(user_data);
    g_hash_table_destroy(S->sat_hist);

    free(S->values[0]);
    free(S->values[1]);
}


void t_time_simple(sat_hist *S, gconstpointer user_data) { 
    UNUSED(user_data);

    //transfer start time
    gdouble t_start = S->w_step;               //starting at index 1

    // (kilobyte per time_step)
    gdouble rate_of_transfer = scaled_inter_sat_link(&S->values[0][0], &S->values[1][0]);

    gdouble data_size = rate_of_transfer * S->w_step * 5;

    //starts at index 1 and needs to transmit for 5 time steps.
    gdouble expected = t_start + (S->w_step * 5);

    gdouble answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    
    g_assert_cmpfloat_with_epsilon(expected, answer, 0.0000001);
}

void t_time_end_overflow(sat_hist *S, gconstpointer user_data) {
    UNUSED(user_data);

    //t_time_simple_setup has history with 9 segments (10 data time positions)
    //we start at time position 1, so 8 segments we can reach.

    //transfer start time
    gdouble t_start = S->w_step;               //starting at index 1

    // (kilobyte per day)
    gdouble rate_of_transfer = scaled_inter_sat_link(&S->values[0][0], &S->values[0][1]);

    gdouble data_size = rate_of_transfer * S->w_step * 7.9;
    gdouble tiny_bit_small_enough = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    g_assert_cmpfloat(G_MAXDOUBLE, !=, tiny_bit_small_enough);
   
    data_size = rate_of_transfer * S->w_step * 8;
    gdouble answer_small_enough = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    g_assert_cmpfloat(G_MAXDOUBLE, !=, answer_small_enough);

    data_size = rate_of_transfer * S->w_step * 8.0001;
    gdouble tiny_bit_too_big = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    g_assert_cmpfloat(G_MAXDOUBLE, ==, tiny_bit_too_big);

    data_size = rate_of_transfer * S->w_step * 9;
    gdouble answer_too_big = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    g_assert_cmpfloat(G_MAXDOUBLE, ==, answer_too_big); 
}



//pre_accum + post_accum
//starts right at start (no pre_accum)

void t_time_post_end_test(sat_hist *S, gconstpointer user_data) { 
    UNUSED(user_data);

    gdouble t_start = S->w_step;               //starting at index 1

    gdouble rate_of_transfer = scaled_inter_sat_link(&S->values[0][0], &S->values[0][1]);

    gdouble data_size = rate_of_transfer * S->w_step * 4.5;

    //starts at index 1 and needs to transmit for 5 time steps
    gdouble expected = t_start + (S->w_step * 4.5);

    gdouble answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);

    g_assert_cmpfloat_with_epsilon(expected, answer, 0.0000001);
}


void t_time_post_end_sloped(sat_hist *S, gconstpointer user_data) { 
    UNUSED(user_data);

    //sloped down, distance: 1 km -> 5 km
    memcpy(&S->values[1][6], &(vector_t){.x=2, .y=0, .z=0}, sizeof(vector_t));

    gdouble t_start = S->w_step;               //starting at index 1

    //rate of transfer
    gdouble norm_ROT = scaled_inter_sat_link(&S->values[0][0], &S->values[1][0]);
    gdouble small_ROT = scaled_inter_sat_link(&S->values[0][6], &S->values[1][6]);

    gdouble simpson_part = (norm_ROT * S->w_step * 4);
    gdouble mid_ROT = (norm_ROT + small_ROT) / 2;
    gdouble post_end_part = 0.5 * (0.5 * S->w_step) * (norm_ROT + mid_ROT);

    gdouble data_size = simpson_part + post_end_part;

    //starts at index 1 and needs to transmit for 5 time steps
    gdouble expected = t_start + (S->w_step * 4.5);

    gdouble small_answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    g_assert_cmpfloat_with_epsilon(expected, small_answer, 0.0000001);


    //sloped up, distance: 1 km -> 0.1 km
    memcpy(&S->values[1][6], &(vector_t){.x=7.1, .y=0, .z=0}, sizeof(vector_t));
    gdouble big_ROT = scaled_inter_sat_link(&S->values[0][6], &S->values[1][6]);

    simpson_part = (norm_ROT * S->w_step * 4);
    mid_ROT = (norm_ROT + big_ROT) / 2;
    post_end_part = 0.5 * (0.5 * S->w_step) * (norm_ROT + mid_ROT);

    data_size = simpson_part + post_end_part;

    gdouble big_answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    
    g_assert_cmpfloat_with_epsilon(expected, big_answer, 0.0000001);

}

void t_time_pre_start_sloped(sat_hist *S, gconstpointer user_data) { 
    UNUSED(user_data);

    //sloped down, distance: 0.1 km -> 1 km
    memcpy(&S->values[1][0], &(vector_t){.x=1.1, .y=0, .z=0}, sizeof(vector_t));

    gdouble t_start = S->w_step;               //starting at index 1

    //rate of transfer
    gdouble norm_ROT = scaled_inter_sat_link(&S->values[0][2], &S->values[1][2]);
    gdouble small_ROT = scaled_inter_sat_link(&S->values[0][0], &S->values[1][0]);

    gdouble simpson_part = (norm_ROT * S->w_step * 4);
    gdouble mid_ROT = (norm_ROT + small_ROT) / 2;
    gdouble pre_start_part = 0.5 * (0.5 * S->w_step) * (norm_ROT + mid_ROT);

    gdouble data_size = pre_start_part + simpson_part;

    //starts at index 1 and needs to transmit for 5 time steps
    gdouble expected = t_start + (S->w_step * 4.5);

    gdouble small_answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    g_assert_cmpfloat_with_epsilon(expected, small_answer, 0.0000002);


    //sloped down, distance: 5 km -> 1 km
    memcpy(&S->values[1][0], &(vector_t){.x=6, .y=0, .z=0}, sizeof(vector_t));
    gdouble big_ROT = scaled_inter_sat_link(&S->values[0][0], &S->values[1][0]);

    mid_ROT = (norm_ROT + big_ROT) / 2;
    pre_start_part = 0.5 * (0.5 * S->w_step) * (norm_ROT + mid_ROT);
    data_size = pre_start_part + simpson_part;

    gdouble big_answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);
    
    g_assert_cmpfloat_with_epsilon(expected, big_answer, 0.0000002);
}


//suddenly jumps to zero at 2112
void t_time_complicated_setup(sat_hist *S, gconstpointer user_data) {
    UNUSED(user_data); 

    S->values[0] = malloc(21 * sizeof(vector_t));
    S->values[1] = malloc(21 * sizeof(vector_t));

    memcpy(S->values[0], &(vector_t[21]){
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0},
        {.x=0.0, .y=0.0, .z=0.0, .w=0.0}
        }, 21 * sizeof(vector_t));

    memcpy(S->values[1], &(vector_t[21]){
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2107.20574, .y=0.0, .z=0.0, .w=0.0},
            {.x=2103.58529, .y=0.0, .z=0.0, .w=0.0},
            {.x=2102.02505, .y=0.0, .z=0.0, .w=0.0},
            {.x=2102.90703, .y=0.0, .z=0.0, .w=0.0},
            {.x=2106.01528, .y=0.0, .z=0.0, .w=0.0},
            {.x=2110.5888,  .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2109.8488,  .y=0.0, .z=0.0, .w=0.0},
            {.x=2105.43013, .y=0.0, .z=0.0, .w=0.0},
            {.x=2102.62,    .y=0.0, .z=0.0, .w=0.0},
            {.x=2102.10642, .y=0.0, .z=0.0, .w=0.0},
            {.x=2104.01513, .y=0.0, .z=0.0, .w=0.0},
            {.x=2107.87882, .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
            {.x=2112,       .y=0.0, .z=0.0, .w=0.0},
        }, 21 * sizeof(vector_t));
    
    memcpy(&S->catnrs, &(gint[]){1, 2}, 2 * sizeof(gint));

    S->sat_hist = g_hash_table_new(g_int_hash, g_int_equal);
    g_hash_table_insert(S->sat_hist, &S->catnrs[0], S->values[0]);
    g_hash_table_insert(S->sat_hist, &S->catnrs[1], S->values[1]);

    S->hist_len = 21;

    S->w_step = 0.00034722222222222;      //30 seconds

    S->w_start = 0;
    S->w_end = S->w_start + (20 * S->w_step);
}

void print_all_measurements(vector_t *src, vector_t *dst, int hist_len) {
    for (int i = 0; i < hist_len; i++) {
        gdouble val = scaled_inter_sat_link(&src[i], &dst[i]);
        printf("index: %i, meas: %f\n", i, val);
    }
}

void t_time_complicated_sine(sat_hist *S, gconstpointer user_data) {
    UNUSED(user_data);

    //print_all_measurements(S->values[0], S->values[1], S->hist_len);
    
    //transfer start time
    gdouble t_start = 0;

    gdouble data_size = 33187.18994724562093531767096;

    gdouble expected = t_start + (S->w_step * 18);

    gdouble answer = get_transfer_time(&S->catnrs[0], &S->catnrs[1], data_size, 
        S->sat_hist, S->hist_len, t_start, S->w_start, S->w_end, S->w_step);

    
    g_assert_cmpfloat_with_epsilon(expected, answer, 0.0000001);
}