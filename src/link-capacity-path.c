#include <glib/gi18n.h>
#include "sgpsdp/sgp4sdp4.h"

gpointer copy_glist_element(gconstpointer src, gpointer user_data) {
    sat_t *source = (sat_t *)src;
    sat_t *destin = malloc(sizeof(sat_t));
    memcpy(destin, source, sizeof(sat_t));

    return destin;
}

void link_capacity_path(GHashTable *sats) {
    GList *const_sats = g_hash_table_get_values(sats);
    GList *mut_sats = g_list_copy_deep(const_sats, copy_glist_element, NULL);

    sat_t *firstSat = (sat_t *)g_list_first(const_sats)->data;
    printf("const sat orgininal alt: %f\n", firstSat->alt);
    printf("mod sat original alt: %f\n", firstSat->alt);

    sat_t *mod_sat = g_list_first(mut_sats)->data;
    mod_sat->alt = 404;

    printf("const sat alt after change: %f\n", firstSat->alt);
    printf("mod sat alt after change: %f\n", mod_sat->alt);

}