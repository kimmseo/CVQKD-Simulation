#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <ctype.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-popup-common.h"
#include "gtk-max-path-view.h"
#include "locator.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sat-pass-dialogs.h"
#include "time-tools.h"
#include "skr-utils.h"

#include "max-capacity-path/link-capacity-path.h"
#include "max-capacity-path/satellite-history.h"


/* Column titles indexed with column symb. refs */
const gchar     *MAX_PATH_VIEW_FIELD_TITLE[MAX_PATH_VIEW_FIELD_NUMBER] = {
    N_("Azimuth"),
    N_("Elevation"),
    N_("Direction"),
    N_("Right Asc."),
    N_("Declination"),
    N_("SSP Lat."),
    N_("SSP Lon."),
    N_("SSP Loc."),
    N_("Footprint"),
    N_("Altitude"),
    N_("Velocity"),
    N_("Doppler@100M"),
    N_("Sig. Loss"),
    N_("Mean Anom."),
    N_("Orbit Phase"),
    N_("Orbit Num."),
};

const gchar     *MAX_PATH_VIEW_FIELD_HINT[MAX_PATH_VIEW_FIELD_NUMBER] = {
    N_("Azimuth of the satellite"),
    N_("Elevation of the satellite"),
    N_("Direction of the satellite"),
    N_("Right Ascension of the satellite"),
    N_("Declination of the satellite"),
    N_("Latitude of the sub-satellite point"),
    N_("Longitude of the sub-satellite point"),
    N_("Sub-Satellite Point as Maidenhead grid square"),
    N_("Diameter of the satellite footprint"),
    N_("Altitude of the satellite"),
    N_("Tangential velocity of the satellite"),
    N_("Doppler Shift @ 100MHz"),
    N_("Signal loss @ 100MHz"),
    N_("Mean Anomaly"),
    N_("Orbit Phase"),
    N_("Orbit Number"),
};

static GtkBoxClass *parent_class = NULL;

static void gtk_max_path_view_destroy(GtkWidget * widget)
{
    GtkMaxPathView      *msat = GTK_MAX_PATH_VIEW(widget);
    guint i;
    gint satlist[msat->dyn_num_sat];
    
    //Free memory (gpredict call this function twice for some reason
    // so we use sanity check before free)
    if (msat->dyn_num_sat != 0) { 

        for (i = 0; i < msat->dyn_num_sat; i++)
        {
            sat_t *sat = SAT(g_slist_nth_data(msat->sats, g_array_index(msat->selected, guint, i)));
            if (sat != NULL)
            {
                satlist[i] = sat->tle.catnr;
            }
            else
            {
                satlist[i] = 0;
            }
        }
        
        g_key_file_set_integer_list(msat->cfgdata, MOD_CFG_MAX_PATH_VIEW_SECTION,
                                    MOD_CFG_MAX_PATH_VIEW_SELECT, satlist,
                                    msat->dyn_num_sat);

        g_array_free(msat->selected, TRUE);

        msat->dyn_num_sat = 0;
    }

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_max_path_view_class_init(GtkMaxPathViewClass * class,
                                        gpointer class_data)
{
    GtkWidgetClass *widget_class;

    (void)class_data;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_max_path_view_destroy;
    parent_class = g_type_class_peek_parent(class);
}

static void gtk_max_path_view_init(GtkMaxPathView * list, gpointer g_class)
{
    (void)list;
    (void)g_class; 
}

/* Update a field in the GtkMultipleSat view */
static void update_field(GtkMaxPathView * msat, guint i, guint index)
{
    sat_t       *sat;
    gchar       *buff = NULL;
    gchar       hmf = ' ';
    gdouble     number;
    gint        retcode;
        
    // Get selected satellite

    //sat = SAT(g_slist_nth_data(msat->sats, msat->selected[index]));
    sat = SAT(g_slist_nth_data(msat->sats, g_array_index(msat->selected, guint, index)));

    if (!sat)
    {
        /*
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    _("%s:%d: Can not update non-existing sat"),
                    __FILE__, __LINE__);
        */
        // Satellite is NULL, set label to NULL
        buff = g_strdup("NULL");
    }
    else
    {
        // Update requested field
        switch (i)
        {
        case MAX_PATH_VIEW_FIELD_AZ:
            buff = g_strdup_printf("%6.2f\302\260", sat->az);
            break;
        case MAX_PATH_VIEW_FIELD_EL:
            buff = g_strdup_printf("%6.2f\302\260", sat->el);
            break;
        case MAX_PATH_VIEW_FIELD_DIR:
            if (sat->otype == ORBIT_TYPE_GEO)
            {
                buff = g_strdup("Geostationary");
            }
            else if (decayed(sat))
            {
                buff = g_strdup("Decayed");
            }
            else if (sat->range_rate > 0.0)
            {
                /* Receding */
                buff = g_strdup("Receding");
            }
            else if (sat->range_rate < 0.0)
            {
                /* Approaching */
                buff = g_strdup("Approaching");
            }
            else
            {
                buff = g_strdup("N/A");
            }
            break;
        case MAX_PATH_VIEW_FIELD_RA:
            buff = g_strdup_printf("%6.2f\302\260", sat->ra);
            break;
        case MAX_PATH_VIEW_FIELD_DEC:
            buff = g_strdup_printf("%6.2f\302\260", sat->dec);
            break;
        case MAX_PATH_VIEW_FIELD_LAT:
            number = sat->ssplat;
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
            {
                if (number < 0.00)
                {
                    number = -number;
                    hmf = 'S';
                }
                else
                {
                    hmf = 'N';
                }
            }
            buff = g_strdup_printf("%.2f\302\260%c", number, hmf);
            break;
        case MAX_PATH_VIEW_FIELD_LON:
            number = sat->ssplon;
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_NSEW))
            {
                if (number < 0.00)
                {
                    number = -number;
                    hmf = 'W';
                }
                else
                {
                    hmf = 'E';
                }
            }
            buff = g_strdup_printf("%.2f\302\260%c", number, hmf);
            break;
        case MAX_PATH_VIEW_FIELD_SSP:
            /* SSP locator */
            buff = g_try_malloc(7);
            retcode = longlat2locator(sat->ssplon, sat->ssplat, buff, 3);
            if (retcode == RIG_OK)
            {
                buff[6] = '\0';
            }
            else
            {
                g_free(buff);
                buff = NULL;
            }

            break;
        case MAX_PATH_VIEW_FIELD_FOOTPRINT:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            {
                buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->footprint));
            }
            else
            {
                buff = g_strdup_printf("%.0f km", sat->footprint);
            }
            break;
        case MAX_PATH_VIEW_FIELD_ALT:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
                buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->alt));
            else
                buff = g_strdup_printf("%.0f km", sat->alt);
            break;
        case MAX_PATH_VIEW_FIELD_VEL:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
                buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->velo));
            else
                buff = g_strdup_printf("%.3f km/sec", sat->velo);
            break;
        case MAX_PATH_VIEW_FIELD_DOPPLER:
            number = -100.0e06 * (sat->range_rate / 299792.4580);   // Hz
            buff = g_strdup_printf("%.0f Hz", number);
            break;
        case MAX_PATH_VIEW_FIELD_LOSS:
            number = 72.4 + 20.0 * log10(sat->range);       // dB
            buff = g_strdup_printf("%.2f dB", number);
            break;
        case MAX_PATH_VIEW_FIELD_MA:
            buff = g_strdup_printf("%.2f\302\260", sat->ma);
            break;
        case MAX_PATH_VIEW_FIELD_PHASE:
            buff = g_strdup_printf("%.2f\302\260", sat->phase);
            break;
        case MAX_PATH_VIEW_FIELD_ORBIT:
            buff = g_strdup_printf("%ld", sat->orbit);
            break;
        default:
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s:%d: Invalid field number (%d)"),
                        __FILE__, __LINE__, i);
            break;
        }
    }
}

