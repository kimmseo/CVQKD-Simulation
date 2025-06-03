#ifdef HAVE_CONFIG_H
#include <build-config.h>
#endif
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "config-keys.h"
#include "gpredict-utils.h"
#include "gtk-sat-data.h"
#include "gtk-sat-popup-common.h"
#include "gtk-two-sat.h"
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

/* Column titles indexed with column symb. refs. */
const gchar    *TWO_SAT_FIELD_TITLE[TWO_SAT_FIELD_NUMBER] = {
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
    N_("SKR")
};

/* Column title hints indexed with column symb. refs. */
const gchar    *TWO_SAT_FIELD_HINT[TWO_SAT_FIELD_NUMBER] = {
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
    N_("Secret Key Rate")
};

static GtkBoxClass *parent_class = NULL;

static void gtk_two_sat_destroy(GtkWidget * widget)
{
    GtkTwoSat   *tsat = GTK_TWO_SAT(widget);
    sat_t       *sat1 = SAT(g_slist_nth_data(tsat->sats, tsat->selected1));
    sat_t       *sat2 = SAT(g_slist_nth_data(tsat->sats, tsat->selected2));

    if (sat1 != NULL || sat2 != NULL)
    {
        g_key_file_set_integer(tsat->cfgdata, MOD_CFG_TWO_SAT_SECTION,
                               MOD_CFG_TWO_SAT_SELECT_FIRST, sat1->tle.catnr);
        g_key_file_set_integer(tsat->cfgdata, MOD_CFG_SINGLE_SAT_SECTION,
                               MOD_CFG_TWO_SAT_SELECT_SECOND, sat2->tle.catnr);
    }

    (*GTK_WIDGET_CLASS(parent_class)->destroy) (widget);
}

static void gtk_two_sat_class_init(GtkTwoSatClass * class, gpointer class_data)
{
    GtkWidgetClass *widget_class;

    (void)class_data;

    widget_class = (GtkWidgetClass *) class;
    widget_class->destroy = gtk_two_sat_destroy;
    parent_class = g_type_class_peek_parent(class);
}

static void gtk_two_sat_init(GtkTwoSat * list, gpointer g_class)
{
    (void)list;
    (void)g_class;
}


