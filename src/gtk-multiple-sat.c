#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-data.h"
#include "gtk-sat-popup-common.h"
#include "gtk-multiple-sat.h"
#include "locator.h"
#include "mod-cfg-get-param.h"
#include "orbit-tools.h"
#include "predict-tools.h"
#include "sat-cfg.h"
#include "sat-info.h"
#include "sat-log.h"
#include "sat-vis.h"
#include "sgpsdp/sgp4sdp4.h"
#include "sat-pass-dialogs.h"
#include "time-tools.h"
#include "sat-kdtree-utils.h"
#include "calc-dist-two-sat.h"
#include "skr-utils.h"
#include "sat-graph.h"


/* Column titles indexed with column symb. refs */
const gchar     *MULTIPLE_SAT_FIELD_TITLE[MULTIPLE_SAT_FIELD_NUMBER] = {
    N_("Azimuth"),
    N_("Elevation"),
    N_("Direction"),
    N_("Right Asc."),
    N_("Declination"),
    N_("Slant Range"),
    N_("Range Rate"),
    N_("Next Event"),
    N_("Next AOS"),
    N_("Next LOS"),
    N_("SSP Lat."),
    N_("SSP Lon."),
    N_("SSP Loc."),
    N_("Footprint"),
    N_("Altitude"),
    N_("Velocity"),
    N_("Doppler@100M"),
    N_("Sig. Loss"),
    N_("Sig. Delay"),
    N_("Mean Anom."),
    N_("Orbit Phase"),
    N_("Orbit Num."),
    N_("Visibility"),
    N_("Downlink SKR"),
    N_("Uplink SKR"),
    N_("Inter-satellite SKR")
};

const gchar     *MULTIPLE_SAT_FIELD_HINT[MULTIPLE_SAT_FIELD_NUMBER] = {
    N_("Azimuth of the satellite"),
    N_("Elevation of the satellite"),
    N_("Direction of the satellite"),
    N_("Right Ascension of the satellite"),
    N_("Declination of the satellite"),
    N_("The range between satellite and observer"),
    N_("The rate at which the Slant Range changes"),
    N_("The time of next AOS or LOS"),
    N_("The time of next AOS"),
    N_("The time of next LOS"),
    N_("Latitude of the sub-satellite point"),
    N_("Longitude of the sub-satellite point"),
    N_("Sub-Satellite Point as Maidenhead grid square"),
    N_("Diameter of the satellite footprint"),
    N_("Altitude of the satellite"),
    N_("Tangential velocity of the satellite"),
    N_("Doppler Shift @ 100MHz"),
    N_("Signal loss @ 100MHz"),
    N_("Signal Delay"),
    N_("Mean Anomaly"),
    N_("Orbit Phase"),
    N_("Orbit Number"),
    N_("Visibility of the satellite"),
    N_("Secret Key Rate (satellite to ground, downlink)"),
    N_("Secret Key Rate (ground to satellite, uplink)"),
    N_("Secret Key Rate (satellite to nearest satellite)")
};

static GtkBoxClass *parent_class = NULL;