static gint sat_name_compare (sat_t * a, sat_t * b)
{
    return gpredict_strcmp(a->nickname, b->nickname);
}

/* Copy satellite from hash table to singly linked list */
static void store_sats(gpointer key, gpointer value, gpointer user_data)
{
    GtkMaxPathView      *max_path_view = GTK_MAX_PATH_VIEW(user_data);
    sat_t               *sat = SAT(value);

    (void)key;

    max_path_view->sats = g_slist_insert_sorted(max_path_view->sats, sat,
                                               (GCompareFunc) sat_name_compare);
}

static void Calculate_RADec(sat_t * sat, qth_t * qth, obs_astro_t * obs_set)
{
    /* Reference:  Methods of Orbit Determination by  */
    /*                Pedro Ramon Escobal, pp. 401-402 */
    double          phi, theta, sin_theta, cos_theta, sin_phi, cos_phi,
        az, el, Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz,
        Lx, Ly, Lz, cos_delta, sin_alpha, cos_alpha;
    geodetic_t      geodetic;

    geodetic.lon = qth->lon * de2ra;
    geodetic.lat = qth->lat * de2ra;
    geodetic.alt = qth->alt / 1000.0;
    geodetic.theta = 0;

    az = sat->az * de2ra;
    el = sat->el * de2ra;
    phi = geodetic.lat;
    theta = FMod2p(ThetaG_JD(sat->jul_utc) + geodetic.lon);
    sin_theta = sin(theta);
    cos_theta = cos(theta);
    sin_phi = sin(phi);
    cos_phi = cos(phi);
    Lxh = -cos(az) * cos(el);
    Lyh = sin(az) * cos(el);
    Lzh = sin(el);
    Sx = sin_phi * cos_theta;
    Ex = -sin_theta;
    Zx = cos_theta * cos_phi;
    Sy = sin_phi * sin_theta;
    Ey = cos_theta;
    Zy = sin_theta * cos_phi;
    Sz = -cos_phi;
    Ez = 0;
    Zz = sin_phi;
    Lx = Sx * Lxh + Ex * Lyh + Zx * Lzh;
    Ly = Sy * Lxh + Ey * Lyh + Zy * Lzh;
    Lz = Sz * Lxh + Ez * Lyh + Zz * Lzh;
    obs_set->dec = ArcSin(Lz);  /* Declination (radians) */
    cos_delta = sqrt(1 - Sqr(Lz));
    sin_alpha = Ly / cos_delta;
    cos_alpha = Lx / cos_delta;
    obs_set->ra = AcTan(sin_alpha, cos_alpha);  /* Right Ascension (radians) */
    obs_set->ra = FMod2p(obs_set->ra);
}

