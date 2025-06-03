#ifndef __GTK_SECOND_SAT_H__
#define __GTK_SECOND_SAT_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "gtk-sat-module.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/** Symbolic references to columns */
typedef enum {
    SECOND_SAT_FIELD_AZ = 0,    /*!< Azimuth. */
    SECOND_SAT_FIELD_EL,        /*!< Elvation. */
    SECOND_SAT_FIELD_DIR,       /*!< Direction, satellite on its way up or down. */
    SECOND_SAT_FIELD_RA,        /*!< Right Ascension. */
    SECOND_SAT_FIELD_DEC,       /*!< Declination. */
    SECOND_SAT_FIELD_RANGE,     /*!< Range. */
    SECOND_SAT_FIELD_RANGE_RATE,        /*!< Range rate. */
    SECOND_SAT_FIELD_NEXT_EVENT,        /*!< Next event AOS or LOS depending on El. */
    SECOND_SAT_FIELD_AOS,       /*!< Next AOS regardless of El. */
    SECOND_SAT_FIELD_LOS,       /*!< Next LOS regardless of El. */
    SECOND_SAT_FIELD_LAT,       /*!< Latitude. */
    SECOND_SAT_FIELD_LON,       /*!< Longitude. */
    SECOND_SAT_FIELD_SSP,       /*!< Sub satellite point grid square */
    SECOND_SAT_FIELD_FOOTPRINT, /*!< Footprint. */
    SECOND_SAT_FIELD_ALT,       /*!< Altitude. */
    SECOND_SAT_FIELD_VEL,       /*!< Velocity. */
    SECOND_SAT_FIELD_DOPPLER,   /*!< Doppler shift at 100 MHz. */
    SECOND_SAT_FIELD_LOSS,      /*!< Path Loss at 100 MHz. */
    SECOND_SAT_FIELD_DELAY,     /*!< Signal delay */
    SECOND_SAT_FIELD_MA,        /*!< Mean Anomaly. */
    SECOND_SAT_FIELD_PHASE,     /*!< Phase. */
    SECOND_SAT_FIELD_ORBIT,     /*!< Orbit Number. */
    SECOND_SAT_FIELD_VISIBILITY,        /*!< Visibility. */
    SECOND_SAT_FIELD_SKR,              /*!< Secret Key Rate */
    SECOND_SAT_FIELD_NUMBER
} second_sat_field_t;

/** Fieldnum Flags */
typedef enum {
    SECOND_SAT_FLAG_AZ = 1 << SECOND_SAT_FIELD_AZ,      /*!< Azimuth. */
    SECOND_SAT_FLAG_EL = 1 << SECOND_SAT_FIELD_EL,      /*!< Elvation. */
    SECOND_SAT_FLAG_DIR = 1 << SECOND_SAT_FIELD_DIR,    /*!< Direction */
    SECOND_SAT_FLAG_RA = 1 << SECOND_SAT_FIELD_RA,      /*!< Right Ascension. */
    SECOND_SAT_FLAG_DEC = 1 << SECOND_SAT_FIELD_DEC,    /*!< Declination. */
    SECOND_SAT_FLAG_RANGE = 1 << SECOND_SAT_FIELD_RANGE,        /*!< Range. */
    SECOND_SAT_FLAG_RANGE_RATE = 1 << SECOND_SAT_FIELD_RANGE_RATE,      /*!< Range rate. */
    SECOND_SAT_FLAG_NEXT_EVENT = 1 << SECOND_SAT_FIELD_NEXT_EVENT,      /*!< Next event. */
    SECOND_SAT_FLAG_AOS = 1 << SECOND_SAT_FIELD_AOS,    /*!< Next AOS. */
    SECOND_SAT_FLAG_LOS = 1 << SECOND_SAT_FIELD_LOS,    /*!< Next LOS. */
    SECOND_SAT_FLAG_LAT = 1 << SECOND_SAT_FIELD_LAT,    /*!< Latitude. */
    SECOND_SAT_FLAG_LON = 1 << SECOND_SAT_FIELD_LON,    /*!< Longitude. */
    SECOND_SAT_FLAG_SSP = 1 << SECOND_SAT_FIELD_SSP,    /*!< SSP grid square */
    SECOND_SAT_FLAG_FOOTPRINT = 1 << SECOND_SAT_FIELD_FOOTPRINT,        /*!< Footprint. */
    SECOND_SAT_FLAG_ALT = 1 << SECOND_SAT_FIELD_ALT,    /*!< Altitude. */
    SECOND_SAT_FLAG_VEL = 1 << SECOND_SAT_FIELD_VEL,    /*!< Velocity. */
    SECOND_SAT_FLAG_DOPPLER = 1 << SECOND_SAT_FIELD_DOPPLER,    /*!< Doppler shift. */
    SECOND_SAT_FLAG_LOSS = 1 << SECOND_SAT_FIELD_LOSS,  /*!< Path Loss. */
    SECOND_SAT_FLAG_DELAY = 1 << SECOND_SAT_FIELD_DELAY,        /*!< Delay */
    SECOND_SAT_FLAG_MA = 1 << SECOND_SAT_FIELD_MA,      /*!< Mean Anomaly. */
    SECOND_SAT_FLAG_PHASE = 1 << SECOND_SAT_FIELD_PHASE,        /*!< Phase. */
    SECOND_SAT_FLAG_ORBIT = 1 << SECOND_SAT_FIELD_ORBIT,        /*!< Orbit Number. */
    SECOND_SAT_FLAG_VISIBILITY = 1 << SECOND_SAT_FIELD_VISIBILITY,       /*!< Visibility. */
    SECOND_SAT_FLAG_SKR = 1 << SECOND_SAT_FIELD_SKR    /*!< Secret Key Rate*/
} second_sat_flag_t;

