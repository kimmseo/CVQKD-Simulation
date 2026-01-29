#include <glib/gi18n.h>
#include "test-headers.h"

int main (int argc, char *argv[]) {
    
    g_test_init(&argc, &argv, NULL);

    g_test_add_data_func("/t_time_test.c/t_time_simple", NULL, t_time_simple);

    return g_test_run();
}