/* Refresh internal references to the satellites */
void gtk_max_path_view_reload_sats(GtkWidget * max_path_view, GHashTable * sats)
{
    // Free GSLists
    g_slist_free(GTK_MAX_PATH_VIEW(max_path_view)->sats);
    GTK_MAX_PATH_VIEW(max_path_view)->sats = NULL;

    // Reload satellites
    g_hash_table_foreach(sats, store_sats, max_path_view);
}

/* Reload configuration */
void gtk_max_path_view_reconf(GtkWidget * widget,
                              GKeyFile * newcfg,
                              GHashTable * sats, qth_t * qth, gboolean local)
{
    guint32         fields;

    // Store pointer to new cfg data
    GTK_MAX_PATH_VIEW(widget)->cfgdata = newcfg;

    // Get visible fields from new configuration
    fields = mod_cfg_get_int(newcfg,
                             MOD_CFG_MAX_PATH_VIEW_SECTION,
                             MOD_CFG_MAX_PATH_VIEW_FIELDS,
                             SAT_CFG_INT_MAX_PATH_VIEW_FIELDS);
    
    if (fields != GTK_MAX_PATH_VIEW(widget)->flags)
    {
        GTK_MAX_PATH_VIEW(widget)->flags = fields;
    }

    // If this is a local reconfiguration sats may have changed
    if (local)
    {
        gtk_max_path_view_reload_sats(widget, sats);
    }

    // QTH may have changed too single we have a default QTH
    GTK_MAX_PATH_VIEW(widget)->qth = qth;

    // Get refresh rate and cycle counter
    GTK_MAX_PATH_VIEW(widget)->refresh = mod_cfg_get_int(newcfg,
                                                        MOD_CFG_MAX_PATH_VIEW_SECTION,
                                                        MOD_CFG_MAX_PATH_VIEW_REFRESH,
                                                        SAT_CFG_INT_MAX_PATH_VIEW_REFRESH);
    GTK_MAX_PATH_VIEW(widget)->counter = 1;
}


// Update satellites
void gtk_max_path_view_update(GtkWidget * widget, guint index)
{
    GtkMaxPathView      *msat = GTK_MAX_PATH_VIEW(widget);
    guint               i;

    // Sanity checks
    if ((msat == NULL) || !IS_GTK_MAX_PATH_VIEW(msat))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Invalid GtkMultipleSat!"), __func__);
        //return;
    }

    // Check refresh rate
    // have integer overflow, temporarily setting refresh to 1
    msat->refresh = 1;
    if (msat->counter < msat->refresh)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: counter = %u, refresh = %u", __FILE__, __LINE__, msat->counter, msat->refresh);
        msat->counter++;
    }
    else
    {
        // Calculate here to avoid double calculation
        if ((msat->flags & MAX_PATH_VIEW_FLAG_RA) ||
            (msat->flags & MAX_PATH_VIEW_FLAG_DEC))
        {
            obs_astro_t     astro;
            sat_t           *sat =
                //SAT(g_slist_nth_data(msat->sats, msat->selected[index]));
                SAT(g_slist_nth_data(msat->sats, g_array_index(msat->selected, guint, index)));
 
            Calculate_RADec(sat, msat->qth, &astro);
            sat->ra = Degrees(astro.ra);
            sat->dec = Degrees(astro.dec);
        }

        // Update visible fields one by one
        for (i = 0; i < MAX_PATH_VIEW_FIELD_NUMBER; i++)
        {
            if (msat->flags & (1 << i))
            {
                //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: checking... index = %d, i = %d", __FILE__, __LINE__, index, i);
                update_field(msat, i, index);
            }
        }
        msat->counter = 1;
    }
}

GType gtk_max_path_view_get_type()
{
    static GType    gtk_max_path_view_type = 0;

    if (!gtk_max_path_view_type)
    {
        static const GTypeInfo gtk_max_path_view_info = {
            sizeof(GtkMaxPathViewClass),
            NULL,                   // base_init
            NULL,                   // base_finalize
            (GClassInitFunc) gtk_max_path_view_class_init,
            NULL,                   // class_finalize
            NULL,                   // class_data
            sizeof(GtkMaxPathView),
            5,                      // n_preallocs
            (GInstanceInitFunc) gtk_max_path_view_init,
            NULL
        };
        
        gtk_max_path_view_type = g_type_register_static(GTK_TYPE_BOX,
                                                       "GtkMaxPathView",
                                                       &gtk_max_path_view_info, 0);
    }

    return gtk_max_path_view_type;
}


