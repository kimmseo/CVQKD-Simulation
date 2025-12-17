#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"
#include "calc-dist-two-sat.h"
#include "skr-utils.h"
#include "link-capacity-path.h"

gpointer copy_glist_element(gconstpointer src, gpointer user_data) {
    sat_t *source = (sat_t *)src;
    sat_t *destin = malloc(sizeof(sat_t));
    memcpy(destin, source, sizeof(sat_t));

    return destin;
}

void print_readable_time(gdouble time) {
    char msg[50];
    char fmt[] = "%x - %I:%M%p";
    daynum_to_str(msg, 50, fmt, time);

    printf("%s", msg);
}

void sat_close_approach(GList *sats, gdouble limit) {
    GList *current = sats;

    gboolean has = FALSE;

    while (current != NULL) {
        GList *check = current->next;

        while (check != NULL) {
            // ideally get distance first and only calc skr if close enough
            // but calc for what distance skr > 0 is troublesome
            // potential performance improvement maybe
            gdouble skr = inter_sat_link(current->data, check->data);
            if (skr >= limit) {
                print_readable_time(((sat_t *)current->data)->jul_utc);
                printf(" %s and %s skr in bits/second: %f\n", ((sat_t *)current->data)->nickname, 
                                                        ((sat_t *)check->data)->nickname, 
                                                        skr); 

                has = TRUE;
            }

            check = check->next;
        }

        current = current->next; 
    }
    if (has) printf("\n\n");
}

void link_capacity_path(GHashTable *sats, double t_start, double t_end, double time_step) {
    GList *const_sats = g_hash_table_get_values(sats);

    //copy satellite values
    GList *mut_sats = g_list_copy_deep(const_sats, copy_glist_element, NULL);

    for (double time = t_start; time < t_end; time += time_step) {
        
        sat_at_time(mut_sats, time);
        sat_close_approach(mut_sats, 1000);
    } 
 
}

void sat_at_time(GList *sats, gdouble time) {
    GList *current = sats;

    while (current != NULL) {
        sat_t *sat = current->data; 

        sat->jul_utc = time;
        sat->tsince = (sat->jul_utc - sat->jul_epoch) * xmnpda;

        /* call the norad routines according to the deep-space flag */
        if (sat->flags & DEEP_SPACE_EPHEM_FLAG)
            SDP4(sat, sat->tsince);
        else
            SGP4(sat, sat->tsince);

        Convert_Sat_State(&sat->pos, &sat->vel);

        current = current->next;
    }
    
}