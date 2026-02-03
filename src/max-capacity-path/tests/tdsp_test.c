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
    
    if (*src == 0 && *dst == 1) {
        return time + 1;
    } else if (*src == 1 && *dst == 2) {
        return time + 1;
    } else if (*src == 2 && *dst == 3) {
        return time + 1;
    }

    return G_MAXDOUBLE;
}

void tdsp_simple_test() {
    GHashTable *sats = g_hash_table_new(g_int_hash, g_int_equal);

    sat_t val[1] = {0};

    gint catnr0 = 0; 
    g_hash_table_insert(sats, &catnr0, &val);

    gint catnr1 = 1; 
    g_hash_table_insert(sats, &catnr1, &val);

    gint catnr2 = 2; 
    g_hash_table_insert(sats, &catnr2, &val);

    gint catnr3 = 3; 
    g_hash_table_insert(sats, &catnr3, &val);

    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 3;

    GList *result = TDSP_fixed_size(sats, NULL, straight_path, 0, 0, start_catnr, end_catnr, t_start, 0, 0);
    
    tdsp_node *check = result->data;
    g_assert_true(check->prev == -1);
    g_assert_true(check->current == 0);
    g_assert_true(check->time == 0);
    
    result = result->next;
    check = (tdsp_node *)result->data;
    g_assert_true(check->prev == 0);
    g_assert_true(check->current == 1);
    g_assert_true(check->time == 1);

    result = result->next;
    check = (tdsp_node *)result->data;
    g_assert_true(check->prev == 1);
    g_assert_true(check->current == 2);
    g_assert_true(check->time == 2);

    result = result->next;
    check = (tdsp_node *)result->data;
    g_assert_true(check->prev == 2);
    g_assert_true(check->current == 3);
    g_assert_true(check->time == 3);

    result = result->next;
    g_assert_true(result == NULL); 
}