gdouble get_overlay_values(GtkWidget *overlay, gchar **text) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(overlay));
    gdouble value = 0;
   
    for (GList *child = children; child != NULL; child = child->next) {
        GtkWidget *w_child = (GtkWidget *)child->data;
        if (IS_GTK_COMBO_BOX_TEXT(w_child)) {
            *text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w_child));
        } else if (IS_GTK_SPIN_BUTTON(w_child)) {
            value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(w_child));
        } 
    }

    return value;
}

MaxSearchParams *get_path_search_fields(GtkWidget *controls) {
    MaxSearchParams *params = malloc(sizeof(MaxSearchParams));

    GtkWidget *src_select = gtk_grid_get_child_at(GTK_GRID(controls), 1, 0);
    params->src = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(src_select));
    if (params->src == NULL) return NULL;

    GtkWidget *dst_select = gtk_grid_get_child_at(GTK_GRID(controls), 1, 1);
    params->dst = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(dst_select));
    if (params->dst == NULL) return NULL;

    GtkWidget *overlay = gtk_grid_get_child_at(GTK_GRID(controls), 1, 2); 
    gchar *unit = NULL;
    gdouble val = get_overlay_values(overlay, &unit);

    if (unit == NULL) return NULL;
    
    if (strcmp(unit, "  Gb  ") == 0) val *= 1000;
    params->max_data = val;

    GtkWidget *time_start = gtk_grid_get_child_at(GTK_GRID(controls), 1, 3);
    params->t_start = strtod(gtk_entry_get_text(GTK_ENTRY(time_start)), NULL);

    GtkWidget *time_end = gtk_grid_get_child_at(GTK_GRID(controls), 1, 5);
    params->t_end = strtod(gtk_entry_get_text(GTK_ENTRY(time_end)), NULL);

    GtkWidget *t_overlay = gtk_grid_get_child_at(GTK_GRID(controls), 1, 7);
    unit = NULL;
    val  = get_overlay_values(t_overlay, &unit);
    if (unit == NULL) return NULL;

    if (strcmp(unit, " Sec  ") == 0) val /= 60;
    //val in minutes divided by # minutes per day = val in days
    params->t_step = val / xmnpda;

    return params;
}

GdkRGBA hsv_2_rgb(gdouble h, gdouble s, gdouble v) {
    gdouble hh, p, q, t, ff;
    glong i;
    GdkRGBA out;

    if (s <= 0.0) {
       return (GdkRGBA){.red=v, .green=v, .blue=v, .alpha=1}; 
    }

    hh = h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (glong)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch (i) {
        case 0:
            out = (GdkRGBA){.red=v, .green=t, .blue=p};
            break;
        case 1:
            out = (GdkRGBA){.red=q, .green=v, .blue=p};
            break;
        case 2:
            out = (GdkRGBA){.red=p, .green=v, .blue=t};
            break;
        case 3:
            out = (GdkRGBA){.red=p, .green=q, .blue=v};
            break;
        case 4:
            out = (GdkRGBA){.red=t, .green=p, .blue=v};
            break;
        case 5:
        default:
            out = (GdkRGBA){.red=v, .green=p, .blue=q};
            break;
    }

    out.alpha = 1.0;
    return out;
}

GList *generate_path_colors(GList *path_colors, GList *path) {

    //ToDo: maybe_g_list_free_full
    if (path_colors != NULL) g_list_free(path_colors);

    guint len = g_list_length(path);
    GdkRGBA *color;

    GList *new_colors = NULL;
    for (gdouble i = 0; i < 360; i += 360 / len) {
        color = malloc(sizeof(GdkRGBA));
        *color = hsv_2_rgb(i, 1.0, 0.8);
        new_colors = g_list_prepend(new_colors, color);
    }

    return new_colors;
}

void calculate_max_capacity_path(GtkWidget *button, gpointer data) {
    UNUSED(button);
    GtkMaxPathView *obj = (GtkMaxPathView *)data;

    if (obj->max_capacity_path != NULL) free(obj->max_capacity_path); 

    MaxSearchParams *search = get_path_search_fields(obj->search_controls);
    if (search == NULL) {
        return;
    }   
    //gdouble is in days, multiply with xmnpda to convert minutes to days
    
    clock_t timer_start, timer_end;
    double cpu_time_used;
    timer_start = clock();

    guint sat_hist_len = 0;
    //GHashtable {gint catnr : lw_sat_t[] history}
    GHashTable *sat_history = generate_sat_pos_data(
        obj->sats, 
        &sat_hist_len, 
        search->t_start, 
        search->t_end, 
        search->t_step);
 
    //returns GList of path_node
    obj->max_capacity_path = get_max_link_path(
        obj->sats, 
        sat_history, 
        sat_hist_len, 
        obj->qths,
        search);

    g_hash_table_destroy(sat_history);

    timer_end = clock();
    cpu_time_used = ((double)(timer_end - timer_start)) / CLOCKS_PER_SEC;
    printf("Function took %f seconds to execute (CPU time).\n", cpu_time_used);

    if (obj->max_capacity_path->size == 0) return;

    obj->path_colors = generate_path_colors(obj->path_colors, obj->max_capacity_path->path);

    g_signal_emit_by_name(obj, "update_path");
}

