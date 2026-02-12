#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

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
        g_array_free(msat->panels, TRUE);
        g_array_free(msat->labels, TRUE);

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

    if (buff != NULL)
    {

        GtkWidget *labelIJ = g_array_index(msat->labels, GtkWidget *, 
                                (index * MAX_PATH_VIEW_FIELD_NUMBER) + i);
 
        //gtk_label_set_text(GTK_LABEL(msat->labels[index][i]), buff);
        gtk_label_set_text(GTK_LABEL(labelIJ), buff);
        /*
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    ("%s: current buff is %s, writing to %d case, first sat"),
                    buff, i);
        */        
        g_free(buff);
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

static void select_satellite(GtkWidget * menuitem, MaxSelectSatCallbackData * cb_data)
{
    GtkMaxPathView      *msat = GTK_MAX_PATH_VIEW(cb_data->parent);
    guint               i =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "index"));
    gchar               *title;
    sat_t               *sat;
    guint               index = cb_data->index;

    sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: select_satellite called and started, index = %d", __FILE__, __LINE__, index);
    // There are many ghost triggering of this signal, but we only need to
    // make a new selection when the received menuitem is selected
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
    {
        // i is the position in the grid
        // index is the index of the selected satellite in the satellites list
        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: if condition passed for select_satellite", __FILE__, __LINE__);

        //msat->selected[index] = i;
        guint *selected_index  = &g_array_index(msat->selected, guint, index);
        *selected_index = i;

        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: checking... msat->selected[index] is %u", __FILE__, __LINE__, msat->selected[index]);

        sat = SAT(g_slist_nth_data(msat->sats, i));

        title = g_markup_printf_escaped("<b>Satellite: %s</b>", sat ? sat->nickname : "No Satellite Selected");
        // This is probably the issue
        // Header is not updating (fixed - make sure i-th element in grid and index in sats is correct)
        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: checking... title = %s", __FILE__, __LINE__, title);
        //gtk_label_set_markup(GTK_LABEL(msat->panels[index]->header), title);
        MaxSatPanel *panelI = g_array_index(msat->panels, MaxSatPanel *, i);
        gtk_label_set_markup(GTK_LABEL(panelI->header), title);
        
        // bugged line: gtk_widget_queue_draw(msat->panels[i]->header);
        //gtk_widget_queue_draw(msat->panels[index]->header);
        gtk_widget_queue_draw(panelI->header);

        if (panelI->header == NULL) {
            sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: header is NULL!", __FILE__, __LINE__);
        } else {
            sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: header is NOT NULL!", __FILE__, __LINE__);
            // OLD, BUGGY LINE: sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: checking... msat->panels[i]->header = %s", __FILE__, __LINE__, msat->panels[i]->header);
            sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: checking... msat->panels[index]->header content updated", __FILE__, __LINE__);
        }
        
        g_free(title);
    }
}