// Update a field in the GtkTwoSat View, first satellite
static void update_field_first(GtkTwoSat * tsat, guint i)
{
    sat_t      *sat;
    gchar      *buff = NULL;
    gchar       tbuf[TIME_FORMAT_MAX_LENGTH];
    gchar       hmf = ' ';
    gdouble     number;
    gint        retcode;
    gchar      *fmtstr;
    gchar      *alstr;
    sat_vis_t   vis;
    gdouble     skr;

    // Make some sanity checks
    if(tsat->labels1[i] == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Can not update invisible field (I:%d F:%d)"),
                    __FILE__, __LINE__, i, tsat->flags);
        return;
    }

    sat = SAT(g_slist_nth_data(tsat->sats, tsat->selected1));
    if (!sat)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, "%s: %d - Cannot update non-existing sat",
                    __FILE__, __LINE__);
        return;
    }

    // Update requested field
    switch (i)
    {
    case TWO_SAT_FIELD_AZ:
        buff = g_strdup_printf("%6.2f\302\260", sat->az);
        break;
    case TWO_SAT_FIELD_EL:
        buff = g_strdup_printf("%6.2f\302\260", sat->el);
        break;
    case TWO_SAT_FIELD_DIR:
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
            // Receding
            buff = g_strdup("Receding");
        }
        else if (sat->range_rate < 0.0)
        {
            // Approaching
            buff = g_strdup("Approaching");
        }
        else
        {
            buff = g_strdup("Approaching");
        }
        break;
    case TWO_SAT_FIELD_RA:
        buff = g_strdup_printf("%6.2f\302\260", sat->ra);
        break;
    case TWO_SAT_FIELD_DEC:
        buff = g_strdup_printf("%6.2f\302\260", sat->dec);
        break;
    case TWO_SAT_FIELD_RANGE:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->range));
        else
            buff = g_strdup_printf("%.3f km/sec", sat->range);
        break;
    case TWO_SAT_FIELD_RANGE_RATE:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->range_rate));
        }
        else
        {
            buff = g_strdup_printf("%.3f km/sec", sat->range_rate);
        }
        break;
    case TWO_SAT_FIELD_NEXT_EVENT:
        if(sat->aos > sat->los)
        {
            // Next event is LOS
            number = sat->los;
            alstr = g_strdup("LOS: ");
        }
        else
        {
            // Next event is AOS
            number = sat->aos;
            alstr = g_strdup("AOS: ");
        }
        if (number > 0.0)
        {
            // Format the number
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
    case TWO_SAT_FIELD_AOS:
        if (sat->aos > 0.0)
        {
            // Format the number
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
    case TWO_SAT_FIELD_LOS:
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
    case TWO_SAT_FIELD_LAT:
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
    case TWO_SAT_FIELD_LON:
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
    case TWO_SAT_FIELD_SSP:
        // SSP Locator
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
    case TWO_SAT_FIELD_FOOTPRINT:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->footprint));
        }
        else
        {
            buff = g_strdup_printf("%.0f km", sat->footprint);
        }
        break;
    case TWO_SAT_FIELD_ALT:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->alt));
        }
        else
        {
            buff = g_strdup_printf("%.0f km", sat->alt);
        }
        break;
    case TWO_SAT_FIELD_VEL:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->velo));
        }
        else
        {
            buff = g_strdup_printf("%.3f km/sec", sat->velo);
        }
        break;
    case TWO_SAT_FIELD_DOPPLER:
        number = -100.0e06 * (sat->range_rate / 299792.4580);   // Hz
        buff = g_strdup_printf("%.0f Hz", number);
        break;
    case TWO_SAT_FIELD_LOSS:
        number = 72.4 + 20.0 * log10(sat->range);       // dB
        buff = g_strdup_printf("%.2f dB", number);
        break;
    case TWO_SAT_FIELD_DELAY:
        number = sat->range / 299.7924580;      // msec 
        buff = g_strdup_printf("%.2f msec", number);
        break;
    case TWO_SAT_FIELD_MA:
        buff = g_strdup_printf("%.2f\302\260", sat->ma);
        break;
    case TWO_SAT_FIELD_PHASE:
        buff = g_strdup_printf("%.2f\302\260", sat->phase);
        break;
    case TWO_SAT_FIELD_ORBIT:
        buff = g_strdup_printf("%ld", sat->orbit);
        break;
    case TWO_SAT_FIELD_VISIBILITY:
        vis = get_sat_vis(sat, tsat->qth, sat->jul_utc);
        buff = vis_to_str(vis);
        break;
    case TWO_SAT_FIELD_SKR:
        // using this field for finding out what is xyz in sgp first
        // placeholder
        buff = g_strdup_printf("x %lf y %lf z %lf mag %lf", sat->pos.x, sat->pos.y, sat->pos.z, sat->pos.w);
        break;
    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Invalid field number (%d)"),
                    __FILE__, __LINE__, i);
        break;
    }

    if (buff != NULL)
    {
        gtk_label_set_text(GTK_LABEL(tsat->labels1[i]), buff);
        /*
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    ("%s %s: current buff is %s, writing to %d case"), __FILE__,
                    buff, i);
        */        
        g_free(buff);
    }
}