#define GTK_TYPE_SECOND_SAT          (gtk_second_sat_get_type ())
#define GTK_SECOND_SAT(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                        gtk_second_sat_get_type (),\
                                        GtkSecondSat)
#define GTK_SECOND_SAT_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                        gtk_second_sat_get_type (),\
                                        GtkSecondSatClass)
#define IS_GTK_SECOND_SAT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_second_sat_get_type ())

typedef struct _gtk_second_sat GtkSecondSat;
typedef struct _GtkSecondSatClass GtkSecondSatClass;

struct _gtk_second_sat {
    GtkBox          vbox;

    GtkWidget      *header;     /*!< Header label, ie. satellite name. */

    GtkWidget      *labels[SECOND_SAT_FIELD_NUMBER];    /*!< GtkLabels displaying the data. */

    GtkWidget      *swin;       /*!< scrolled window. */
    GtkWidget      *table;      /*!< table. */

    GtkWidget      *popup_button;       /*!< Popup button. */


    GKeyFile       *cfgdata;    /*!< Configuration data. */
    GSList         *sats;       /*!< Satellites. */
    qth_t          *qth;        /*!< Pointer to current location. */


    guint32         flags;      /*!< Flags indicating which columns are visible. */
    guint           refresh;    /*!< Refresh rate. */
    guint           counter;    /*!< cycle counter. */
    //guint           selected;   /*!< index of selected sat. */
    guint           selected2;  /*!< index of second selected sat */

    gdouble         tstamp;     /*!< time stamp of calculations; update by GtkSatModule */

    void            (*update) (GtkWidget * widget);     /*!< update function */
};

struct _GtkSecondSatClass {
    GtkBoxClass     parent_class;
};

GType           gtk_second_sat_get_type(void);
GtkWidget      *gtk_second_sat_new(GKeyFile * cfgdata,
                                   GHashTable * sats,
                                   qth_t * qth, guint32 fields);
void            gtk_second_sat_update(GtkWidget * widget);
void            gtk_second_sat_reconf(GtkWidget * widget,
                                      GKeyFile * newcfg,
                                      GHashTable * sats,
                                      qth_t * qth, gboolean local);

void            gtk_second_sat_reload_sats(GtkWidget * second_sat,
                                           GHashTable * sats);
void            gtk_second_sat_select_sat(GtkWidget * second_sat, gint catnum);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif /* __GTK_SECOND_SAT_H__ */
