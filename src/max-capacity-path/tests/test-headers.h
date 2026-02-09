#include "../../sgpsdp/sgp4sdp4.h"
#include "../path-util.h"

#define UNUSED(x) (void)(x)

typedef struct {
    GHashTable *sat_hist;
    gint catnrs[2];
    lw_sat_t *values[2];
    gint hist_len;
    gdouble w_start;            //time window start time
    gdouble w_end;
    gdouble w_step;
    tdsp_node src;
    tdsp_node dst;
} sat_hist;

void t_time_simple_setup(sat_hist *S, gconstpointer user_data);
void t_time_complicated_setup(sat_hist *S, gconstpointer user_data);
void t_time_teardown(sat_hist *S, gconstpointer user_data);

void t_time_simple(sat_hist *S, gconstpointer user_data);

void t_time_end_overflow(sat_hist *S, gconstpointer user_data);

void t_time_post_end_test(sat_hist *S, gconstpointer user_data);

void t_time_post_end_sloped(sat_hist *S, gconstpointer user_data);

void t_time_pre_start_sloped(sat_hist *S, gconstpointer user_data);

void t_time_complicated_sine(sat_hist *S, gconstpointer user_data);

void tdsp_simple_test();

void tdsp_multi_paths_1_correct_test();

void tdsp_end_transfers_away_test();