// Update a field in the GtkTwoSat View, second satellite
static void update_field_second(GtkTwoSat * tsat, guint i)
{
    sat_t      *sat;
    gchar      *buff = NULL;
    gchar       tbuf[TIME_FORMAT_MAX_LENGTH];
    gchar       hmf = ' ';
    gdouble     number;
    gint        retcode;
    gchar      *fmtstr;
    gchar      *alstr;
    sat_vis_t   vis;
    gdouble     skr;

    // Make some sanity checks
    if(tsat->labels2[i] == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Can not update invisible field (I:%d F:%d)"),
                    __FILE__, __LINE__, i, tsat->flags);
        return;
    }

    sat = SAT(g_slist_nth_data(tsat->sats, tsat->selected2));
    if (!sat)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, "%s: %d - Cannot update non-existing sat",
                    __FILE__, __LINE__);
        return;
    }

    // Update requested field
    switch (i)
    {
    case TWO_SAT_FIELD_AZ:
        buff = g_strdup_printf("%6.2f\302\260", sat->az);
        break;
    case TWO_SAT_FIELD_EL:
        buff = g_strdup_printf("%6.2f\302\260", sat->el);
        break;
    case TWO_SAT_FIELD_DIR:
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
            // Receding
            buff = g_strdup("Receding");
        }
        else if (sat->range_rate < 0.0)
        {
            // Approaching
            buff = g_strdup("Approaching");
        }
        else
        {
            buff = g_strdup("Approaching");
        }
        break;
    case TWO_SAT_FIELD_RA:
        buff = g_strdup_printf("%6.2f\302\260", sat->ra);
        break;
    case TWO_SAT_FIELD_DEC:
        buff = g_strdup_printf("%6.2f\302\260", sat->dec);
        break;
    case TWO_SAT_FIELD_RANGE:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->range));
        else
            buff = g_strdup_printf("%.3f km/sec", sat->range);
        break;
    case TWO_SAT_FIELD_RANGE_RATE:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->range_rate));
        }
        else
        {
            buff = g_strdup_printf("%.3f km/sec", sat->range_rate);
        }
        break;
    case TWO_SAT_FIELD_NEXT_EVENT:
        if(sat->aos > sat->los)
        {
            // Next event is LOS
            number = sat->los;
            alstr = g_strdup("LOS: ");
        }
        else
        {
            // Next event is AOS
            number = sat->aos;
            alstr = g_strdup("AOS: ");
        }
        if (number > 0.0)
        {
            // Format the number
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
    case TWO_SAT_FIELD_AOS:
        if (sat->aos > 0.0)
        {
            // Format the number
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
    case TWO_SAT_FIELD_LOS:
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
    case TWO_SAT_FIELD_LAT:
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
    case TWO_SAT_FIELD_LON:
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
    case TWO_SAT_FIELD_SSP:
        // SSP Locator
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
    case TWO_SAT_FIELD_FOOTPRINT:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->footprint));
        }
        else
        {
            buff = g_strdup_printf("%.0f km", sat->footprint);
        }
        break;
    case TWO_SAT_FIELD_ALT:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.0f mi", KM_TO_MI(sat->alt));
        }
        else
        {
            buff = g_strdup_printf("%.0f km", sat->alt);
        }
        break;
    case TWO_SAT_FIELD_VEL:
        if (sat_cfg_get_bool(SAT_CFG_BOOL_USE_IMPERIAL))
        {
            buff = g_strdup_printf("%.3f mi/sec", KM_TO_MI(sat->velo));
        }
        else
        {
            buff = g_strdup_printf("%.3f km/sec", sat->velo);
        }
        break;
    case TWO_SAT_FIELD_DOPPLER:
        number = -100.0e06 * (sat->range_rate / 299792.4580);   // Hz
        buff = g_strdup_printf("%.0f Hz", number);
        break;
    case TWO_SAT_FIELD_LOSS:
        number = 72.4 + 20.0 * log10(sat->range);       // dB
        buff = g_strdup_printf("%.2f dB", number);
        break;
    case TWO_SAT_FIELD_DELAY:
        number = sat->range / 299.7924580;      // msec 
        buff = g_strdup_printf("%.2f msec", number);
        break;
    case TWO_SAT_FIELD_MA:
        buff = g_strdup_printf("%.2f\302\260", sat->ma);
        break;
    case TWO_SAT_FIELD_PHASE:
        buff = g_strdup_printf("%.2f\302\260", sat->phase);
        break;
    case TWO_SAT_FIELD_ORBIT:
        buff = g_strdup_printf("%ld", sat->orbit);
        break;
    case TWO_SAT_FIELD_VISIBILITY:
        vis = get_sat_vis(sat, tsat->qth, sat->jul_utc);
        buff = vis_to_str(vis);
        break;
    case TWO_SAT_FIELD_SKR:
        // using this field for finding out what is xyz in sgp first
        // placeholder
        buff = g_strdup_printf("x %lf y %lf z %lf mag %lf", sat->pos.x, sat->pos.y, sat->pos.z, sat->pos.w);
        break;
    default:
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s:%d: Invalid field number (%d)"),
                    __FILE__, __LINE__, i);
        break;
    }

    if (buff != NULL)
    {
        gtk_label_set_text(GTK_LABEL(tsat->labels2[i]), buff);
        /*
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    ("%s %s: current buff is %s, writing to %d case"), __FILE__,
                    buff, i);
        */        
        g_free(buff);
    }
}


static gint sat_name_compare (sat_t * a, sat_t * b)
{
    return gpredict_strcmp(a->nickname, b->nickname);
}

