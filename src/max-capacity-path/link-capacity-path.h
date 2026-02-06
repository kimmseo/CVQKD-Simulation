#include <glib/gi18n.h>

typedef enum {
    path_STATION,
    path_SATELLITE
} path_type;

typedef struct path_node{
    gdouble time;                   //earliest arrival so far
    gint id;                        //for satellites id is catnr, for ground stations, assigned negative number ids
    path_type type;
} path_node;

typedef struct tdsp_node {
    struct tdsp_node *prev_node;
    path_node node;
} tdsp_node;              //wrapper type keeps track of node with best path to it
                    //including prev_node into path_node is troublesome 
                    //when returning result (GList of copies)

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