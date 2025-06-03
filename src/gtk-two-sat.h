#ifndef __GTK_TWO_SAT_H__
#define __GTK_TWO_SAT_H__ 1

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
    TWO_SAT_FIELD_AZ = 0,    /*!< Azimuth. */
    TWO_SAT_FIELD_EL,        /*!< Elvation. */
    TWO_SAT_FIELD_DIR,       /*!< Direction, satellite on its way up or down. */
    TWO_SAT_FIELD_RA,        /*!< Right Ascension. */
    TWO_SAT_FIELD_DEC,       /*!< Declination. */
    TWO_SAT_FIELD_RANGE,     /*!< Range. */
    TWO_SAT_FIELD_RANGE_RATE,        /*!< Range rate. */
    TWO_SAT_FIELD_NEXT_EVENT,        /*!< Next event AOS or LOS depending on El. */
    TWO_SAT_FIELD_AOS,       /*!< Next AOS regardless of El. */
    TWO_SAT_FIELD_LOS,       /*!< Next LOS regardless of El. */
    TWO_SAT_FIELD_LAT,       /*!< Latitude. */
    TWO_SAT_FIELD_LON,       /*!< Longitude. */
    TWO_SAT_FIELD_SSP,       /*!< Sub satellite point grid square */
    TWO_SAT_FIELD_FOOTPRINT, /*!< Footprint. */
    TWO_SAT_FIELD_ALT,       /*!< Altitude. */
    TWO_SAT_FIELD_VEL,       /*!< Velocity. */
    TWO_SAT_FIELD_DOPPLER,   /*!< Doppler shift at 100 MHz. */
    TWO_SAT_FIELD_LOSS,      /*!< Path Loss at 100 MHz. */
    TWO_SAT_FIELD_DELAY,     /*!< Signal delay */
    TWO_SAT_FIELD_MA,        /*!< Mean Anomaly. */
    TWO_SAT_FIELD_PHASE,     /*!< Phase. */
    TWO_SAT_FIELD_ORBIT,     /*!< Orbit Number. */
    TWO_SAT_FIELD_VISIBILITY,   /*!< Visibility. */
    TWO_SAT_FIELD_DISTANCE,     /*!< Distance between the two satellites */
    TWO_SAT_FIELD_SKR_INTER,    /*!< Secret Key Rate (satellite to satellite) */
    TWO_SAT_FIELD_SKR_DOWN,     /*!< Secret Key Rate (satellite to ground, downlink) */
    TWO_SAT_FIELD_SKR_UP,       /*!< Secret Key Rate (ground to satellite, uplink) */
    TWO_SAT_FIELD_NUMBER
} two_sat_field_t;

/** Fieldnum Flags */
typedef enum {
    TWO_SAT_FLAG_AZ = 1 << TWO_SAT_FIELD_AZ,      /*!< Azimuth. */
    TWO_SAT_FLAG_EL = 1 << TWO_SAT_FIELD_EL,      /*!< Elvation. */
    TWO_SAT_FLAG_DIR = 1 << TWO_SAT_FIELD_DIR,    /*!< Direction */
    TWO_SAT_FLAG_RA = 1 << TWO_SAT_FIELD_RA,      /*!< Right Ascension. */
    TWO_SAT_FLAG_DEC = 1 << TWO_SAT_FIELD_DEC,    /*!< Declination. */
    TWO_SAT_FLAG_RANGE = 1 << TWO_SAT_FIELD_RANGE,        /*!< Range. */
    TWO_SAT_FLAG_RANGE_RATE = 1 << TWO_SAT_FIELD_RANGE_RATE,      /*!< Range rate. */
    TWO_SAT_FLAG_NEXT_EVENT = 1 << TWO_SAT_FIELD_NEXT_EVENT,      /*!< Next event. */
    TWO_SAT_FLAG_AOS = 1 << TWO_SAT_FIELD_AOS,    /*!< Next AOS. */
    TWO_SAT_FLAG_LOS = 1 << TWO_SAT_FIELD_LOS,    /*!< Next LOS. */
    TWO_SAT_FLAG_LAT = 1 << TWO_SAT_FIELD_LAT,    /*!< Latitude. */
    TWO_SAT_FLAG_LON = 1 << TWO_SAT_FIELD_LON,    /*!< Longitude. */
    TWO_SAT_FLAG_SSP = 1 << TWO_SAT_FIELD_SSP,    /*!< SSP grid square */
    TWO_SAT_FLAG_FOOTPRINT = 1 << TWO_SAT_FIELD_FOOTPRINT,        /*!< Footprint. */
    TWO_SAT_FLAG_ALT = 1 << TWO_SAT_FIELD_ALT,    /*!< Altitude. */
    TWO_SAT_FLAG_VEL = 1 << TWO_SAT_FIELD_VEL,    /*!< Velocity. */
    TWO_SAT_FLAG_DOPPLER = 1 << TWO_SAT_FIELD_DOPPLER,    /*!< Doppler shift. */
    TWO_SAT_FLAG_LOSS = 1 << TWO_SAT_FIELD_LOSS,  /*!< Path Loss. */
    TWO_SAT_FLAG_DELAY = 1 << TWO_SAT_FIELD_DELAY,        /*!< Delay */
    TWO_SAT_FLAG_MA = 1 << TWO_SAT_FIELD_MA,      /*!< Mean Anomaly. */
    TWO_SAT_FLAG_PHASE = 1 << TWO_SAT_FIELD_PHASE,        /*!< Phase. */
    TWO_SAT_FLAG_ORBIT = 1 << TWO_SAT_FIELD_ORBIT,        /*!< Orbit Number. */
    TWO_SAT_FLAG_VISIBILITY = 1 << TWO_SAT_FIELD_VISIBILITY,       /*!< Visibility. */
    TWO_SAT_FLAG_DISTANCE = 1 << TWO_SAT_FIELD_DISTANCE,    /*!< Distance between the two satellites */
    TWO_SAT_FLAG_SKR_INTER = 1 << TWO_SAT_FIELD_SKR_INTER,  /*!< Secret Key Rate (satellite to satellite) */
    TWO_SAT_FLAG_SKR_DOWN = 1 << TWO_SAT_FIELD_SKR_DOWN,    /*!< Secret Key Rate (satellite to ground, downlink) */
    TWO_SAT_FLAG_SKR_UP = 1 << TWO_SAT_FIELD_SKR_UP         /*!< Secret Key Rate (ground to satellite, uplink) */
} two_sat_flag_t;