// Copy satellite from hash table to singly linked list
static void store_sats(gpointer key, gpointer value, gpointer user_data)
{
    GtkTwoSat      *two_sat = GTK_TWO_SAT(user_data);
    sat_t          *sat = SAT(value);

    (void)key;

    two_sat->sats = g_slist_insert_sorted(two_sat->sats, sat,
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


// Select first satellite from drop down menu
static void select_first_satellite(GtkWidget * menuitem, gpointer data)
{
    GtkTwoSat      *tsat = GTK_TWO_SAT(data);
    guint           i =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "index"));
    gchar          *title;
    sat_t          *sat;

    // There are many "ghost"-triggering of this signal, but we only need to
    // make a new selection when the received menuitem is selected
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
    {
        tsat->selected1 = i;

        sat = SAT(g_slist_nth_data(tsat->sats, i));

        title = g_markup_printf_escaped("<b>First Satellite: %s</b>", sat->nickname);
        gtk_label_set_markup(GTK_LABEL(tsat->header1), title);
        g_free(title);
    }
}

// Select second satellite from drop down menu
static void select_second_satellite(GtkWidget * menuitem, gpointer data)
{
    GtkTwoSat      *tsat = GTK_TWO_SAT(data);
    guint           i =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuitem), "index"));
    gchar          *title;
    sat_t          *sat;

    // There are many "ghost"-triggering of this signal, but we only need to
    // make a new selection when the received menuitem is selected
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
    {
        tsat->selected2 = i;

        sat = SAT(g_slist_nth_data(tsat->sats, i));

        title = g_markup_printf_escaped("<b>Second Satellite: %s</b>", sat->nickname);
        gtk_label_set_markup(GTK_LABEL(tsat->header2), title);
        g_free(title);
    }
}

// Two sat options menu, first sat
static void gtk_two_sat_popup_first_cb(GtkWidget * button, gpointer data)
{
    GtkTwoSat   *two_sat = GTK_TWO_SAT(data);
    GtkWidget   *menu;
    GtkWidget   *menuitem;
    GtkWidget   *label;
    GSList      *group = NULL;
    gchar       *buff;
    sat_t       *sat1;
    sat_t       *sati;      // Used to create list of satellites
    guint       i, n;

    sat1 = SAT(g_slist_nth_data(two_sat->sats, two_sat->selected1));
    if (sat1 == NULL){
        sat_log_log(SAT_LOG_LEVEL_ERROR, "%s %d: Sat is NULL", __FILE__, __LINE__);
        return;
    }

    n = g_slist_length(two_sat->sats);

    menu = gtk_menu_new();

    // Satellite name/info
    menuitem = gtk_menu_item_new();
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    buff = g_markup_printf_escaped("<b>%s</b>", sat1->nickname);
    gtk_label_set_markup(GTK_LABEL(label), buff);
    g_free(buff);
    gtk_container_add(GTK_CONTAINER(menuitem), label);

    // Attach data to menuitem and connect callback
    g_object_set_data(G_OBJECT(menuitem), "sat", sat1);
    g_object_set_data(G_OBJECT(menuitem), "qth", two_sat->qth);
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_sat_info_menu_cb), gtk_widget_get_toplevel(button));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Separator
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Add the menu items for current, next, and future passese
    add_pass_menu_items(menu, sat1, two_sat->qth, &two_sat->tstamp, data);

    // Separator
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Select sat
    for (i = 0; i< n; i++)
    {
        sati = SAT(g_slist_nth_data(two_sat->sats, i));

        menuitem = gtk_radio_menu_item_new_with_label(group, sati->nickname);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));

        if (i == two_sat->selected1)
        {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
        }

        // Store item index so that it is available in the callback
        g_object_set_data(G_OBJECT(menuitem), "index", GUINT_TO_POINTER(i));
        g_signal_connect_after(menuitem, "activate", G_CALLBACK(select_first_satellite), two_sat);
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

