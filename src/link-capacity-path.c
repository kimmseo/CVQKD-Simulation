#include "link-capacity-path.h"
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "skr-utils.h"
#include <transfer-heap.h>

gpointer copy_glist_element(gconstpointer src, gpointer user_data);
gint time_compare(gconstpointer a, gconstpointer b);
void TDSP_fixed_size(GHashTable *sats, gdouble data_size, gint start_node, gint end_node, double t_start, double t_end, double time_step);
void sats_at_time(GList *sats, gdouble time);
void update_link_cap(GList *sats, GHashTable *history, gdouble limit, gdouble time_step);
void new_simpson_entry(GHashTable *history, char *tfr_key, gdouble tfr_value);
void update_simpson_entry(simpson_intg *tfr_val, gdouble cur_val, gdouble time_step);
void last_simpson_entry(GHashTable *history, char *key, simpson_intg *val, gdouble cur_val, gdouble time_step);
char *find_key(sat_t *satA, sat_t *satB);

/**
 * Prints path of satellite transfers that can carry maximum amount of data from
 * first to last savoid.
 * 
 * Runs binary search on size of data it transfers to find max limit for which 
 * a path exists in the time window given.Find a path for a given fixed size by
 * running modified time-dependent Dijkstra.
 * 
 * ToDo:
 *  - Implement modified Dijkstra
 *      -returns value of shortest path @done
 *      -returns shortest path          @done
 *  - Implement Simpson integration for time need to transfer data
 *  - Implement the Binary Search
 *  - Write about modified algorithm in code comments. Mention paper and DOI
 *  - Clean up code, keep same style as rest of project
 */
void get_max_link_path(
    GHashTable *sats, 
    gdouble link_cap_limit, 
    double t_start, 
    double t_end, 
    double time_step) {

    //GHashtable sats has {key:value} = {gint Catnr : sat_t *satellite}

    gdouble data_size = 1000;
    TDSP_fixed_size(sats, data_size, 42775, 43600, 0, t_end, time_step);
}

gboolean catnr_equal(gconstpointer a, gconstpointer b) {
    return *(gint *)a == *(gint *)b;
}

//amount of time it takes to transmit data from source -> i ++ i to j at time j->time
gdouble test_adj_list(gdouble start_time, gint *src, gint *dst, gdouble data_size){
   /*
    catnr: 42775
    catnr: 41460
    catnr: 46395
    catnr: 43600
   */ 
    
    gdouble thing = start_time;
    thing = data_size;

    GHashTable *look = g_hash_table_new(g_int_hash, g_int_equal);
    int zero_k = 42775;
    int one_k = 41460;
    int two_k = 46395;
    int three_k = 43600;

    int zero = 0;
    int one = 1;
    int two = 2;
    int three = 3;
    
    g_hash_table_insert(look, &zero_k, &zero);
    g_hash_table_insert(look, &one_k, &one);
    g_hash_table_insert(look, &two_k, &two);
    g_hash_table_insert(look, &three_k, &three);
    
    gdouble adjList[4][4] = {
        {G_MAXDOUBLE,           1,           3, G_MAXDOUBLE},
        {G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE,           6},
        {G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE,           5},
        {G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE}
    };

    return start_time + adjList[*(int *)g_hash_table_lookup(look, src)][*(int *)g_hash_table_lookup(look, dst)];
}


