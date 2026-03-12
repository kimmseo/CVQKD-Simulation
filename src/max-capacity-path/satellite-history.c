#include <glib/gi18n.h>
#include "path-util.h"
#include "../qth-data.h"

/**
 * Generates the GHashtable {gint catnr : lw_sat_t[] history} where the history
 * array holds the satellites positions through time. index i of array is
 * start_time + (i * time_step). We generate data for time steps until is passes 
 * end_time. 
 * 
 * Also sets sat_hist_len variable through pointer for use in other functions
 */
GHashTable *generate_sat_pos_data(GSList *sats_list, guint *sat_hist_len, gdouble start_time, gdouble end_time, gdouble time_step) {
    
    GHashTable *data_fields = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);

    *sat_hist_len = (guint)ceil((end_time - start_time) / time_step);

    for (GSList *current = sats_list; current != NULL; current = current->next) {
        sat_t sat = *(sat_t *)current->data;

        gint *cat_nr = malloc(sizeof(gint));
        *cat_nr = sat.tle.catnr;

        lw_sat_t *entries = malloc(*sat_hist_len * sizeof(lw_sat_t));

        for (guint i = 0; i < *sat_hist_len; i++) {
            entries[i] = sat_at_time(&sat, start_time + (i * time_step));
        }

        g_hash_table_insert(data_fields, cat_nr, entries);
    }

    return data_fields;
}