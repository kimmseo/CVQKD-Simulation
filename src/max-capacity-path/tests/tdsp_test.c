#include "../link-capacity-path.h"
#include "test-headers.h"
#include <stdio.h>

gdouble straight_path(
        gint *src, 
        gint *dst, 
        gdouble data_size, 
        GHashTable *sat_history,
        gint history_len,
        gdouble time,
        gdouble t_start, 
        gdouble t_end,
        gdouble time_step) {

    UNUSED(data_size);
    UNUSED(sat_history);
    UNUSED(history_len);
    UNUSED(t_start);
    UNUSED(t_end);
    UNUSED(time_step);
    
    if (*src == 0 && *dst == 1) return time + 1;

    else if (*src == 1 && *dst == 2) return time + 1;

    else if (*src == 2 && *dst == 3) return time + 1;

    return G_MAXDOUBLE;
}

void tdsp_simple_test() {
    GHashTable *sats = g_hash_table_new(g_int_hash, g_int_equal);
    
    gint catnrs[4];
    sat_t val[1] = {0};

    for (int i = 0; i < 4; i++) {
        catnrs[i] = i;
        g_hash_table_insert(sats, &catnrs[i], &val);        
    }

    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 3;

    GList *result = TDSP_fixed_size(sats, NULL, straight_path, 0, 0, start_catnr, end_catnr, t_start, 0, 0);

    tdsp_node solution[4] = {
        {.prev = -1, .current = 0, .time = 0},
        {.prev = 0,  .current = 1, .time = 1},
        {.prev = 1,  .current = 2, .time = 2},
        {.prev = 2,   .current = 3, .time = 3}
    };

    for (int i = 0; i < 4; i++) {
        tdsp_node *check = result->data;
        g_assert_true(check->prev == solution[i].prev);
        g_assert_true(check->current == solution[i].current);
        g_assert_true(check->time == solution[i].time);

        result = result->next;
    }

    g_assert_true(result == NULL); 
}

gdouble multi_paths_1_correct(
        gint *src, 
        gint *dst, 
        gdouble data_size, 
        GHashTable *sat_history,
        gint history_len,
        gdouble time,
        gdouble t_start, 
        gdouble t_end,
        gdouble time_step) {

    UNUSED(data_size);
    UNUSED(sat_history);
    UNUSED(history_len);
    UNUSED(t_start);
    UNUSED(t_end);
    UNUSED(time_step);
    
    if (time <= 0 && *src == 0 && *dst == 1) return 1;
    else if (time <= 0 && *src == 0 && *dst == 4) return 2;

    else if (time <= 1 && *src == 1 && *dst == 3) return 2;

    else if (time <= 2 && *src == 1 && *dst == 3) return 3;
    else if (time <= 2 && *src == 3 && *dst == 2) return 3;
    else if (time <= 2 && *src == 4 && *dst == 3) return 3;

    else if (time <= 3 && *src == 1 && *dst == 2) return 4;
    else if (time <= 3 && *src == 1 && *dst == 3) return 4;
    else if (time <= 3 && *src == 2 && *dst == 5) return 5;
    else if (time <= 3 && *src == 3 && *dst == 1) return 4;
    else if (time <= 3 && *src == 3 && *dst == 4) return 4;

    else if (time <= 5 && *src == 3 && *dst == 5) return 6;
    else if (time <= 5 && *src == 4 && *dst == 5) return 6;
    else if (time <= 5 && *src == 5 && *dst == 6) return 7;

    return G_MAXDOUBLE;
}

void tdsp_multi_paths_1_correct_test() {
    GHashTable *sats = g_hash_table_new(g_int_hash, g_int_equal);
    
    gint catnrs[7] = {0};
    sat_t val[1] = {0};

    for (int i = 0; i < 7; i++) {
        catnrs[i] = i;
        g_hash_table_insert(sats, &catnrs[i], &val);        
    }

    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 6;

    GList *result = TDSP_fixed_size(sats, NULL, multi_paths_1_correct, 0, 0, start_catnr, end_catnr, t_start, 0, 0);

    tdsp_node solution[6] = {
        {.prev = -1, .current = 0, .time = 0},
        {.prev =  0, .current = 1, .time = 1},
        {.prev =  1, .current = 3, .time = 2},
        {.prev =  3, .current = 2, .time = 3},
        {.prev =  2, .current = 5, .time = 5},
        {.prev =  5, .current = 6, .time = 7}
    };

    for (int i = 0; i < 6; i++) {
        tdsp_node *check = result->data;
        g_assert_true(check->prev == solution[i].prev);
        g_assert_true(check->current == solution[i].current);
        g_assert_true(check->time == solution[i].time);

        result = result->next;
    }

    g_assert_true(result == NULL); 
}

gdouble end_transfers_away(
        gint *src, 
        gint *dst, 
        gdouble data_size, 
        GHashTable *sat_history,
        gint history_len,
        gdouble time,
        gdouble t_start, 
        gdouble t_end,
        gdouble time_step) {

    UNUSED(data_size);
    UNUSED(sat_history);
    UNUSED(history_len);
    UNUSED(t_start);
    UNUSED(t_end);
    UNUSED(time_step);
    
    if (time <= 0 && *src == 0 && *dst == 1) return 1;
    else if (time <= 0 && *src == 0 && *dst == 2) return 1;

    else if (time <= 2 && *src == 1 && *dst == 2) return 3;

    else if (time <= 3 && *src == 1 && *dst == 3) return 4;

    else if (time <= 4 && *src == 4 && *dst == 1) return 5;
    else if (time <= 4 && *src == 4 && *dst == 2) return 5;

    else if (time <= 5 && *src == 1 && *dst == 2) return 6;
    else if (time <= 5 && *src == 2 && *dst == 1) return 6;

    else if (time <= 6 && *src == 1 && *dst == 3) return 7;
    else if (time <= 6 && *src == 2 && *dst == 3) return 7;

    return G_MAXDOUBLE;
}

void tdsp_end_transfers_away_test() {
    GHashTable *sats = g_hash_table_new(g_int_hash, g_int_equal);
    
    gint catnrs[4] = {0};
    sat_t val[1] = {0};

    for (int i = 0; i < 4; i++) {
        catnrs[i] = i;
        g_hash_table_insert(sats, &catnrs[i], &val);        
    }

    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 3;

    GList *result = TDSP_fixed_size(sats, NULL, end_transfers_away, 0, 0, start_catnr, end_catnr, t_start, 0, 0);

    tdsp_node solution[3] = {
        {.prev = -1, .current = 0, .time = 0},
        {.prev =  0, .current = 1, .time = 1},
        {.prev =  1, .current = 3, .time = 4}
    };

    for (int i = 0; i < 3; i++) {
        tdsp_node *check = result->data;
        g_assert_true(check->prev == solution[i].prev);
        g_assert_true(check->current == solution[i].current);
        g_assert_true(check->time == solution[i].time);

        result = result->next;
    }

    g_assert_true(result == NULL); 
}


//test holding data is best path