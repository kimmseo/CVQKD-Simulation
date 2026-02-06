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
    tdsp_node *data = malloc(4 * sizeof(tdsp_node));

    memcpy(data, (tdsp_node[4]){
        {.node={.id = 0, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 1, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 2, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 3, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
    }, 4 * sizeof(tdsp_node));

    GArray *tdsp_array = g_array_new_take(data, 4, TRUE, sizeof(tdsp_node));

    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 3;

    GList *result = TDSP_fixed_size(tdsp_array, NULL, straight_path, 0, 0, start_catnr, end_catnr, t_start, 0, 0);

    path_node solution[4] = {
        {.id = 0, .time = 0, .type = path_SATELLITE},
        {.id = 1, .time = 1, .type = path_SATELLITE},
        {.id = 2, .time = 2, .type = path_SATELLITE},
        {.id = 3, .time = 3, .type = path_SATELLITE}
    };


    for (int i = 0; i < 4; i++) {
        path_node *check = result->data;

        g_assert_true(check->id == solution[i].id);
        g_assert_true(check->time == solution[i].time);
        g_assert_true(check->type == solution[i].type);

        result = result->next;
    }

    g_assert_true(result == NULL); 

    g_array_free(tdsp_array, TRUE);
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
    tdsp_node *data = malloc(7 * sizeof(tdsp_node));

    memcpy(data, (tdsp_node[7]){
        {.node={.id = 0, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 1, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 2, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 3, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 4, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 5, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 6, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL}
    }, 7 * sizeof(tdsp_node));

    GArray *tdsp_array = g_array_new_take(data, 7, TRUE, sizeof(tdsp_node));
    
    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 6;

    GList *result = TDSP_fixed_size(tdsp_array, NULL, multi_paths_1_correct, 0, 0, start_catnr, end_catnr, t_start, 0, 0);

    path_node solution[6] = {
        {.id = 0, .time = 0, .type=path_SATELLITE},
        {.id = 1, .time = 1, .type=path_SATELLITE},
        {.id = 3, .time = 2, .type=path_SATELLITE},
        {.id = 2, .time = 3, .type=path_SATELLITE},
        {.id = 5, .time = 5, .type=path_SATELLITE},
        {.id = 6, .time = 7, .type=path_SATELLITE}
    };

    for (int i = 0; i < 6; i++) {
        path_node *check = result->data;
        g_assert_true(check->id == solution[i].id);
        g_assert_true(check->time == solution[i].time);
        g_assert_true(check->type == solution[i].type);

        result = result->next;
    }

    g_assert_true(result == NULL); 
    g_array_free(tdsp_array, TRUE);
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
    tdsp_node *data = malloc(4 * sizeof(tdsp_node));

    memcpy(data, (tdsp_node[4]){
        {.node={.id = 0, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 1, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 2, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL},
        {.node={.id = 3, .time=G_MAXDOUBLE, .type=path_SATELLITE}, .prev_node=NULL}
    }, 4 * sizeof(tdsp_node));

    GArray *tdsp_array = g_array_new_take(data, 4, TRUE, sizeof(tdsp_node));

    gdouble t_start = 0;
    gint start_catnr = 0;
    gint end_catnr = 3;

    GList *result = TDSP_fixed_size(tdsp_array, NULL, end_transfers_away, 0, 0, start_catnr, end_catnr, t_start, 0, 0);

    path_node solution[3] = {
        {.id = 0, .time = 0, .type=path_SATELLITE},
        {.id = 1, .time = 1, .type=path_SATELLITE},
        {.id = 3, .time = 4, .type=path_SATELLITE}
    };

    for (int i = 0; i < 3; i++) {
        path_node *check = result->data;

        g_assert_true(check->id == solution[i].id);
        g_assert_true(check->time == solution[i].time);
        g_assert_true(check->type == solution[i].type);

        result = result->next;
    }

    g_assert_true(result == NULL); 
    g_array_free(tdsp_array, TRUE);
}