void update_time_display(GtkWidget *entry, gpointer data) { 
    GtkWidget *display = (GtkWidget *)data;
    
    gdouble val = strtod(gtk_entry_get_text(GTK_ENTRY(entry)), NULL);

    //daynum_to_str can only handle 12 digits before . numbers
    if (val > 999999999) {
        gtk_label_set_text(GTK_LABEL(display), "number too big");
        return;
    }

    char msg[50];
    char fmt[] = "%d/%m/%G - %H:%M";

    daynum_to_str(msg, 50, fmt, val);

    gtk_label_set_text(GTK_LABEL(display), msg);
}

gboolean entry_lost_focus_cb(GtkWidget *self, GdkEventFocus *event, gpointer user_data) {
    UNUSED(event);
    update_time_display(self, user_data);

    return FALSE;
}

void float_only_input(GtkEditable *editable, const gchar *text, gint length, gint *position, gpointer data) {
    UNUSED(position);
    UNUSED(data);

    int already_dot = FALSE;

    gchar *cur_text = gtk_editable_get_chars(editable, 0, -1);
    for (int i = 0; cur_text[i] != '\0'; i++) {
        if (cur_text[i] == '.') {
            already_dot = TRUE;
            break;
        }
    }

    for (int i = 0; i < length; i++) { 
        if ((text[i] == '.' && already_dot) ||
            (text[i] != '.' && !isdigit(text[i]))) {
            g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
            return;
        }
    }
}

static void expander_activate_cb(GtkExpander* self, gpointer user_data) {
    (void) user_data;

    gboolean expanded = gtk_expander_get_expanded(self);
    
    for (GList *child = gtk_container_get_children(GTK_CONTAINER(self));
        child != NULL; 
        child = child->next) {
        g_object_set(child->data, "vexpand", !expanded, NULL);
    }
}

GtkWidget *gen_SAT_display(GSList *sats) {
    gchar *text;
    GtkWidget *label; 

    GdkRGBA gdk_rgba;
    gint rgba = sat_cfg_get_int(SAT_CFG_INT_MAP_SAT_COL);
    rgba_from_cfg(rgba, &gdk_rgba);
    gdk_rgba.alpha = 0.2;

    GtkWidget *display_SAT = gtk_expander_new(NULL);
    label = gtk_label_new(NULL);
    text = g_markup_printf_escaped("<b>Satellites</b>");
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_expander_set_label_widget(GTK_EXPANDER(display_SAT), label);
    g_free(text);

    g_signal_connect(display_SAT, "activate", G_CALLBACK(expander_activate_cb), NULL);
   
   GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(display_SAT), scroll);
    g_object_set(scroll, "margin", 5, NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_container_add(GTK_CONTAINER(scroll), grid);
   
    int i = 0;
    for (GSList *iter = sats; iter != NULL; iter = iter->next) {
        sat_t *sat = (sat_t *)iter->data;

        text = g_markup_printf_escaped("<u>%s</u>", sat->name);
        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), text);
        GtkWidget *sat_frame = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(sat_frame), label);

        gtk_widget_override_background_color(sat_frame, GTK_STATE_FLAG_NORMAL, 
            &gdk_rgba);

        g_free(text);
        
        gtk_grid_attach(GTK_GRID(grid), sat_frame, 0, i, 1, 1);
        i++;
    }

    return display_SAT;
}

