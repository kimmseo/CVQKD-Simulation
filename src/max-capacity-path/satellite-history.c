#include <glib/gi18n.h>
#include "../sgpsdp/sgp4sdp4.h"

void sat_at_time(sat_t *sat, gdouble time);

/**
 * Generates the GHashtable {sat catnr : vector_t array history} where the history
 * array holds the satellites positions through time. index i of array is
 * start_time + (i * time_step). We generate data for time steps until is passes 
 * end_time. 
 * 
 * Also sets sat_hist_len variable through pointer for use in other functions
 */
GHashTable *generate_sat_pos_data(GList *sats_list, guint *sat_hist_len, gdouble start_time, gdouble end_time, gdouble time_step) {
    
    GHashTable *data_fields = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);

    *sat_hist_len = (guint)ceil((end_time - start_time) / time_step);

    for (GList *current = sats_list; current != NULL; current = current->next) {
        sat_t *sat = malloc(sizeof(sat_t));
        memcpy(sat, current->data, sizeof(sat_t));

        gint *cat_nr = malloc(sizeof(gint));
        *cat_nr = sat->tle.catnr;

        vector_t *entries = malloc(*sat_hist_len * sizeof(vector_t));
        
        for (guint i = 0; i < *sat_hist_len; i++) {
            sat_at_time(sat, start_time + (i * time_step));
            entries[i] = sat->pos;
        }

        g_hash_table_insert(data_fields, cat_nr, entries);
        free(sat);
    }

    return data_fields;
}

/**
 * Calculate and set position of satellites to given time.
 * Copies move calc from predict-tools.c without Qth calc
 */
void sat_at_time(sat_t *sat, gdouble time) {

    sat->jul_utc = time;
    sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;

    /* call the norad routines according to the deep-space flag */
    if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
        SDP4(sat, sat->tsince);
    else
        SGP4(sat, sat->tsince);

    Convert_Sat_State(&sat->pos, &sat->vel);
}