/* Multiple sat options menu */
static void gtk_max_path_view_popup_cb(GtkWidget * button, MaxPopupCallbackData *cb_data)
{
    GtkMaxPathView      *max_path_view = GTK_MAX_PATH_VIEW(cb_data->parent);
    GtkWidget           *menu;
    GtkWidget           *menuitem;
    GtkWidget           *label;
    GSList              *group = NULL;
    gchar               *buff;
    sat_t               *sat = NULL;
    sat_t               *sati;
    guint                i, n;

    // Index here refers to position of the SatPanel in Multiple Sat View
    guint index = cb_data->index;
    // selectedIndex refers to the index of the satellite selected in the SatPanel
    gint selectedIndex = cb_data->selectedIndex;
    sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: callback started, index = %u", __FILE__, __LINE__, index);

    if (selectedIndex >= 0)
    {
        //sat = SAT(g_slist_nth_data(multiple_sat->sats, multiple_sat->selected[selectedIndex]));
        guint selectedNum = g_array_index(max_path_view->selected, guint, selectedIndex);
        sat = SAT(g_slist_nth_data(max_path_view->sats, selectedNum));
    }
    if (sat == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: invalid satellite", __FILE__, __LINE__);
        //return;
    }

    n = g_slist_length(max_path_view->sats);

    menu = gtk_menu_new();

    // Satellite name and info
    menuitem = gtk_menu_item_new();
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    buff = g_markup_printf_escaped("<b>%s</b>", sat ? sat->nickname : "No Satellite Selected");
    gtk_label_set_markup(GTK_LABEL(label), buff);
    g_free(buff);
    gtk_container_add(GTK_CONTAINER(menuitem), label);

    if (sat != NULL)
    {
        // Attach data to menuitem and connect callback
        g_object_set_data(G_OBJECT(menuitem), "sat", sat);
        g_object_set_data(G_OBJECT(menuitem), "qth", max_path_view->qth);
        g_signal_connect(menuitem, "activate",
                         G_CALLBACK(show_sat_info_menu_cb),
                         gtk_widget_get_toplevel(button));
    }
    else
    {
        gtk_widget_set_sensitive(menuitem, FALSE);
    }

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Separator
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    if (sat != NULL)
    {
        // Add the menu items for current, next and future passes
        add_pass_menu_items(menu, sat, max_path_view->qth, &max_path_view->tstamp, cb_data->parent);

        // Separator
        menuitem = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
    
    // Select sat
    for (i = 0; i < n; i++)
    {
        sati = SAT(g_slist_nth_data(max_path_view->sats, i));

        menuitem = gtk_radio_menu_item_new_with_label(group, sati->nickname);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));

        //if (i == multiple_sat->selected[selectedIndex])
        if (i == g_array_index(max_path_view->selected, guint, index))
        {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
        }

        // Store item index so that it is available in the callback
        g_object_set_data(G_OBJECT(menuitem), "index", GUINT_TO_POINTER(i));
        MaxSelectSatCallbackData *select_cb_data = g_new(MaxSelectSatCallbackData, 1);
        select_cb_data->parent = cb_data->parent;
        select_cb_data->index = index;
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: select_satellite callback signal triggered", __FILE__, __LINE__);
        g_signal_connect_after(menuitem, "activate",
                               G_CALLBACK(select_satellite), select_cb_data);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }

    gtk_widget_show_all(menu);

    /* gtk_menu_popup got deprecated in 3.22, first available in Ubuntu 18.04 */
#if GTK_MINOR_VERSION < 22
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   0, gdk_event_get_time((GdkEvent *) NULL));
#else
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
#endif
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

// Select new satellite
void gtk_max_path_view_select_sat(GtkWidget * max_path_view, gint catnum, guint index)
{
    GtkMaxPathView      *msat = GTK_MAX_PATH_VIEW(max_path_view);
    sat_t               *sat = NULL;
    gchar               *title;
    gboolean            foundsat = FALSE;
    gint                i, n;

    // Find satellite with catnum
    n = g_slist_length(msat->sats);
    for (i = 0; i < n; i++)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: iteration %d", __FILE__, __LINE__, i);
        sat = SAT(g_slist_nth_data(msat->sats, i));
        if (sat->tle.catnr == catnum)
        {
            // Found satellite
            //msat->selected[index] = i;
            guint *selected_index  = &g_array_index(msat->selected, guint, index);
            *selected_index = i;
            foundsat = TRUE;

            // Exit loop
            i = n;
        }
        if (!foundsat)
        {
            sat_log_log(SAT_LOG_LEVEL_ERROR,
                        _("%s: Could not find satellite with catalog number %d"),
                        __func__, catnum);
            //return;
            //msat->selected[index] = i;
            guint *selected_index  = &g_array_index(msat->selected, guint, index);
            *selected_index = i;
        }
    }

    title = g_markup_printf_escaped("<b>Satellite: %s</b>", sat ? sat->nickname : "No Satellite Selected");
    //gtk_label_set_markup(GTK_LABEL(msat->panels[index]->header), title);
    //gtk_widget_queue_draw(msat->panels[i]->header);
    MaxSatPanel *panelPointer = g_array_index(msat->panels, MaxSatPanel *, i);
    gtk_label_set_markup(GTK_LABEL(panelPointer->header), title);
    gtk_widget_queue_draw(panelPointer->header);
    g_free(title);
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