GtkWidget *gen_OGS_display(GSList *qths) {
    gchar *text;
    GtkWidget *label;

    GdkRGBA gdk_rgba;
    gint rgba = sat_cfg_get_int(SAT_CFG_INT_MAP_QTH_COL);
    rgba_from_cfg(rgba, &gdk_rgba);
    gdk_rgba.alpha = 0.2;

    GtkWidget *display_OGS = gtk_expander_new(NULL);
    label = gtk_label_new(NULL);
    text = g_markup_printf_escaped("<b>Optical Ground Stations</b>");
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_expander_set_label_widget(GTK_EXPANDER(display_OGS), label); 
    g_signal_connect(display_OGS, "activate", G_CALLBACK(expander_activate_cb), NULL);
    g_free(text);
   
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(display_OGS), scroll);
    g_object_set(scroll, "margin", 5, NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_container_add(GTK_CONTAINER(scroll), grid);
   
    int i = 0;
    for (GSList *iter = qths; iter != NULL; iter = iter->next) {
        qth_t *ogs = (qth_t *)iter->data;
       
        GtkWidget *ogs_frame = gtk_frame_new(NULL);
        gtk_widget_override_background_color(ogs_frame, GTK_STATE_FLAG_NORMAL, 
            &gdk_rgba);

        GtkWidget *ogs_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); 
        gtk_container_add(GTK_CONTAINER(ogs_frame), ogs_box);

        GtkWidget *name_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start(GTK_BOX(ogs_box), name_info, TRUE, TRUE, 10);
        gtk_widget_set_halign(name_info, GTK_ALIGN_END);

        text = g_markup_printf_escaped("<u>%s</u>", ogs->name);
        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), text);
        gtk_box_pack_start(GTK_BOX(name_info), label, TRUE, TRUE, 0); 
        g_free(text);

        label = gtk_label_new(ogs->loc);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_box_pack_start(GTK_BOX(name_info), label, TRUE, TRUE, 0); 
        
        label = gtk_label_new(ogs->desc);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_box_pack_start(GTK_BOX(name_info), label, TRUE, TRUE, 0); 

        GtkWidget *pos_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_end(GTK_BOX(ogs_box), pos_info, TRUE, TRUE, 10);
        gtk_widget_set_halign(pos_info, GTK_ALIGN_END);
        gtk_widget_set_size_request(pos_info, 100, -1);
        
        text = g_strdup_printf("Alt: %i m", ogs->alt);
        label = gtk_label_new(text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(pos_info), label, FALSE, FALSE, 0);
        g_free(text);

        text = g_strdup_printf("Lat: %.2f° N", ogs->lat);
        label = gtk_label_new(text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(pos_info), label, FALSE, FALSE, 0); 
        g_free(text);

        text = g_strdup_printf("Lon: %.2f° E", ogs->lon);
        label = gtk_label_new(text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(pos_info), label, FALSE, FALSE, 0); 
        g_free(text);

        text = g_strdup_printf("WX: %s", ogs->wx);
        label = gtk_label_new(text);
        gtk_label_set_xalign(GTK_LABEL(label), 0.0);
        gtk_box_pack_start(GTK_BOX(pos_info), label, FALSE, FALSE, 0); 
        g_free(text);

        gtk_grid_attach(GTK_GRID(grid), ogs_frame, 0, i, 1, 1);
        i++;
    }

    return display_OGS;
}

GtkWidget *gen_path_display() {
    GtkWidget *display_path = gtk_expander_new(NULL);
    gchar *text = g_markup_printf_escaped("<b>Result</b>");
    GtkWidget *label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), text);
    gtk_expander_set_label_widget(GTK_EXPANDER(display_path), label); 
    g_free(text);

    g_signal_connect(display_path, "activate", G_CALLBACK(expander_activate_cb), NULL);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(display_path), scroll);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(scroll), grid);

    return display_path;
}

GtkWidget *new_path_grid_panel(path_node *node, GdkRGBA *color) {
    gchar *label;
    gchar *name;
    gdouble alt, lat, lon;
    if (node->type == path_SATELLITE) {
        sat_t sat = *(sat_t*)node->obj;
        label = "Satellite";
        name = ((sat_t *)node->obj)->name;

        sat_at_time(&sat, node->time);
        alt = sat.alt;
        lat = sat.ssplat;
        lon = sat.ssplon;
        
    } else { //node->type == path_STATION
        qth_t ogs = *(qth_t *)node->obj;
        label = "OGS";
        name = ogs.name;
        alt = ogs.alt;
        lat = ogs.lat; 
        lon = ogs.lon; 
    } 

    GtkWidget *frame = gtk_frame_new(label);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_add(GTK_CONTAINER(frame), box);

    //node color indicator
    GtkWidget *indi = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), indi, FALSE, FALSE, 5); 
    gtk_widget_override_background_color(indi, GTK_STATE_FLAG_NORMAL, color);
    gtk_widget_set_size_request(indi, 30, 50);
    

    //satellite name
    GtkWidget *name_date = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), name_date, FALSE, FALSE, 0); 

    gtk_box_pack_start(GTK_BOX(name_date), gtk_label_new(name), FALSE, FALSE, 0);
 
    char fmted_time[50];
    daynum_to_str(fmted_time, 50, "%d/%m/%G - %H:%M", node->time);
    gtk_box_pack_start(GTK_BOX(name_date), gtk_label_new(fmted_time), TRUE, TRUE, 0);   

    //info like time and pos when last transfer happens
    GtkWidget *sat_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(box), sat_info, TRUE, TRUE, 0);

    gchar *str_alt = (node->type == path_SATELLITE ? 
        fmted_to_string("Alt: %.2f km", alt) 
        :fmted_to_string("Alt: %i m", (int)alt));
    gtk_box_pack_start(GTK_BOX(sat_info), gtk_label_new(str_alt), TRUE, TRUE, 0);

    gchar *str_lat = fmted_to_string("Lat: %.2f\u00b0", lat);
    gtk_box_pack_start(GTK_BOX(sat_info), gtk_label_new(str_lat), TRUE, TRUE, 0);

    gchar *str_lon = fmted_to_string("Lon: %.2f\u00b0 \0", lon);
    gtk_box_pack_start(GTK_BOX(sat_info), gtk_label_new(str_lon), TRUE, TRUE, 0);

    //ToDo: change it to local variables??
    free(str_alt);
    free(str_lat);
    free(str_lon);
    return frame;
}

