#include <glib/gi18n.h>
#include "../transfer-time.h"
#include "../../sgpsdp/sgp4sdp4.h"
#include "../../skr-utils.h"

#include "test-headers.h"

//transfer-time.c   test cases

void t_time_simple() {
    gint catnrs[2] = {1, 2};

    gint history_len = 10; 
    
    vector_t values[2][10] = {
        {
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
        },
        {
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
        }
    };

    GHashTable *history = g_hash_table_new(g_int_hash, g_int_equal);

    for (int i = 0; i < 2; i++) {
        g_hash_table_insert(history, &catnrs[i], values[i]);
    }

    gdouble time = 0.007;
    gdouble t_start = 0;
    gdouble time_step = 0.007;      //1 minutes time_steps
    gdouble t_end = t_start + (9 * time_step);

    vector_t front = {.x=0, .y=0, .z=0, .w=0};
    vector_t back = {.x=1, .y=0, .z=0, .w=0};

    // SKR (kb/time step) * 5 time steps
    gdouble data_size = scaled_inter_sat_link(&front, &back, time_step) * 5 * time_step;

    //starts at index 1 and needs to transmit for 5 time steps.
    gdouble expected = time + (time_step * 5);

    gdouble answer = get_transfer_time(&catnrs[0], &catnrs[1], data_size, history, history_len, time, t_start, t_end, time_step);
    
    g_assert_cmpfloat_with_epsilon(expected, answer, 0.00001);
}

void t_time_odd_simpson() {
    gint catnrs[2] = {1, 2};

    gint history_len = 10; 
    
    vector_t values[2][10] = {
        {
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
        },
        {
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
        }
    };

    GHashTable *history = g_hash_table_new(g_int_hash, g_int_equal);

    for (int i = 0; i < 2; i++) {
        g_hash_table_insert(history, &catnrs[i], values[i]);
    }

    gdouble time = 0.007;
    gdouble t_start = 0;
    gdouble time_step = 0.007;      //1 minutes time_steps
    gdouble t_end = t_start + (9 * time_step);

    vector_t front = {.x=0, .y=0, .z=0, .w=0};
    vector_t back = {.x=1, .y=0, .z=0, .w=0};

    // SKR (kb/time step) * 5 time steps
    gdouble data_size = scaled_inter_sat_link(&front, &back, time_step) * 5 * time_step;

    //starts at index 1 and needs to transmit for 5 time steps.
    gdouble expected = time + (time_step * 5);

    gdouble answer = get_transfer_time(&catnrs[0], &catnrs[1], data_size, history, history_len, time, t_start, t_end, time_step);
    
    g_assert_cmpfloat_with_epsilon(expected, answer, 0.00001);
}

//odd simpson
//even simpson
//pre_accum + post_accum
//starts right at start (no pre_accum)
//ends right at end (not post_accum)

//  -- EASY FAIL tests --
//not enough time to accumulate everything (just 1 step too big)
//not enough time to accumulate everything (< 1 step too big
//catnrs not part of given history