#define GTK_TYPE_TWO_SAT          (gtk_two_sat_get_type ())
#define GTK_TWO_SAT(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj,\
                                        gtk_two_sat_get_type (),\
                                        GtkTwoSat)
#define GTK_TWO_SAT_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass,\
                                        gtk_two_sat_get_type (),\
                                        GtkTwoSatClass)
#define IS_GTK_TWO_SAT(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_two_sat_get_type ())

typedef struct _gtk_two_sat GtkTwoSat;
typedef struct _GtkTwoSatClass GtkTwoSatClass;

struct _gtk_two_sat {
    GtkBox      vbox;
    
    GtkWidget       *header1;       /*<! Header label for first satellite */
    GtkWidget       *header2;       /*<! Header label for second satellite */

    /*<! GtkLabels displaying the data for first satellite */
    GtkWidget       *labels1[TWO_SAT_FIELD_NUMBER];
    /*<! GtkLabels displaying the data for second satellite */
    GtkWidget       *labels2[TWO_SAT_FIELD_NUMBER];
    // above may be needed, keep for debugging ideas

    //GtkWidget       *swin1;         /*<! Scrolled window for first satellite */
    //GtkWidget       *swin2;         /*<! Scrolled window for second satellite */
    GtkWidget       *swin;          /*<! Scrolled window for satellite */
    GtkWidget       *table1;        /*<! Table for first satellite */
    GtkWidget       *table2;        /*<! Table for second satellite */

    GtkWidget       *popup_button1; /*<! Popup button for first satellite */
    GtkWidget       *popup_button2; /*<! Popup button for second satellite */

    GKeyFile        *cfgdata;       /*<! Configuration data */
    GSList          *sats;          /*<! Satellites data SList */
    qth_t           *qth;           /*<! Pointer to current location */

    guint32         flags;          /*<! Flags indicating which columns are visible */
    guint           refresh;        /*<! Refresh rate */
    guint           counter;        /*<! Cycle counter */
    guint           selected1;      /*<! Index of first selected satellite */
    guint           selected2;      /*<! Index of second selected satellite */

    /*<! Time stamp of calculations; update by GtkSatModule */
    gdouble         tstamp;

    /* Update function */
    void        (*update_first) (GtkWidget * widget);
    void        (*update_second) (GtkWidget * widget);
};

struct _GtkTwoSatClass {
    GtkBoxClass     parent_class;
};

GType       gtk_two_sat_get_type(void);
GtkWidget  *gtk_two_sat_new(GKeyFile * cfgdata, GHashTable * sats,
                            qth_t * qth, guint32 fields);
void        gtk_two_sat_update_first(GtkWidget * widget);
void        gtk_two_sat_update_second(GtkWidget * widget);
void        gtk_two_sat_reconf(GtkWidget * widget, GKeyFile * newcfg,
                               GHashTable * sats, qth_t * qth, gboolean local);

void        gtk_two_sat_reload_sats(GtkWidget * two_sat, GHashTable * sats);
void        gtk_two_sat_select_first_sat(GtkWidget * two_sat, gint catnum);
void        gtk_two_sat_select_second_sat(GtkWidget * two_sat, gint catnum);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */


#endif      /* __GTK_TWO_SAT_H__ */