void update_path_display(GtkWidget *button, gpointer data) {
    UNUSED(button);
    GtkMaxPathView *max_path_view = (GtkMaxPathView *)data; 

    GtkWidget *expandable = max_path_view->display_path;
    GtkWidget *scroll = gtk_container_get_children(GTK_CONTAINER(expandable))->data;

    for (GList *iter = gtk_container_get_children(GTK_CONTAINER(scroll));
            iter != NULL; iter = iter->next) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    //press button but max_capacity_path is empty, then 
    if (max_path_view->max_capacity_path == NULL) {
        gtk_container_add(GTK_CONTAINER(scroll), grid);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new("NO_PATH_FOUND"), 0, 0, 1, 1);
    } else {
        gchar *str_data_size = fmted_to_string("Max Data Tranferable: %.2f (kb)", 
            max_path_view->max_capacity_path->size);
        gtk_grid_attach(GTK_GRID(grid), gtk_label_new(str_data_size), 0, 0, 1, 1);

        GList *node = max_path_view->max_capacity_path->path;
        GList *color = max_path_view->path_colors;
        guint node_len = g_list_length(node);
        
        for (guint i = 1; i < node_len+1; i++) {
            gtk_grid_attach(
                GTK_GRID(grid),
                new_path_grid_panel((path_node *)node->data, (GdkRGBA *)color->data),
                0, i, 1, 1);
            node = node->next;
            color = color->next;

        }
    }

    gtk_container_add(GTK_CONTAINER(scroll), grid);

    gtk_widget_show_all(expandable);
}

GtkWidget *gen_search_controls(GtkMaxPathView *max_path_view) {
    GtkWidget *label;
    GtkWidget *controls = gtk_grid_new();

    gtk_grid_set_column_spacing(GTK_GRID(controls), 5);
    gtk_grid_set_row_spacing(GTK_GRID(controls), 5);
    gtk_container_set_border_width(GTK_CONTAINER(controls), 10);

    //start location
    label = gtk_label_new("Source:");
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(controls), label, 0, 0, 1, 1);

    GtkWidget *src_select =  gtk_combo_box_text_new();
    g_object_set(src_select, "hexpand", TRUE, NULL);
    gtk_grid_attach(GTK_GRID(controls), src_select, 1, 0, 3, 1);

    //end location
    label = gtk_label_new("Destination:");
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(controls), label, 0, 1, 1, 1);

    GtkWidget *dst_select =  gtk_combo_box_text_new();
    g_object_set(dst_select, "hexpand", TRUE, NULL);
    gtk_grid_attach(GTK_GRID(controls), dst_select, 1, 1, 3, 1);

    //Add names to drop down selectors
    for (GSList *i = max_path_view->qths; i != NULL; i=i->next) {
        qth_t *station = (qth_t *)i->data;
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(src_select), station->name);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dst_select), station->name);
    }
   
    //max data size
    label = gtk_label_new("Max Data:");
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(controls), label, 0, 2, 1, 1);

    GtkWidget *data = gtk_spin_button_new_with_range(0.00, G_MAXDOUBLE - 10, 0.0001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(data), TRUE);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(data), 10, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(data), 3);

    GtkWidget *kb_vs_gb= gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kb_vs_gb), "  Gb  ");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(kb_vs_gb), "  Kb  ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(kb_vs_gb), 0);
    gtk_widget_set_halign(kb_vs_gb, GTK_ALIGN_END);

    GtkWidget *overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(overlay), data);
    gtk_overlay_add_overlay(GTK_OVERLAY(overlay), kb_vs_gb);
    
    gtk_grid_attach(GTK_GRID(controls), overlay, 1, 2, 3, 1);

    gdouble daynum_val = get_current_daynum();
    char daynum_str[20];
    snprintf(daynum_str, 20, "%f", daynum_val);
    
    char daynum_fmted[50];
    char fmt[] = "%d/%m/%G - %H:%M";
    daynum_to_str(daynum_fmted, 50, fmt, daynum_val);

    //time start
    label = gtk_label_new("time start:");
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(controls), label, 0, 3, 1, 1);

    GtkWidget *start_display = gtk_label_new(daynum_fmted);
    gtk_grid_attach(GTK_GRID(controls), start_display, 1, 4, 3, 1);

    GtkWidget *time_start = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(time_start), daynum_str);
    g_signal_connect(time_start, "insert-text", G_CALLBACK(float_only_input), NULL);
    g_signal_connect(time_start, "focus-out-event", G_CALLBACK(entry_lost_focus_cb), start_display);
    g_signal_connect(time_start, "activate", G_CALLBACK(update_time_display), start_display);
    gtk_grid_attach(GTK_GRID(controls), time_start, 1, 3, 3, 1);

    //time end
    label = gtk_label_new("time end:");
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(controls), label, 0, 5, 1, 1);

    GtkWidget *end_display = gtk_label_new(daynum_fmted);
    gtk_grid_attach(GTK_GRID(controls), end_display, 1, 6, 3, 1);

    GtkWidget *time_end = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(time_end), daynum_str);
    g_signal_connect(time_end, "insert-text", G_CALLBACK(float_only_input), NULL);
    g_signal_connect(time_end, "focus-out-event", G_CALLBACK(entry_lost_focus_cb), end_display);
    g_signal_connect(time_end, "activate", G_CALLBACK(update_time_display), end_display);
    gtk_grid_attach(GTK_GRID(controls), time_end, 1, 5, 3, 1);

    //time steps
    label = gtk_label_new("Step Size:"); 
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_grid_attach(GTK_GRID(controls), label, 0, 7, 1, 1);

    GtkWidget *time_step = gtk_spin_button_new_with_range(0.00, G_MAXDOUBLE - 10, 0.0001);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(time_step), TRUE);
    gtk_spin_button_set_increments(GTK_SPIN_BUTTON(time_step), 10, 1.0);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(time_step), 3);

    GtkWidget *sec_vs_min = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sec_vs_min), " Min  ");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sec_vs_min), " Sec  ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(sec_vs_min), 0);
    gtk_widget_set_halign(sec_vs_min, GTK_ALIGN_END);

    GtkWidget *t_overlay = gtk_overlay_new();
    gtk_container_add(GTK_CONTAINER(t_overlay), time_step);
    gtk_overlay_add_overlay(GTK_OVERLAY(t_overlay), sec_vs_min);
    
    gtk_grid_attach(GTK_GRID(controls), t_overlay, 1, 7, 3, 1);

    //button
    GtkWidget *button = gtk_button_new_with_label("Calculate Path");
    gtk_grid_attach(GTK_GRID(controls), button, 0, 8, 4, 1);

    g_signal_new("update-path", G_TYPE_FROM_INSTANCE(max_path_view),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(calculate_max_capacity_path), max_path_view);
    g_signal_connect(button, "clicked", G_CALLBACK(update_path_display), max_path_view);
    
    return controls;
}