// Two sat options menu, second sat
static void gtk_two_sat_popup_second_cb(GtkWidget * button, gpointer data)
{
    GtkTwoSat   *two_sat = GTK_TWO_SAT(data);
    GtkWidget   *menu;
    GtkWidget   *menuitem;
    GtkWidget   *label;
    GSList      *group = NULL;
    gchar       *buff;
    sat_t       *sat2;
    sat_t       *sati;      // used to create list of satellites
    guint       i, n;

    sat2 = SAT(g_slist_nth_data(two_sat->sats, two_sat->selected2));
    if (sat2 == NULL)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, "%s %d: Sat is NULL", __FILE__, __LINE__);
        return;
    }

    n = g_slist_length(two_sat->sats);

    menu = gtk_menu_new();

    // Satellite name/info
    menuitem = gtk_menu_item_new();
    label = gtk_label_new(NULL);
    g_object_set(label, "xalign", 0.0f, "yalign", 0.5f, NULL);
    buff = g_markup_printf_escaped("<b>%s</b>", sat2->nickname);
    gtk_label_set_markup(GTK_LABEL(label), buff);
    g_free(buff);
    gtk_container_add(GTK_CONTAINER(menuitem), label);

    // Attach data to menuitem and connect callback
    g_object_set_data(G_OBJECT(menuitem), "sat", sat2);
    g_object_set_data(G_OBJECT(menuitem), "qth", two_sat->qth);
    g_signal_connect(menuitem, "activate", G_CALLBACK(show_sat_info_menu_cb), gtk_widget_get_toplevel(button));

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Separator
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Add the menu items for current, next and future passes
    add_pass_menu_items(menu, sat2, two_sat->qth, &two_sat->tstamp, data);

    // Separator
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

    // Select sat
    for (i = 0; i < n; i++)
    {
        sati = SAT(g_slist_nth_data(two_sat->sats, i));

        menuitem = gtk_radio_menu_item_new_with_label(group, sati->nickname);
        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));

        if (i == two_sat->selected2)
        {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
        }

        // Store item index so that it is available in the callback
        g_object_set_data(G_OBJECT(menuitem), "index", GUINT_TO_POINTER(i));
        g_signal_connect_after(menuitem, "activate", G_CALLBACK(select_second_satellite), two_sat);
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

// Refresh internal references to the satellites
void gtk_two_sat_reload_sats(GtkWidget *two_sat, GHashTable * sats)
{
    // free GSLists
    g_slist_free(GTK_TWO_SAT(two_sat)->sats);
    GTK_TWO_SAT(two_sat)->sats = NULL;

    // reload satellites
    g_hash_table_foreach(sats, store_sats, two_sat);
}

// Reload configurations
/*
    @param widget   GtkTwoSat widget
    @param newcfg   The new configuration data for the module
    @param sats     Satellites
    @param local    Flag indicating whether reconfiguration is requested from
                    local configuration dialog
*/
void gtk_two_sat_reconf(GtkWidget * widget, GKeyFile * newcfg, GHashTable * sats, qth_t * qth, gboolean local)
{
    guint32 fields;

    // Store pointer to new cfg data
    GTK_TWO_SAT(widget)->cfgdata = newcfg;

    // Get visible firleds from new configuration
    fields = mod_cfg_get_int(newcfg, MOD_CFG_TWO_SAT_SECTION, 
                             MOD_CFG_TWO_SAT_FIELDS, SAT_CFG_INT_TWO_SAT_FIELDS);

    if (fields != GTK_TWO_SAT(widget)->flags)
    {
        // Update flags
        GTK_TWO_SAT(widget)->flags = fields;
    }

    // If this is a local reconfiguration, sats may have changed
    if (local)
    {
        gtk_two_sat_reload_sats(widget, sats);
    }

    // QTH may have changed too since we have a default QTH
    GTK_TWO_SAT(widget)->qth = qth;

    // Get refresh rate and cycle counter
    GTK_TWO_SAT(widget)->refresh = mod_cfg_get_int(newcfg,
                                                   MOD_CFG_TWO_SAT_SECTION,
                                                   MOD_CFG_TWO_SAT_REFRESH, 
                                                   SAT_CFG_INT_TWO_SAT_REFRESH);
    GTK_TWO_SAT(widget)->counter = 1;
}

// Select new first satellite
void gtk_two_sat_select_first_sat(GtkWidget * two_sat, gint catnum)
{
    GtkTwoSat   *tsat = GTK_TWO_SAT(two_sat);
    sat_t       *sat = NULL;
    gchar       *title;
    gboolean     foundsat = FALSE;
    gint         i, n;

    // Find satellite with catnum
    n = g_slist_length(tsat->sats);
    for (i = 0; i < n; i++)
    {
        sat = SAT(g_slist_nth_data(tsat->sats, i));
        if (sat->tle.catnr == catnum)
        {
            // Found satellite
            tsat->selected1 = i;
            foundsat = TRUE;

            // Exit loop
            i = n;
        }
    }

    if (!foundsat)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not find satellite with catalog number %d"),
                    __func__, catnum);
        return;
    }
    title = g_markup_printf_escaped("<b>First Satellite: %s</b>", sat->nickname);
    gtk_label_set_markup(GTK_LABEL(tsat->header1), title);
    g_free(title);
}