// index is the position in the grid
// selectedSatIndex is the index of the satellite in sats
static MaxSatPanel *create_sat_panel(gint index, guint32 flags,
                                  GtkWidget * parent, GSList * sats, gint selectedSatIndex)
{
    MaxSatPanel * panel = g_new0(MaxSatPanel, 1);
    GtkWidget *label1, *label2;
    sat_t *sat = NULL;
    
    if (index >= 0)
        sat = SAT(g_slist_nth_data(sats, selectedSatIndex));

    // Create popup button
    panel->popup_button = gpredict_mini_mod_button("gpredict-mod-popup.png",
                                                   _("Satellite options / shortcuts"));
    
    
    MaxPopupCallbackData *cb_data = g_new(MaxPopupCallbackData, 1);
    cb_data->parent = parent;
    // Index here represents the position of the SatPanel in the Multiple Sat View
    cb_data->index = index;
    cb_data->selectedIndex = selectedSatIndex;
    g_signal_connect(panel->popup_button, "clicked",
                     G_CALLBACK(gtk_max_path_view_popup_cb), cb_data);
    
    // Create header
    gchar *title = g_markup_printf_escaped("<b>Satellite: %s</b>", sat ? sat->nickname : "No Satellite Selected");
    panel->header = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(panel->header), title);
    g_free(title);
    g_object_set(panel->header, "xalign", 0.0f, "yalign", 0.5f, NULL);

    // Create table
    panel->table = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(panel->table), 5);
    gtk_grid_set_row_spacing(GTK_GRID(panel->table), 0);
    gtk_grid_set_column_spacing(GTK_GRID(panel->table), 5);

    for (guint i = 0; i < MAX_PATH_VIEW_FIELD_NUMBER; i++)
    {
        if (flags & (1 << i))
        {
            label1 = gtk_label_new(MAX_PATH_VIEW_FIELD_TITLE[i]);
            g_object_set(label1, "xalign", 1.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(panel->table), label1, 0, i, 1, 1);

            label2 = gtk_label_new("-");
            g_object_set(label2, "xalign", 0.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(panel->table), label2, 2, i, 1, 1);
            panel->labels[i] = label2;
            
            // Add tooltips
            gtk_widget_set_tooltip_text(label1, "NONESENSE");
            gtk_widget_set_tooltip_text(label2, "NONESENSE 2");

            label1 = gtk_label_new(":");
            gtk_grid_attach(GTK_GRID(panel->table), label1, 1, i, 1, 1);
        }
        else
        {
            panel->labels[i] = NULL;
        }
    }

    return panel;
}

void calculate_max_capacity_path(GtkWidget *button, gpointer data) {
    (void)button;
    GtkMaxPathView *obj = (GtkMaxPathView *)data;
    
    //to get actual current time use time-tools.c/get_current_daynum();
    //gdouble is in days, multiply with xmnpda to convert minutes to days
    
    gdouble t_start = 2458849.5;
    gdouble t_end = t_start + 1;
    gdouble time_step = 0.0007;    //1 minute time step
    
    clock_t timer_start, timer_end;
    double cpu_time_used;
    timer_start = clock();

    GList *sat_list = g_hash_table_get_values(obj->satsTable);
    guint sat_hist_len = 0;

    //GHashtable {gint catnr : lw_sat_t[] history}
    GHashTable *sat_history = generate_sat_pos_data(sat_list, &sat_hist_len, t_start, t_end, time_step);

    //GHashtable {gint assigned_id : qth_t station}
    GHashTable *ground_stations = g_hash_table_new(g_int_hash, g_int_equal);
    gint key1 = -1;
    g_hash_table_insert(ground_stations, &key1, obj->qth);
 
    //get_all_link_capacities(satmap->sats, 1, t_start, t_end, time_step);
    //returns GList of path_node
    obj->max_capacity_path = get_max_link_path(obj->satsTable, sat_history, sat_hist_len, ground_stations, t_start, t_end, time_step);    

    g_hash_table_destroy(sat_history);
    g_hash_table_destroy(ground_stations);

    timer_end = clock();
    cpu_time_used = ((double)(timer_end - timer_start)) / CLOCKS_PER_SEC;
    printf("Function took %f seconds to execute (CPU time).\n", cpu_time_used);

    g_signal_emit_by_name(obj, "update_path");
}


