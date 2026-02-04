#include <glib/gi18n.h>
#include "test-headers.h"

/**
 * ToDo testing:
 *  - finish generate satellite history test
 *  - finish disjkstra logic test
 */

int main (int argc, char *argv[]) {
    
    g_test_init(&argc, &argv, NULL);

    g_test_add("/t_time_test.c/t_time_simple", sat_hist, NULL, 
        t_time_simple_setup, t_time_simple, t_time_teardown);
    
    g_test_add("/t_time_test.c/t_time_end_overflow", sat_hist, NULL, 
        t_time_simple_setup, t_time_end_overflow, t_time_teardown);   

    g_test_add("/t_time_test.c/t_time_post_end_test", sat_hist, NULL, 
        t_time_simple_setup, t_time_post_end_test, t_time_teardown);

    g_test_add("/t_time_test.c/t_time_post_end_sloped", sat_hist, NULL, 
        t_time_simple_setup, t_time_post_end_sloped, t_time_teardown);
   
    g_test_add("/t_time_test.c/t_time_pre_start_sloped", sat_hist, NULL,
        t_time_simple_setup, t_time_pre_start_sloped, t_time_teardown);
    
    g_test_add("/t_time_test.c/t_time_complicated_sine", sat_hist, NULL,
        t_time_complicated_setup, t_time_complicated_sine, t_time_teardown);

    g_test_add_func("/tdsp_test.c/tdsp_simple_test", tdsp_simple_test);

    g_test_add_func("/tdsp_test.c/tdsp_multi_paths_1_correct_test", tdsp_multi_paths_1_correct_test);

    g_test_add_func("/tdsp_test.c/tdsp_end_transfers_away_test", tdsp_end_transfers_away_test);

    return g_test_run();
}