// Select new second satellite
void gtk_two_sat_select_second_sat(GtkWidget * two_sat, gint catnum)
{
    GtkTwoSat   *tsat = GTK_TWO_SAT(two_sat);
    sat_t       *sat = NULL;
    gchar       *title;
    gboolean     foundsat = FALSE;
    gint         i, n;

    // Find satellite with catnum
    n = g_slist_length(tsat->sats);
    for (i = 0; i < n; i++)
    {
        sat = SAT(g_slist_nth_data(tsat->sats, i));
        if (sat->tle.catnr == catnum)
        {
            // Found satellite
            tsat->selected2 = i;
            foundsat = TRUE;

            // Exit loop
            i = n;
        }
    }

    if (!foundsat)
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR,
                    _("%s: Could not find satellite with catalog number %d"),
                    __func__, catnum);
        return;
    }
    title = g_markup_printf_escaped("<b>Second Satellite: %s</b>", sat->nickname);
    gtk_label_set_markup(GTK_LABEL(tsat->header2), title);
    g_free(title);
}

// Update satellites for first satellite
void gtk_two_sat_update_first(GtkWidget * widget)
{
    GtkTwoSat   *tsat = GTK_TWO_SAT(widget);
    guint       i;

    // Do some sanity checks
    if ((tsat == NULL) || (!IS_GTK_TWO_SAT(tsat)))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Invald GtkTWoSat!"), __func__);
        return;
    }

    // Check refresh rate
    if (tsat->counter < tsat->refresh)
    {
        tsat->counter++;
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    "%s %d: counter is %d, refresh is %d",
                    __FILE__, __LINE__, tsat->counter, tsat->refresh);
    }
    else
    {
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    "%s %d: counter < refresh rate condition passed",
                    __FILE__, __LINE__);
        // Calculate here to avoid double calc
        if ((tsat->flags & TWO_SAT_FLAG_RA) || (tsat->flags & TWO_SAT_FLAG_DEC))
        {
            obs_astro_t     astro;
            sat_t           *sat = SAT(g_slist_nth_data(tsat->sats, tsat->selected1));

            Calculate_RADec(sat, tsat->qth, &astro);
            sat->ra = Degrees(astro.ra);
            sat->dec = Degrees(astro.dec);
        }

        // Update visible fields one by one
        for (i = 0; i < TWO_SAT_FIELD_NUMBER; i++)
        {
            if (tsat->flags & (1 << i))
                update_field_first(tsat, i);
        }
        tsat->counter = 1;
    }
}

// Update satellites for second satellite
void gtk_two_sat_update_second(GtkWidget * widget)
{
    GtkTwoSat   *tsat = GTK_TWO_SAT(widget);
    guint       i;

    // Do some sanity checks
    if ((tsat == NULL) || (!IS_GTK_TWO_SAT(tsat)))
    {
        sat_log_log(SAT_LOG_LEVEL_ERROR, _("%s: Invald GtkTWoSat!"), __func__);
        return;
    }

    // Check refresh rate
    if (tsat->counter < tsat->refresh)
    {
        tsat->counter++;
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    "%s %d: counter is %d, refresh is %d",
                    __FILE__, __LINE__, tsat->counter, tsat->refresh);
    }
    else
    {
        /*
        sat_log_log(SAT_LOG_LEVEL_DEBUG,
                    "%s %d: counter < refresh rate condition passed",
                    __FILE__, __LINE__);
        */
        // Calculate here to avoid double calc
        if ((tsat->flags & TWO_SAT_FLAG_RA) || (tsat->flags & TWO_SAT_FLAG_DEC))
        {
            obs_astro_t     astro;
            sat_t           *sat = SAT(g_slist_nth_data(tsat->sats, tsat->selected2));

            Calculate_RADec(sat, tsat->qth, &astro);
            sat->ra = Degrees(astro.ra);
            sat->dec = Degrees(astro.dec);
        }

        // Update visible fields one by one
        for (i = 0; i < TWO_SAT_FIELD_NUMBER; i++)
        {
            if (tsat->flags & (1 << i))
                update_field_second(tsat, i);
        }
        tsat->counter = 1;
    }
}