// Time-dependent shortest path for a data amount of fixed size. Modified Dijkstra
void TDSP_fixed_size(
    GHashTable *sats, 
    gdouble data_size,
    gint start_node,
    gint end_node,
    double t_start,
    double t_end,
    double time_step) {

    //best arrival times so far
    //{key : value} = {gint catnr : tdsp_node i}
    GHashTable *best = g_hash_table_new_full(g_int_hash, catnr_equal, NULL, free);        

    //priority queue implemented with binary heap
    heap_tfr *S = (heap_tfr *)calloc(1, sizeof(heap_tfr));

    //initialize distance to all satellites to infinity, except for start node
    GList *cats_list = g_hash_table_get_keys(sats); 
    for (GList *sat_cat = cats_list; sat_cat != NULL; sat_cat = sat_cat->next) {

        gint *catnr = (gint *)sat_cat->data;

        tdsp_node *i = malloc(sizeof(tdsp_node));
        i->prev = -1;
        i->time = G_MAXDOUBLE;

        //time at start node = start time, all other nodes = infinity
        if (*catnr == start_node) {
            i->time = t_start;
        }

        g_hash_table_insert(best, catnr, i);        
        push_tfr(S, i->time, catnr);
    } 
    
    node_tfr *min = malloc(sizeof(node_tfr));
    //main dijkstra loop
    while (S->len != 0) {
        pop_tfr(S, min);

        if (min == NULL) {
            return;
        }

        //found shortest path to result
        if (*min->cat_nr == end_node) break;

        for (GList *check = cats_list; check != NULL; check = check->next) {
            gint *i_catnr = (gint *)check->data;
            tdsp_node *i = (tdsp_node *)g_hash_table_lookup(best, i_catnr); 

            if (*i_catnr == *min->cat_nr) continue;

            if (*i_catnr == start_node) continue;
            
            gdouble a_ij = test_adj_list(min->time, min->cat_nr, i_catnr, data_size);
            
            if (a_ij < i->time) {
                i->time = a_ij;
                i->prev = *min->cat_nr;
                push_tfr(S, i->time, i_catnr);
            }
        }
    }

    tdsp_node *answer = (tdsp_node *)g_hash_table_lookup(best, &end_node);

    if (answer == NULL) return;

    //print previous list
    if (answer->time != G_MAXDOUBLE) {
        printf("found path: \n  prev catnr: %i at time %f\n", end_node, answer->time);

        while (answer->prev != -1) {
            tdsp_node *next = (tdsp_node *)g_hash_table_lookup(best, &answer->prev);
            printf("    prev catnr: %i at time %f\n", answer->prev, next->time);

            answer = (tdsp_node *)g_hash_table_lookup(best, &answer->prev);
        }
    }

    //clean up memory 
    free(S);
    free(min);
    g_hash_table_destroy(best);
}

//Helper function for sorting earliest arrival time binary tree
gint time_compare(gconstpointer a, gconstpointer b) {
    gdouble timeA = *(gdouble *)a;
    gdouble timeB = *(gdouble *)b;
    if (timeA < timeB) return -1;
    if (timeA > timeB) return 1; 

    return 0;
}

/** 
 * Increments all satellites by time_step from t_start to t_end and calculates
 * link capacities between them. The ones above link_cap_limit are printed.
 * 
 * Astronomical Julian Date. Number of days since 12:00 January 1, 4713 BCE.
 * Numbers after decimal point represent smaller time units as fraction of a day.
 * Ex) 0.5 is half a day.
 * 
 * \param sats hash table of all satellites
 * \param link_cap_limit lower bound on evaluated link capacities, kilobytes/minute
 * \param t_start time when calculations start, in Julian date format
 * \param t_end time when calculations stop, in Julian date format
 * \param time_step increment to consider when moving satellites, in minutes  
 */
void get_all_link_capacities(
    GHashTable *sats, 
    gdouble link_cap_limit, 
    gdouble t_start, 
    gdouble t_end, 
    gdouble time_step) {

    GList *const_sats = g_hash_table_get_values(sats);

    //copy satellite values
    GList *mut_sats = g_list_copy_deep(const_sats, copy_glist_element, NULL);

    /* 
    keys are g_string of concatonation of satellite catelogue number, 
        WLOG catA < catB, transfer between sat A and sat B, key = "catA:catB"
    value is struct simpson_intg pointer
    */
    GHashTable *linkCapBuild = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);

    for (gdouble time = t_start; time < t_end; time += (time_step / xmnpda)) {
        sats_at_time(mut_sats, time);
        update_link_cap(mut_sats, linkCapBuild, link_cap_limit, time_step);
    }

    //transfers that didn't finish before t_end
    guint *rem_len = malloc(sizeof(gpointer));
    gpointer *keys = g_hash_table_get_keys_as_array(linkCapBuild, rem_len);
    for (guint i = 0; i < *rem_len; i++){
        simpson_intg *val = g_hash_table_lookup(linkCapBuild, keys[i]); 
        last_simpson_entry(linkCapBuild, keys[i], val, 0, time_step);
    }

    free(keys);
    free(rem_len);

    //free the copy of the satellites
    g_list_free_full(mut_sats, free);

    g_hash_table_destroy(linkCapBuild);
}