static void gtk_multiple_sat_destroy(GtkWidget * widget)
{
    GtkMultipleSat      *msat = GTK_MULTIPLE_SAT(widget);
    guint i;
    gint satlist[NUMBER_OF_SATS];
    gboolean check = FALSE;
    
    //Free memory (gpredict call this function twice for some reason
    // so we use sanity check before free)
    if (msat->dyn_num_sat != NULL) { 

        for (i = 0; i < NUMBER_OF_SATS; i++)
        {
            sat_t *sat = SAT(g_slist_nth_data(msat->sats, g_array_index(msat->selected, guint, i)));
            if (sat != NULL)
            {
                satlist[i] = sat->tle.catnr;
                check = TRUE;
            }
            else
            {
                satlist[i] = 0;
            }
        }
        
        g_key_file_set_integer_list(msat->cfgdata, MOD_CFG_MULTIPLE_SAT_SECTION,
                                    MOD_CFG_MULTIPLE_SAT_SELECT, satlist,
                                    NUMBER_OF_SATS);

        g_array_free(msat->selected, TRUE);
        g_array_free(msat->panels, TRUE);

        msat->dyn_num_sat = NULL;
    }

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_multiple_sat_class_init(GtkMultipleSatClass * class,
                                        gpointer class_data)
{
    GtkWidgetClass *widget_class;

    (void)class_data;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_multiple_sat_destroy;
    parent_class = g_type_class_peek_parent(class);
}

static void gtk_multiple_sat_init(GtkMultipleSat * list, gpointer g_class)
{
    (void)list;
    (void)g_class;
}

/* Update a field in the GtkMultipleSat view */
static void update_field(GtkMultipleSat * msat, guint i, guint index)
{
    sat_t       *sat;
    gchar       *buff = NULL;
    gchar       tbuf[TIME_FORMAT_MAX_LENGTH];
    gchar       hmf = ' ';
    gdouble     number;
    gint        retcode;
    gchar       *fmtstr;
    gchar       *alstr;
    sat_vis_t   vis;
    sat_t       *nsat;  // Nearest satellite, for calculating inter-sat SKR
    //gdouble     dist;   // Distance to nsat
    gboolean    los_vis;   // Is LOS clear from sat to nsat
    gdouble     up_skr, down_skr, inter_sat_skr;

    // sanity checks
    if (msat->labels[index][i] == NULL)
    {
        /*
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Can not update invisible field (I:%d F:%d)"),
                    __FILE__, __LINE__, i, msat->flags);
        */
        //return;
    }

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
        case MULTIPLE_SAT_FIELD_AZ:
            buff = g_strdup_printf("%6.2f\302\260", sat->az);
            break;
        case MULTIPLE_SAT_FIELD_EL:
            buff = g_strdup_printf("%6.2f\302\260", sat->el);
            break;
        case MULTIPLE_SAT_FIELD_DIR:
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
        case MULTIPLE_SAT_FIELD_RA:
            buff = g_strdup_printf("%6.2f\302\260", sat->ra);
            break;
        case MULTIPLE_SAT_FIELD_DEC:
            buff = g_strdup_printf("%6.2f\302\260", sat->dec);
            break;
        case MULTIPLE_SAT_FIELD_RANGE:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
                buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->range));
            else
                buff = g_strdup_printf("%.0f km", sat->range);
            break;
        case MULTIPLE_SAT_FIELD_RANGE_RATE:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
                buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->range_rate));
            else
                buff = g_strdup_printf("%.3f km/sec", sat->range_rate);
            break;
        case MULTIPLE_SAT_FIELD_NEXT_EVENT:
            if (sat->aos > sat->los)
            {
                /* next event is LOS */
                number = sat->los;
                alstr = g_strdup("LOS: ");
            }
            else
            {
                /* next event is AOS */
                number = sat->aos;
                alstr = g_strdup("AOS: ");
            }
            if (number > 0.0)
            {

                /* format the number */
                fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
                daynum_to_str(tbuf, TIME_FORMAT_MAX_LENGTH, fmtstr, number);

                g_free(fmtstr);

                buff = g_strconcat(alstr, tbuf, NULL);

            }
            else
            {
                buff = g_strdup(_("N/A"));
            }
            g_free(alstr);
            break;
        case MULTIPLE_SAT_FIELD_AOS:
            if (sat->aos > 0.0)
            {
                /* format the number */
                fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
                daynum_to_str(tbuf, TIME_FORMAT_MAX_LENGTH, fmtstr, sat->aos);
                g_free(fmtstr);
                buff = g_strdup(tbuf);
            }
            else
            {
                buff = g_strdup(_("N/A"));
            }
            break;
        case MULTIPLE_SAT_FIELD_LOS:
            if (sat->los > 0.0)
            {
                fmtstr = sat_cfg_get_str(SAT_CFG_STR_TIME_FORMAT);
                daynum_to_str(tbuf, TIME_FORMAT_MAX_LENGTH, fmtstr, sat->los);
                g_free(fmtstr);
                buff = g_strdup(tbuf);
            }
            else
            {
                buff = g_strdup(_("N/A"));
            }
            break;
        case MULTIPLE_SAT_FIELD_LAT:
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
        case MULTIPLE_SAT_FIELD_LON:
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
        case MULTIPLE_SAT_FIELD_SSP:
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
        case MULTIPLE_SAT_FIELD_FOOTPRINT:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            {
                buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->footprint));
            }
            else
            {
                buff = g_strdup_printf("%.0f km", sat->footprint);
            }
            break;
        case MULTIPLE_SAT_FIELD_ALT:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
                buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->alt));
            else
                buff = g_strdup_printf("%.0f km", sat->alt);
            break;
        case MULTIPLE_SAT_FIELD_VEL:
            if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
                buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->velo));
            else
                buff = g_strdup_printf("%.3f km/sec", sat->velo);
            break;
        case MULTIPLE_SAT_FIELD_DOPPLER:
            number = -100.0e06 * (sat->range_rate / 299792.4580);   // Hz
            buff = g_strdup_printf("%.0f Hz", number);
            break;
        case MULTIPLE_SAT_FIELD_LOSS:
            number = 72.4 + 20.0 * log10(sat->range);       // dB
            buff = g_strdup_printf("%.2f dB", number);
            break;
        case MULTIPLE_SAT_FIELD_DELAY:
            number = sat->range / 299.7924580;      // msec 
            buff = g_strdup_printf("%.2f msec", number);
            break;
        case MULTIPLE_SAT_FIELD_MA:
            buff = g_strdup_printf("%.2f\302\260", sat->ma);
            break;
        case MULTIPLE_SAT_FIELD_PHASE:
            buff = g_strdup_printf("%.2f\302\260", sat->phase);
            break;
        case MULTIPLE_SAT_FIELD_ORBIT:
            buff = g_strdup_printf("%ld", sat->orbit);
            break;
        case MULTIPLE_SAT_FIELD_VISIBILITY:
            vis = get_sat_vis(sat, msat->qth, sat->jul_utc);
            buff = vis_to_str(vis);
            break;
        case MULTIPLE_SAT_FIELD_SKR_DOWN:
            if (sat->el < 30)
            {
                buff = g_strdup_printf("Below elevation threshold");
            }
            else
            {
                // Sat is in viable elevation, calculate downlink SKR
                down_skr = sat_to_ground_downlink(sat, msat->qth);
                buff = g_strdup_printf("%.2e bps", down_skr);
            }
            break;

        case MULTIPLE_SAT_FIELD_SKR_UP:
            if (sat->el < 30)
            {
                buff = g_strdup_printf("Below elevation threshold");
            }
            else
            {
                // Sat is in viable elevation, calculate uplink SKR
                up_skr = ground_to_sat_uplink(msat->qth, sat);
                buff = g_strdup_printf("%.2e bps", up_skr);
            }
            break;

        case MULTIPLE_SAT_FIELD_SKR_NEAREST:
            nsat = sat_kdtree_find_nearest_other(msat->kdtree, sat);
            
            // Add a NULL check for nsat to avoid SEGFAULT
            if (nsat == NULL) 
            {
                buff = g_strdup("N/A");
                break;
            }
            
            los_vis = is_los_clear(sat, nsat);
            if (los_vis)
            {
                // Line of sight is clear, calculate distance and inter-satellite SKR
                inter_sat_skr = inter_sat_link(sat, nsat);
                buff = g_strdup_printf("%s, %.2e bps", nsat->nickname, inter_sat_skr);
            }
            else    // line of sight not clear
            {
                buff = g_strdup_printf("%s, No LOS", nsat->nickname);
            }
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
        gtk_label_set_text(GTK_LABEL(msat->labels[index][i]), buff);
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
    GtkMultipleSat      *multiple_sat = GTK_MULTIPLE_SAT(user_data);
    sat_t               *sat = SAT(value);

    (void)key;

    multiple_sat->sats = g_slist_insert_sorted(multiple_sat->sats, sat,
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

static void select_satellite(GtkWidget * menuitem, SelectSatCallbackData * cb_data)
{
    GtkMultipleSat      *msat = GTK_MULTIPLE_SAT(cb_data->parent);
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
        SatPanel *panelI = g_array_index(msat->panels, SatPanel *, i);
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
static void gtk_multiple_sat_popup_cb(GtkWidget * button, PopupCallbackData *cb_data)
{
    GtkMultipleSat      *multiple_sat = GTK_MULTIPLE_SAT(cb_data->parent);
    GtkWidget           *menu;
    GtkWidget           *menuitem;
    GtkWidget           *label;
    GSList              *group = NULL;
    gchar               *buff;
    sat_t               *sat = NULL;
    sat_t               *sati;
    gint                i, n;

    // Index here refers to position of the SatPanel in Multiple Sat View
    guint index = cb_data->index;
    // selectedIndex refers to the index of the satellite selected in the SatPanel
    gint selectedIndex = cb_data->selectedIndex;
    sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: callback started, index = %u", __FILE__, __LINE__, index);

    if (selectedIndex >= 0)
    {
        //sat = SAT(g_slist_nth_data(multiple_sat->sats, multiple_sat->selected[selectedIndex]));
        guint selectedNum = g_array_index(multiple_sat->selected, guint, selectedIndex);
        sat = SAT(g_slist_nth_data(multiple_sat->sats, selectedNum));
    }
    if (sat == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: invalid satellite", __FILE__, __LINE__);
        //return;
    }

    n = g_slist_length(multiple_sat->sats);

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
        g_object_set_data(G_OBJECT(menuitem), "qth", multiple_sat->qth);
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
        add_pass_menu_items(menu, sat, multiple_sat->qth, &multiple_sat->tstamp, cb_data->parent);

        // Separator
        menuitem = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
    }
    
    // Select sat
    for (i = 0; i < n; i++)
    {
        sati = SAT(g_slist_nth_data(multiple_sat->sats, i));

        menuitem = gtk_radio_menu_item_new_with_label(group, sati->nickname);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));

        //if (i == multiple_sat->selected[selectedIndex])
        if (i == g_array_index(multiple_sat->selected, guint, index))
        {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
        }

        // Store item index so that it is available in the callback
        g_object_set_data(G_OBJECT(menuitem), "index", GUINT_TO_POINTER(i));
        SelectSatCallbackData *select_cb_data = g_new(SelectSatCallbackData, 1);
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
void gtk_multiple_sat_reload_sats(GtkWidget * multiple_sat, GHashTable * sats)
{
    // Free GSLists
    g_slist_free(GTK_MULTIPLE_SAT(multiple_sat)->sats);
    GTK_MULTIPLE_SAT(multiple_sat)->sats = NULL;

    // Reload satellites
    g_hash_table_foreach(sats, store_sats, multiple_sat);
}

/* Reload configuration */
void gtk_multiple_sat_reconf(GtkWidget * widget,
                             GKeyFile * newcfg,
                             GHashTable * sats, qth_t * qth, gboolean local)
{
    guint32         fields;

    // Store pointer to new cfg data
    GTK_MULTIPLE_SAT(widget)->cfgdata = newcfg;

    // Get visible fields from new configuration
    fields = mod_cfg_get_int(newcfg,
                             MOD_CFG_MULTIPLE_SAT_SECTION,
                             MOD_CFG_MULTIPLE_SAT_FIELDS,
                             SAT_CFG_INT_MULTIPLE_SAT_FIELDS);
    
    if (fields != GTK_MULTIPLE_SAT(widget)->flags)
    {
        GTK_MULTIPLE_SAT(widget)->flags = fields;
    }

    // If this is a local reconfiguration sats may have changed
    if (local)
    {
        gtk_multiple_sat_reload_sats(widget, sats);
    }

    // QTH may have changed too single we have a default QTH
    GTK_MULTIPLE_SAT(widget)->qth = qth;

    // Get refresh rate and cycle counter
    GTK_MULTIPLE_SAT(widget)->refresh = mod_cfg_get_int(newcfg,
                                                        MOD_CFG_MULTIPLE_SAT_SECTION,
                                                        MOD_CFG_MULTIPLE_SAT_REFRESH,
                                                        SAT_CFG_INT_MULTIPLE_SAT_REFRESH);
    GTK_MULTIPLE_SAT(widget)->counter = 1;
}

// Select new satellite
void gtk_multiple_sat_select_sat(GtkWidget * multiple_sat, gint catnum, guint index)
{
    GtkMultipleSat      *msat = GTK_MULTIPLE_SAT(multiple_sat);
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
    SatPanel *panelPointer = g_array_index(msat->panels, SatPanel *, i);
    gtk_label_set_markup(GTK_LABEL(panelPointer->header), title);
    gtk_widget_queue_draw(panelPointer->header);
    g_free(title);
}

// Update satellites
void gtk_multiple_sat_update(GtkWidget * widget, guint index)
{
    GtkMultipleSat      *msat = GTK_MULTIPLE_SAT(widget);
    guint               i;

    // Sanity checks
    if ((msat == NULL) || !IS_GTK_MULTIPLE_SAT(msat))
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
        if ((msat->flags & MULTIPLE_SAT_FLAG_RA) ||
            (msat->flags & MULTIPLE_SAT_FLAG_DEC))
        {
            obs_astro_t     astro;
            sat_t           *sat =
                //SAT(g_slist_nth_data(msat->sats, msat->selected[index]));
                SAT(g_slist_nth_data(msat->sats, g_array_index(msat->selected, guint, index)));

            //TEMP FIX: when g_array is implemented for gtk-multiple-sat then
            //should remove if statement
            if (index < msat->dyn_num_sat) {
                Calculate_RADec(sat, msat->qth, &astro);
                sat->ra = Degrees(astro.ra);
                sat->dec = Degrees(astro.dec);
 
            }
        }

        // Update visible fields one by one
        for (i = 0; i < MULTIPLE_SAT_FIELD_NUMBER; i++)
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

GType gtk_multiple_sat_get_type()
{
    static GType    gtk_multiple_sat_type = 0;

    if (!gtk_multiple_sat_type)
    {
        static const GTypeInfo gtk_multiple_sat_info = {
            sizeof(GtkMultipleSatClass),
            NULL,                   // base_init
            NULL,                   // base_finalize
            (GClassInitFunc) gtk_multiple_sat_class_init,
            NULL,                   // class_finalize
            NULL,                   // class_data
            sizeof(GtkMultipleSat),
            5,                      // n_preallocs
            (GInstanceInitFunc) gtk_multiple_sat_init,
            NULL
        };
        
        gtk_multiple_sat_type = g_type_register_static(GTK_TYPE_BOX,
                                                       "GtkMultipleSat",
                                                       &gtk_multiple_sat_info, 0);
    }

    return gtk_multiple_sat_type;
}

// index is the position in the grid
// selectedSatIndex is the index of the satellite in sats
static SatPanel *create_sat_panel(gint index, guint32 flags,
                                  GtkWidget * parent, GSList * sats, gint selectedSatIndex)
{
    sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: %s called, position=%d, satIndex=%d",
                __FILE__, __LINE__, __func__, index, selectedSatIndex);
    SatPanel * panel = g_new0(SatPanel, 1);
    GtkWidget *label1, *label2;
    sat_t *sat = NULL;
    //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
    if (index >= 0)
        sat = SAT(g_slist_nth_data(sats, selectedSatIndex));

    //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
    // Create popup button
    panel->popup_button = gpredict_mini_mod_button("gpredict-mod-popup.png",
                                                   _("Satellite options / shortcuts"));
    //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
    PopupCallbackData *cb_data = g_new(PopupCallbackData, 1);
    cb_data->parent = parent;
    // Index here represents the position of the SatPanel in the Multiple Sat View
    cb_data->index = index;
    cb_data->selectedIndex = selectedSatIndex;
    g_signal_connect(panel->popup_button, "clicked",
                     G_CALLBACK(gtk_multiple_sat_popup_cb), cb_data);
    
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

    for (guint i = 0; i < MULTIPLE_SAT_FIELD_NUMBER; i++)
    {
        if (flags & (1 << i))
        {
            label1 = gtk_label_new(MULTIPLE_SAT_FIELD_TITLE[i]);
            g_object_set(label1, "xalign", 1.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(panel->table), label1, 0, i, 1, 1);

            label2 = gtk_label_new("-");
            g_object_set(label2, "xalign", 0.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(panel->table), label2, 2, i, 1, 1);
            panel->labels[i] = label2;
            
            // Add tooltips
            gtk_widget_set_tooltip_text(label1, MULTIPLE_SAT_FIELD_HINT[i]);
            gtk_widget_set_tooltip_text(label2, MULTIPLE_SAT_FIELD_HINT[i]);

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

GtkWidget *gtk_multiple_sat_new(GKeyFile * cfgdata, GHashTable * sats,
                                qth_t * qth, guint32 fields)
{
    GtkWidget       *widget;
    GtkMultipleSat  *multiple_sat;
    guint           i;
    //gint            selectedcatnum[NUMBER_OF_SATS];
    sat_t           *sati;

    widget = g_object_new(GTK_TYPE_MULTIPLE_SAT, NULL);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(widget),
                                   GTK_ORIENTATION_VERTICAL);
    multiple_sat = GTK_MULTIPLE_SAT(widget);

    multiple_sat->update = gtk_multiple_sat_update;

    // Read configuration data
    /* ... */

    g_hash_table_foreach(sats, store_sats, widget);

    multiple_sat->dyn_num_sat = g_slist_length(multiple_sat->sats);
    sat_log_log(SAT_LOG_LEVEL_DEBUG, "dyn_num_sat set to %d", multiple_sat->dyn_num_sat);

    multiple_sat->selected = g_array_sized_new(FALSE, TRUE, sizeof(guint), multiple_sat->dyn_num_sat);
 
    for (i = 0; i < multiple_sat->dyn_num_sat; i++)
    {
        g_array_append_val(multiple_sat->selected, i);
    }
    for (i = 0; i < multiple_sat->dyn_num_sat; i++)
    { 
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    "%s %s: selected[%d] set to: %d", __func__, __FILE__, 
                    i, g_array_index(multiple_sat->selected, guint, i));
    }
    multiple_sat->qth = qth;
    multiple_sat->cfgdata = cfgdata;

    // Populate k-d tree for calcuating nearest sat
    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, sats);
    multiple_sat->kdtree = sat_kdtree_create();
    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        sati = SAT(value);
        sat_kdtree_insert(multiple_sat->kdtree, sati);
    }

    // Initialise column flags
    if (fields > 0)
    {
        multiple_sat->flags = fields;
    }
    else
    {
        multiple_sat->flags = mod_cfg_get_int(cfgdata,
                                              MOD_CFG_MULTIPLE_SAT_SECTION,
                                              MOD_CFG_MULTIPLE_SAT_FIELDS,
                                              SAT_CFG_INT_MULTIPLE_SAT_FIELDS);
    }

    // Get refresh rate and cycle counter
    multiple_sat->refresh = mod_cfg_get_int(cfgdata,
                                            MOD_CFG_MULTIPLE_SAT_SECTION,
                                            MOD_CFG_MULTIPLE_SAT_REFRESH,
                                            SAT_CFG_INT_MULTIPLE_SAT_REFRESH);
    // Max refresh rate is 500
    // If above 500, int overflow has ocurred
    // Catch and set to 1 by default
    if (multiple_sat->refresh > 500)
    {
        multiple_sat->refresh = 1;
    }
    multiple_sat->counter = 1;

    /*
    // Get selected catnum if available
    for (i = 0; i < NUMBER_OF_SATS; i++)
    {
        multiple_sat->selected[i] = mod_cfg_get_int_from_list(cfgdata,
                                                      MOD_CFG_MULTIPLE_SAT_SECTION,
                                                      MOD_CFG_MULTIPLE_SAT_SELECT,
                                                      i, SAT_CFG_INT_MULTIPLE_SAT_SELECT);
    }
    */

    // Make a grid, then populate the grid using SatPanels
    // Create a scrollable area
    multiple_sat->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(multiple_sat->swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 20);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 10);

    multiple_sat->panels = g_array_sized_new(FALSE, TRUE, sizeof(SatPanel *), multiple_sat->dyn_num_sat);

    for (i = 0; i < multiple_sat->dyn_num_sat; i++)
    {
        guint index = g_array_index(multiple_sat->selected, guint, i);

        sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: index set to %u", __FILE__, __LINE__, index);
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint, iteration %d", __FILE__, __LINE__, i);
        // i is the position in the grid
        // index is the index of the satellite in sats

        //multiple_sat->panels[i] = create_sat_panel(i, multiple_sat->flags, widget, multiple_sat->sats, index);

        //Array of SatPanel pointers. We get the pointer to the element in the array, then derefrence it to 
        //assign the new satpanel pointer value to it.
        SatPanel **satPanelI  = &g_array_index(multiple_sat->panels, SatPanel *, i);
        *satPanelI = create_sat_panel(i, multiple_sat->flags, widget, multiple_sat->sats, index);

        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);

        for (guint j = 0; j < MULTIPLE_SAT_FIELD_NUMBER; j++)
        { 
            //multiple_sat->labels[i][j] = multiple_sat->panels[i]->labels[j];
            SatPanel *satPanel = g_array_index(multiple_sat->panels, SatPanel *, i);
            multiple_sat->labels[i][j] = satPanel->labels[j];
        }
        // Some debugging stuff
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: current iteration - %d", __FILE__, __LINE__, i);

        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        // Load selected catnum from config
        //multiple_sat->panels[i]->selected_catnum = selectedcatnum[i];
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Panel catnum - %d", __FILE__, __LINE__, multiple_sat->panels[i]->selected_catnum);

        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        // Build vbox for header+table
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);   // Header box
        gtk_box_pack_start(GTK_BOX(hbox), ((SatPanel *)*satPanelI)->popup_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), ((SatPanel*)*satPanelI)->header, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), ((SatPanel *)*satPanelI)->table, FALSE, FALSE, 0);

        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
        // Place it in the 2xN grid
        guint row = i / SATS_PER_ROW;
        guint col = i % SATS_PER_ROW;
        gtk_grid_attach(GTK_GRID(grid), vbox, col, row, 1, 1);
        //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);

        // Currently broken
        // Apply selection if available
        /*
        if (panel->selected_catnum)
        {
            sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: line called", __FILE__, __LINE__);
            gtk_multiple_sat_select_sat(widget, panel->selected_catnum, i);
        }
        */
    }
    
    //sat_log_log(SAT_LOG_LEVEL_DEBUG, "%s %d: Breakpoint", __FILE__, __LINE__);
    gtk_container_add(GTK_CONTAINER(multiple_sat->swin), grid);
    gtk_box_pack_end(GTK_BOX(widget), multiple_sat->swin, TRUE, TRUE, 0);
    gtk_widget_show_all(widget);

    return widget;
}