GType gtk_two_sat_get_type()
{
    static GType    gtk_two_sat_type = 0;
    
    if (!gtk_two_sat_type)
    {
        static const GTypeInfo gtk_two_sat_info = {
            sizeof(GtkTwoSatClass),
            NULL,                       // base_init
            NULL,                       // base_finalise
            (GClassInitFunc) gtk_two_sat_class_init,
            NULL,                       // class_finalise
            NULL,                       // class_data
            sizeof(GtkTwoSat),
            5,                          // n_preallocs
            (GInstanceInitFunc) gtk_two_sat_init,
            NULL
        };

        gtk_two_sat_type = g_type_register_static(GTK_TYPE_BOX, "GtkTwoSat", 
                                                  &gtk_two_sat_info, 0);
    }

    return gtk_two_sat_type;
}

GtkWidget * gtk_two_sat_new(GKeyFile * cfgdata, GHashTable * sats, qth_t * qth,
                            guint32 fields)
{
    GtkWidget * widget;
    GtkTwoSat * two_sat;
    GtkWidget * hbox1, * hbox2;         // Horizontal box for header
    GtkWidget * label1;
    GtkWidget * label2;
    sat_t *sat1, *sat2;
    gchar *title1, *title2;
    guint i;
    gint selectedcatnum1, selectedcatnum2;

    widget = g_object_new(GTK_TYPE_TWO_SAT, NULL);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(widget), GTK_ORIENTATION_VERTICAL);
    two_sat = GTK_TWO_SAT(widget);

    two_sat->update_first = gtk_two_sat_update_first;
    two_sat->update_second = gtk_two_sat_update_second;

    // Read configuration data
    // ;;

    g_hash_table_foreach(sats, store_sats, widget);
    two_sat->selected1 = 0;
    two_sat->selected2 = 0;
    two_sat->qth = qth;
    two_sat->cfgdata = cfgdata;

    // Initialise column flags
    if (fields > 0)
    {
        two_sat->flags = fields;
    }
    else
    {
        two_sat->flags = mod_cfg_get_int(cfgdata,
                                         MOD_CFG_TWO_SAT_SECTION,
                                         MOD_CFG_TWO_SAT_FIELDS,
                                         SAT_CFG_INT_TWO_SAT_FIELDS);
    }
    
    // Get refresh rate and cycle counter
    two_sat->refresh = mod_cfg_get_int(cfgdata,
                                       MOD_CFG_TWO_SAT_SECTION,
                                       MOD_CFG_TWO_SAT_REFRESH,
                                       SAT_CFG_INT_TWO_SAT_REFRESH);
    two_sat->refresh = 1;

    // Get selected catnum for first satellite if available
    selectedcatnum1 = mod_cfg_get_int(cfgdata,
                                     MOD_CFG_TWO_SAT_SECTION,
                                     MOD_CFG_TWO_SAT_SELECT_FIRST,
                                     SAT_CFG_INT_TWO_SAT_SELECT_FIRST);
    // Get selected catnum for second satellite if available
    selectedcatnum2 = mod_cfg_get_int(cfgdata,
                                      MOD_CFG_TWO_SAT_SECTION,
                                      MOD_CFG_TWO_SAT_SELECT_SECOND,
                                      SAT_CFG_INT_TWO_SAT_SELECT_SECOND);
    
    // Popup button for first satellite
    two_sat->popup_button1 = gpredict_mini_mod_button("gpredict-mod-popup.png",
                                                     _("Satellite options / shortcuts"));
    g_signal_connect(two_sat->popup_button1, "clicked", G_CALLBACK(gtk_two_sat_popup_first_cb), widget);

    hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox1), two_sat->popup_button1, FALSE, FALSE, 0);

    // Create header for first satellite
    sat1 = SAT(g_slist_nth_data(two_sat->sats, 0));
    title1 = g_markup_printf_escaped("<b>First Satellite: %s</b>",
                                    sat1 ? sat1->nickname : "noname");
    two_sat->header1 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(two_sat->header1), title1);
    g_free(title1);
    g_object_set(two_sat->header1, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_box_pack_start(GTK_BOX(hbox1), two_sat->header1, TRUE, TRUE, 10);

    // Create and initialise table for first satellite
    two_sat->table1 = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(two_sat->table1), 5);
    gtk_grid_set_row_spacing(GTK_GRID(two_sat->table1), 0);
    gtk_grid_set_column_spacing(GTK_GRID(two_sat->table1), 5);

    // Create and add label widgets for first satellite
    for (i = 0; i< TWO_SAT_FIELD_NUMBER; i++)
    {
        if (two_sat->flags & (1 << i))
        {
            label1 = gtk_label_new(TWO_SAT_FIELD_TITLE[i]);
            g_object_set(label1, "xalign", 1.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(two_sat->table1), label1, 0, i, 1, 1);

            label2 = gtk_label_new("-");
            g_object_set(label2, "xalign", 0.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(two_sat->table1), label2, 2, i, 1, 1);
            two_sat->labels1[i] = label2;

            // Add tooltips
            gtk_widget_set_tooltip_text(label1, TWO_SAT_FIELD_HINT[i]);
            gtk_widget_set_tooltip_text(label2, TWO_SAT_FIELD_HINT[i]);

            label1 = gtk_label_new(":");
            gtk_grid_attach(GTK_GRID(two_sat->table1), label1, 1, i, 1, 1);
        }
        else
        {
            two_sat->labels1[i] = NULL;
        }
    }


    // Popup button for second satellite
    two_sat->popup_button2 = gpredict_mini_mod_button("gpredict-mod-popup.png",
                                                      _("Satellite options / shortcuts"));
    g_signal_connect(two_sat->popup_button2, "clicked",
                     G_CALLBACK(gtk_two_sat_popup_second_cb), widget);

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(hbox2), two_sat->popup_button2, FALSE, FALSE, 0);

    // Create header for second satellite
    sat2 = SAT(g_slist_nth_data(two_sat->sats, 0));
    title2 = g_markup_printf_escaped("<b>Second Satellite: %s</b>",
                                    sat2 ? sat2->nickname : "noname");
    two_sat->header2 = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(two_sat->header2), title2);
    g_free(title2);
    g_object_set(two_sat->header2, "xalign", 0.0f, "yalign", 0.5f, NULL);
    gtk_box_pack_start(GTK_BOX(hbox2), two_sat->header2, TRUE, TRUE, 10);

    // Create and initialise table for second satellite
    two_sat->table2 = gtk_grid_new();
    gtk_container_set_border_width(GTK_CONTAINER(two_sat->table2), 5);
    gtk_grid_set_row_spacing(GTK_GRID(two_sat->table2), 0);
    gtk_grid_set_column_spacing(GTK_GRID(two_sat->table2), 5);

    // Create and add label widgets for second satellite
    for (i = 0; i < TWO_SAT_FIELD_NUMBER; i++)
    {
        if (two_sat->flags & (1 << i))
        {
            label1 = gtk_label_new(TWO_SAT_FIELD_TITLE[i]);
            g_object_set(label1, "xalign", 1.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(two_sat->table2), label1, 0, i, 1, 1);

            label2 = gtk_label_new("-");
            g_object_set(label2, "xalign", 0.0f, "yalign", 0.5f, NULL);
            gtk_grid_attach(GTK_GRID(two_sat->table2), label2, 2, i, 1, 1);
            two_sat->labels2[i] = label2;

            // Add tooltips
            gtk_widget_set_tooltip_text(label1, (TWO_SAT_FIELD_HINT[i]));
            gtk_widget_set_tooltip_text(label2, (TWO_SAT_FIELD_HINT[i]));

            label1 = gtk_label_new(":");
            gtk_grid_attach(GTK_GRID(two_sat->table2), label1, 1, i, 1, 1);
        }
        else
        {
            two_sat->labels2[i] = NULL;
        }
    }

    // Create a scrolled window
    two_sat->swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(two_sat->swin),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // Create a container for all headers and tables
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);

    // First satellite header and table
    gtk_box_pack_start(GTK_BOX(content_box), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), two_sat->table1, FALSE, FALSE, 0);

    // Second satellite header and table
    gtk_box_pack_start(GTK_BOX(content_box), hbox2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content_box), two_sat->table2, FALSE, FALSE, 0);

    // Add to scrolled window
    gtk_container_add(GTK_CONTAINER(two_sat->swin), content_box);

    // Pack scrolled window into main widget
    gtk_box_pack_end(GTK_BOX(widget), two_sat->swin, TRUE, TRUE, 0);

    gtk_widget_show_all(widget);

    if (selectedcatnum1)
    {
        gtk_two_sat_select_first_sat(widget, selectedcatnum1);
    }
    else if (selectedcatnum2)
    {
        gtk_two_sat_select_second_sat(widget, selectedcatnum2);
    }
    
    return widget;
}