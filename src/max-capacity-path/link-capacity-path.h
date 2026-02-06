#include <glib/gi18n.h>

typedef enum {
    tdsp_STATION,
    tdsp_SATELLITE
} tdsp_type;

typedef struct tdsp_node{
    gdouble time;                   //earliest arrival so far
    gint id;                        //for satellites id is catnr, for ground stations, assigned negative number ids
    struct tdsp_node *prev_node;    //id of previous node
    tdsp_type type;
} tdsp_node;


void get_max_link_path(
    GHashTable *sats, 
    GHashTable *sat_history,
    guint sat_hist_len,
    GHashTable *ground_stations,
    double t_start, 
    double t_end, 
    double time_step);

GList *TDSP_fixed_size(
    GArray *const_tdsp_array,
    GHashTable *sat_history,
    gdouble (*time_func)(gint *, gint *, gdouble, GHashTable *, gint, gdouble, gdouble, gdouble, gdouble),
    gint hist_len,
    gdouble data_size,
    gint start_node,
    gint end_node,
    gdouble t_start,
    gdouble t_end,
    gdouble time_step);