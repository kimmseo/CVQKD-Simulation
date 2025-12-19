#include "link-capacity-path.h"
#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"
#include "time-tools.h"
#include "calc-dist-two-sat.h"
#include "skr-utils.h"

gpointer copy_glist_element(gconstpointer src, gpointer user_data) {
    (void)user_data; //unnecessary param
    sat_t *source = (sat_t *)src;
    sat_t *destin = malloc(sizeof(sat_t));
    memcpy(destin, source, sizeof(sat_t));

    return destin;
}

char* save_transfer_info(gpointer satA_ptr, gpointer satB_ptr, gdouble skr) {
    char *satA_name = ((sat_t *)satA_ptr)->nickname;
    char *satB_name = ((sat_t *)satB_ptr)->nickname;

    gdouble jul_time = ((sat_t *)satA_ptr)->jul_utc;
    int time_str_len = 7;
    char *time_str = malloc(7);

    //returns 0 if part of the string would be cutoff.
    while(daynum_to_str(time_str, time_str_len, "%x - %I:%M%p", jul_time) == 0) {
        time_str_len++;
        free(time_str);
        time_str = malloc(time_str_len);
    }
    
    //formats it and saves it to tfr_str string
    int text_length = snprintf(0, 0, "%s - %s time: %s skr: %f\n", satA_name, satB_name, time_str, skr);
    char *tfr_str = malloc(text_length + 1);
    snprintf(tfr_str, text_length + 1, "%s - %s time: %s skr: %f\n", satA_name, satB_name, time_str, skr);
    
    return tfr_str;
}

void save_sat_close_approach(GList *sats, gdouble limit, GHashTable *history) {
    GList *current = sats;

    //doesnt work when i = 0 and j=1. result gkey = 1
    //fix: just start at 1 instead.
    gint i = 1;

    while (current != NULL) {
        GList *check = current->next;
        gint j = i+1;

        while (check != NULL) {
            // ideally get distance first and only calc skr if close enough
            // but calc for what distance skr > 0 is troublesome
            // performance improvement maybe
            gdouble skr = inter_sat_link(current->data, check->data);

            //Do this better when doing iterative Simpson
            //might cause collisions if 1000 satellites
            gint tfr_key = i * 1000 + j;
            gpointer tfr_value = g_hash_table_lookup(history, &tfr_key);
            
            //skr_history has past transfer and this step has transfer
            // --> add transfer to history
            if (tfr_value != NULL && skr >= limit) {
                char *tfr_str = save_transfer_info(current->data, check->data, skr);
        
                g_string_append(tfr_value, tfr_str);

                //free memory allocated in save_transfer_info
                free(tfr_str);
            
            //skr_history has past transfer and this step has no transfer
            //--> output the string and remove transfer history
            } else if (tfr_value != NULL && skr < limit) {

                GString *hist_str = (GString *)tfr_value;
 
                puts(hist_str->str); 
                
                free(tfr_value);
                g_hash_table_remove(history, &tfr_key);

            //skr_history has no past transfer and this step has transfer
            //--> start new history and add the transfer
            } else if (tfr_value == NULL && skr >= limit) {
                
                int tfr_key_len = snprintf(0, 0, "transfer key: %d\n", tfr_key);
                char *stringed_tfr_key = malloc(tfr_key_len+1);
                snprintf(stringed_tfr_key, tfr_key_len+1, "transfer key: %d\n", tfr_key);
                GString *newHistory = g_string_new(stringed_tfr_key);
                
                free(stringed_tfr_key);

                char *tfr_str = save_transfer_info(current->data, check->data, skr);
                g_string_append(newHistory, tfr_str);
                g_hash_table_insert(history, &tfr_key, newHistory);
            
            }

            //else skr_history has no past and this step has no transfer
            //--> do nothing


            check = check->next;
            j++;
        }

        current = current->next;
        i++; 
    }
}

void print_transfer_connections(GHashTable *sats, double t_start, double t_end, double time_step) {
    GList *const_sats = g_hash_table_get_values(sats);

    //copy satellite values
    GList *mut_sats = g_list_copy_deep(const_sats, copy_glist_element, NULL);
    
    //keys are string satA-NickName + satB-NickName, value is gstring formatted to print
    GHashTable *skrHistory = g_hash_table_new(g_int_hash, g_int_equal);

    gdouble skr_limit = 1000;

    for (double time = t_start; time < t_end; time += time_step) {
        
        sat_at_time(mut_sats, time);
        save_sat_close_approach(mut_sats, skr_limit, skrHistory);
    } 

    //output transfer histories that didnt't end before t_end
    puts("================================================= T_END HERE\n");

    GList *current = g_hash_table_get_values(skrHistory);    

    while (current != NULL) {
        GString *transfer_hist = (GString *)current->data;
        puts(transfer_hist->str);
        free(transfer_hist);

        current = current->next;
    }

    g_hash_table_destroy(skrHistory);
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