GtkWidget *gtk_max_path_view_new(GKeyFile * cfgdata, GHashTable * sats,
                                qth_t * qth, guint32 fields)
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

    max_path_view->dyn_num_sat = g_slist_length(max_path_view->sats);

    max_path_view->selected = g_array_sized_new(FALSE, TRUE, sizeof(guint), max_path_view->dyn_num_sat);
 
    for (i = 0; i < max_path_view->dyn_num_sat; i++)
    {
        g_array_append_val(max_path_view->selected, i);
    }
    
    max_path_view->qth = qth;
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

    //should be empty text box 
    max_path_view->textbox = gtk_label_new("THIS IS THE MAX PATH MODE");

    max_path_view->satsTable = sats;
    max_path_view->button = gtk_button_new_with_label("Calculate Path");

    g_signal_new("update-path", G_TYPE_FROM_INSTANCE(max_path_view),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL, G_TYPE_NONE, 0);

    g_signal_connect(max_path_view->button, "clicked", G_CALLBACK(calculate_max_capacity_path), max_path_view);

    // Make a grid, then populate the grid using SatPanels
    // Create a scrollable area
    max_path_view->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(max_path_view->swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 20);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    max_path_view->panels = g_array_sized_new(FALSE, TRUE, sizeof(MaxSatPanel *), max_path_view->dyn_num_sat);
    max_path_view->labels = g_array_sized_new(FALSE, TRUE, sizeof(GtkWidget *), 
                            max_path_view->dyn_num_sat * MAX_PATH_VIEW_FIELD_NUMBER);

    for (i = 0; i < max_path_view->dyn_num_sat; i++)
    {
        guint index = g_array_index(max_path_view->selected, guint, i);

        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: index set to %u", __FILE__, __LINE__, index);
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint, iteration %d", __FILE__, __LINE__, i);
        // i is the position in the grid
        // index is the index of the satellite in sats

        //multiple_sat->panels[i] = create_sat_panel(i, multiple_sat->flags, widget, multiple_sat->sats, index);
        MaxSatPanel *tmpSatPanel = create_sat_panel(i, max_path_view->flags, widget, max_path_view->sats, index);
        g_array_append_val(max_path_view->panels, tmpSatPanel);

        for (guint j = 0; j < MAX_PATH_VIEW_FIELD_NUMBER; j++)
        { 
            //multiple_sat->labels[i][j] = multiple_sat->panels[i]->labels[j];
            g_array_append_val(max_path_view->labels, tmpSatPanel->labels[j]);
        }
        
        // Build vbox for header+table
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);   // Header box
        gtk_box_pack_start(GTK_BOX(hbox), tmpSatPanel->popup_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), tmpSatPanel->header, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), tmpSatPanel->table, FALSE, FALSE, 0);

        // Place it in the 2xN grid
        guint row = i / MAX_SATS_PER_ROW;
        guint col = i % MAX_SATS_PER_ROW;
        gtk_grid_attach(GTK_GRID(grid), vbox, col, row, 1, 1);
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
    }
    
    gtk_container_add(GTK_CONTAINER(max_path_view->swin), grid);
    gtk_box_pack_end(GTK_BOX(widget), max_path_view->swin, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(widget), max_path_view->textbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(widget), max_path_view->button, FALSE, FALSE, 0);
    gtk_widget_show_all(widget);

    return widget;
}