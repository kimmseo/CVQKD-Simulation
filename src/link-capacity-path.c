#include "link-capacity-path.h"
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "skr-utils.h"

gpointer copy_glist_element(gconstpointer src, gpointer user_data);
void sats_at_time(GList *sats, gdouble time);
void update_link_cap(GList *sats, GHashTable *history, gdouble limit, gdouble time_step);
void new_simpson_entry(GHashTable *history, char *tfr_key, gdouble tfr_value);
void update_simpson_entry(simpson_intg *tfr_val, gdouble cur_val, gdouble time_step);
void last_simpson_entry(GHashTable *history, char *key, simpson_intg *val, gdouble cur_val, gdouble time_step);
char *find_key(sat_t *satA, sat_t *satB);

//make output an array of transfer struct of link capacities organized by start time
/** Increments all satellites by time_step from t_start to t_end and calculates
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
void generate_link_capacities(
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