static gint cmp_sat_by_name_cb(gconstpointer a, gconstpointer b) {
    sat_t *s_a = (sat_t *)a;
    sat_t *s_b = (sat_t *)b;

    return strcmp(s_a->name, s_b->name);
}


GtkWidget *gtk_max_path_view_new(GKeyFile * cfgdata, GHashTable * sats,
                                GSList *qths, qth_t *qth, guint32 fields)
{
    GtkWidget       *widget;
    GtkMaxPathView  *max_path_view;
    guint           i;

    widget = g_object_new(GTK_TYPE_MAX_PATH_VIEW, NULL);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(widget),
                                   GTK_ORIENTATION_VERTICAL);
    max_path_view = GTK_MAX_PATH_VIEW(widget);

    max_path_view->update = gtk_max_path_view_update; 

    // Read configuration data
    /* ... */

    g_hash_table_foreach(sats, store_sats, widget);

    max_path_view->sats = g_slist_sort(max_path_view->sats, (GCompareFunc)cmp_sat_by_name_cb);

    max_path_view->dyn_num_sat = g_slist_length(max_path_view->sats);

    max_path_view->selected = g_array_sized_new(FALSE, TRUE, sizeof(guint), max_path_view->dyn_num_sat);
 
    for (i = 0; i < max_path_view->dyn_num_sat; i++)
    {
        g_array_append_val(max_path_view->selected, i);
    }

    max_path_view->qths = qths;
    max_path_view->qth = qth;

    max_path_view->path_colors = NULL;
    max_path_view->max_capacity_path = NULL;
    
    max_path_view->cfgdata = cfgdata;

    // Initialise column flags
    if (fields > 0)
    {
        max_path_view->flags = fields;
    }
    else
    {
        max_path_view->flags = mod_cfg_get_int(cfgdata,
                                              MOD_CFG_MAX_PATH_VIEW_SECTION,
                                              MOD_CFG_MAX_PATH_VIEW_FIELDS,
                                              SAT_CFG_INT_MAX_PATH_VIEW_FIELDS);
    }

    // Get refresh rate and cycle counter
    max_path_view->refresh = mod_cfg_get_int(cfgdata,
                                            MOD_CFG_MAX_PATH_VIEW_SECTION,
                                            MOD_CFG_MAX_PATH_VIEW_REFRESH,
                                            SAT_CFG_INT_MAX_PATH_VIEW_REFRESH);
    // Max refresh rate is 500
    // If above 500, int overflow has ocurred
    // Catch and set to 1 by default
    if (max_path_view->refresh > 500)
    {
        max_path_view->refresh = 1;
    }
    max_path_view->counter = 1;

    max_path_view->search_controls = gen_search_controls(max_path_view);
    max_path_view->display_path = gen_path_display();

    gtk_box_pack_start(GTK_BOX(widget), max_path_view->search_controls, FALSE, FALSE, 0); 

    gtk_box_pack_start(GTK_BOX(widget), max_path_view->display_path, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(widget), gen_OGS_display(max_path_view->qths), FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(widget), gen_SAT_display(max_path_view->sats), FALSE, TRUE, 0);

    gtk_widget_show_all(widget);

    return widget;
}