void update_link_cap(GList *sats, GHashTable *history, gdouble limit, gdouble time_step) {
    GList *current, *check;

    for (current = sats; current != NULL; current = current->next) {

        for (check = current->next; check != NULL; check = check->next) {

            //find transfer key and value
            char *tfr_key = find_key(current->data, check->data);
            simpson_intg *tfr_value = g_hash_table_lookup(history, tfr_key);
            
            //value is in bits/second so convert to bytes/minute
            gdouble cur_value = inter_sat_link(current->data, check->data) * (60.0/1000.0);

            if (tfr_value  == NULL && cur_value > limit) {
                new_simpson_entry(history, tfr_key, cur_value);
            } else if (tfr_value != NULL && cur_value > limit) {
                update_simpson_entry(tfr_value, cur_value, time_step);
            } else if (tfr_value != NULL && cur_value <= limit) {
                last_simpson_entry(history, tfr_key, tfr_value, cur_value, time_step);
            }

            free(tfr_key);
        }
    }
}

/**
 * Calculate and set position of satellites to given time.
 * Copies move calc from predict-tools.c without Qth calc
 * \param sats linked list of all satellites to be updated
 * \param time time for calculation (Julian time) 
 */
void sats_at_time(GList *sats, gdouble time) {
    GList *current; 

    for (current = sats; current != NULL; current = current->next) {
        sat_t *sat = current->data;

        sat->jul_utc = time;
        sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;
 
        /* call the norad routines according to the deep-space flag */
        if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
            SDP4(sat, sat->tsince);
        else
            SGP4(sat, sat->tsince);

        Convert_Sat_State(&sat->pos, &sat->vel);
    }
}

char *find_key(sat_t *satA, sat_t *satB) {
    //ensure catA < catB
    if (satA->tle.catnr > satB->tle.catnr) {
        sat_t *tmp = satA;
        satA = satB;
        satB = tmp;
    }

    int lenA = snprintf(NULL, 0, "%d", satA->tle.catnr);
    int lenB = snprintf(NULL, 0, "%d", satB->tle.catnr);

    char *str = malloc(lenA + lenB + 2);
    snprintf(str, lenA + lenB + 2, "%d:%d", satA->tle.catnr, satB->tle.catnr);
    
    return str;
}

void new_simpson_entry(GHashTable *history, char *tfr_key, gdouble tfr_value) {
    simpson_intg *new_entry = calloc(1, sizeof(simpson_intg));

    //first value y_0 = 0 already in cumulative, because this is y_1 = tfr_value
    new_entry->cumulative = 0;
    new_entry->last = tfr_value;
    new_entry->last_even = FALSE;

    char *table_key = malloc(strlen(tfr_key) + 1);
    strcpy(table_key, tfr_key);

    g_hash_table_insert(history, table_key, new_entry);
}

void update_simpson_entry(simpson_intg *tfr_val, gdouble cur_val, gdouble time_step) {
    //last value was even, append to cumulative
    if(tfr_val->last_even) {
        tfr_val->cumulative += (time_step / 3) * tfr_val->last * 2;
    
    //last value was odd, append to cumulative
    } else {
        tfr_val->cumulative += (time_step / 3) * tfr_val->last * 4;
    }

    tfr_val->last_even = (!tfr_val->last_even);
    tfr_val->last = cur_val;
}

void last_simpson_entry(
    GHashTable *history, 
    char *key, 
    simpson_intg *val,
    gdouble cur_val, 
    gdouble time_step) {
       
    //excluding current measurment, even number of segments so far
    if (val->last_even) {
        //Wrap up simpson rule with "last" as final element
        val->cumulative += (time_step / 3) * val->last;

        //do Trapezoid rule to clean up this ending zero
        val->cumulative += time_step * ((val->last + cur_val)/2);

    } else {
        //last was odd, add it to simpson rule
        val->cumulative += (time_step / 3) * val->last * 4;
        
        //add this ending zero to make even number of segments
        val->cumulative += (time_step / 3) * val->last;
    } 

    printf("transfer key: %s, capacity (kb): %f\n", key, val->cumulative);

    g_hash_table_remove(history, key);
}

//Helper function for deep copying list of sat_t
gpointer copy_glist_element(gconstpointer src, gpointer user_data) {
    (void)user_data; //unnecessary param
    sat_t *source = (sat_t *)src;
    sat_t *destin = malloc(sizeof(sat_t));
    memcpy(destin, source, sizeof(sat_t));

    return destin;
}