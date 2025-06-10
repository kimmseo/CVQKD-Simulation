#ifndef __GTK_MULTIPLE_SAT_H__
#define __GTK_MULTIPLE_SAT_H__ 1

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

#define NUMBER_OF_SATS 10
#define SATS_PER_ROW 2


/* Symbolic references to columns */
typedef enum {
    MULTIPLE_SAT_FIELD_AZ = 0,  // Azimuth
    MULTIPLE_SAT_FIELD_EL,      // Elevation
    MULTIPLE_SAT_FIELD_DIR,     // Direction, satellite on its way up or down
    MULTIPLE_SAT_FIELD_RA,      // Right Ascension
    MULTIPLE_SAT_FIELD_DEC,     // Declination
    MULTIPLE_SAT_FIELD_RANGE,   // Range
    MULTIPLE_SAT_FIELD_RANGE_RATE,  // Range rate
    MULTIPLE_SAT_FIELD_NEXT_EVENT,  // Next event AOS or LOS depending on El
    MULTIPLE_SAT_FIELD_AOS,     // Next AOS regardless of El
    MULTIPLE_SAT_FIELD_LOS,     // Next LOS regardless of El
    MULTIPLE_SAT_FIELD_LAT,     // Latitude
    MULTIPLE_SAT_FIELD_LON,     // Longitude
    MULTIPLE_SAT_FIELD_SSP,     // Sub Satellite Point grid square
    MULTIPLE_SAT_FIELD_FOOTPRINT,   // Footprint
    MULTIPLE_SAT_FIELD_ALT,     // Altitude
    MULTIPLE_SAT_FIELD_VEL,     // Velocity
    MULTIPLE_SAT_FIELD_DOPPLER, // Doppler shift at 100 Mhz
    MULTIPLE_SAT_FIELD_LOSS,    // Path loss at 100 Mhz
    MULTIPLE_SAT_FIELD_DELAY,   // Signal delay
    MULTIPLE_SAT_FIELD_MA,      // Mean Anomaly
    MULTIPLE_SAT_FIELD_PHASE,   // Phase
    MULTIPLE_SAT_FIELD_ORBIT,   // Orbit number
    MULTIPLE_SAT_FIELD_VISIBILITY,  // Visibility
    MULTIPLE_SAT_FIELD_SKR_DOWN,    // Secret Key Rate downlink
    MULTIPLE_SAT_FIELD_SKR_UP,  // Secret Key Rate uplink
    MULTIPLE_SAT_FIELD_SKR_NEAREST, // Secret Key Rate (inter-sat) to nearest sat
    MULTIPLE_SAT_FIELD_NUMBER
} multiple_sat_field_t;

/* Fieldnum Flags */
typedef enum {
    MULTIPLE_SAT_FLAG_AZ = 1 << MULTIPLE_SAT_FIELD_AZ,
    MULTIPLE_SAT_FLAG_EL = 1 << MULTIPLE_SAT_FIELD_EL,
    MULTIPLE_SAT_FLAG_DIR = 1 << MULTIPLE_SAT_FIELD_DIR,
    MULTIPLE_SAT_FLAG_RA = 1 << MULTIPLE_SAT_FIELD_RA,
    MULTIPLE_SAT_FLAG_DEC = 1 << MULTIPLE_SAT_FIELD_DEC,
    MULTIPLE_SAT_FLAG_RANGE = 1 << MULTIPLE_SAT_FIELD_RANGE,
    MULTIPLE_SAT_FLAG_RANGE_RATE = 1 << MULTIPLE_SAT_FIELD_RANGE_RATE,
    MULTIPLE_SAT_FLAG_NEXT_EVENT = 1 << MULTIPLE_SAT_FIELD_NEXT_EVENT,
    MULTIPLE_SAT_FLAG_AOS = 1 << MULTIPLE_SAT_FIELD_AOS,
    MULTIPLE_SAT_FLAG_LOS = 1 << MULTIPLE_SAT_FIELD_LOS,
    MULTIPLE_SAT_FLAG_LAT = 1 << MULTIPLE_SAT_FIELD_LAT,
    MULTIPLE_SAT_FLAG_LON = 1 << MULTIPLE_SAT_FIELD_LON,
    MULTIPLE_SAT_FLAG_SSP = 1 << MULTIPLE_SAT_FIELD_SSP,
    MULTIPLE_SAT_FLAG_FOOTPRINT = 1 << MULTIPLE_SAT_FIELD_FOOTPRINT,
    MULTIPLE_SAT_FLAG_ALT = 1 << MULTIPLE_SAT_FIELD_ALT,
    MULTIPLE_SAT_FLAG_VEL = 1 << MULTIPLE_SAT_FIELD_VEL,
    MULTIPLE_SAT_FLAG_DOPPLER = 1 << MULTIPLE_SAT_FIELD_DOPPLER,
    MULTIPLE_SAT_FLAG_LOSS = 1 << MULTIPLE_SAT_FIELD_LOSS,
    MULTIPLE_SAT_FLAG_DELAY = 1 << MULTIPLE_SAT_FIELD_DELAY,
    MULTIPLE_SAT_FLAG_MA = 1 << MULTIPLE_SAT_FIELD_MA,
    MULTIPLE_SAT_FLAG_PHASE = 1 << MULTIPLE_SAT_FIELD_PHASE,
    MULTIPLE_SAT_FLAG_ORBIT = 1 << MULTIPLE_SAT_FIELD_ORBIT,
    MULTIPLE_SAT_FLAG_VISIBILITY = 1 << MULTIPLE_SAT_FIELD_VISIBILITY,
    MULTIPLE_SAT_FLAG_SKR_DOWN = 1 << MULTIPLE_SAT_FIELD_SKR_DOWN,
    MULTIPLE_SAT_FLAG_SKR_UP = 1 << MULTIPLE_SAT_FIELD_SKR_UP,
    MULTIPLE_SAT_FLAG_SKR_NEAREST = 1 << MULTIPLE_SAT_FIELD_SKR_NEAREST
} multiple_sat_flag_t;

// Helper struct to define a satellite panel
typedef struct {
    GtkWidget *header;
    GtkWidget *table;
    GtkWidget *popup_button;
    GtkWidget *labels[MULTIPLE_SAT_FIELD_NUMBER];
    gint selected_catnum;
} SatPanel;

// Helper struct for popup callback (when clicked signal is triggered)
typedef struct {
    GtkWidget *parent;
    guint index;
} PopupCallbackData;

#define GTK_TYPE_MULTIPLE_SAT           (gtk_multiple_sat_get_type())
#define GTK_MULTIPLE_SAT(obj)           G_TYPE_CHECK_INSTANCE_CAST(obj,\
                                           gtk_multiple_sat_get_type (),\
                                           GtkMultipleSat)
#define GTK_MULTIPLE_SAT_CLASS(klass)   G_TYPE_CHECK_CLASS_CAST (klass,\
                                           gtk_multiple_sat_get_type (),\
                                           GtkMultipleSatClass)
#define IS_GTK_MULTIPLE_SAT(obj)        G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_multiple_sat_get_type ())

typedef struct _gtk_multiple_sat GtkMultipleSat;
typedef struct _GtkMultipleSatClass GtkMultipleSatClass;

struct _gtk_multiple_sat {
    GtkBox          vbox;

    GtkWidget       *header[NUMBER_OF_SATS];   // Header label for first satellite

    GtkWidget       *labels[NUMBER_OF_SATS][MULTIPLE_SAT_FIELD_NUMBER]; // GtkLabels displaying the data

    GtkWidget       *swin;      // Scrolled window
    GtkWidget       *panels[NUMBER_OF_SATS];    // SatPanels list
    GtkWidget       *table[NUMBER_OF_SATS];     // Table
    
    GtkWidget       *popup_button[NUMBER_OF_SATS];  // Popup button

    GKeyFile        *cfgdata;       // Configuration data
    GSList          *sats;      // Satellites
    qth_t           *qth;       // Pointer to current location

    guint32         flags;      // Flags indicating which columns are visible
    guint           refresh;    // Refresh rate
    guint           counter;    // Cycle counter
    guint           selected[NUMBER_OF_SATS];   // Index of selected sat

    gdouble         tstamp;     // Time stamp of calculations - update by GtkSatModule

    void            (*update) (GtkWidget * widget);     // update function
};

struct _GtkMultipleSatClass {
    GtkBoxClass     parent_class;
};

GType           gtk_multiple_sat_get_type(void);
GtkWidget       *gtk_multiple_sat_new(GKeyFile * cfgdata,
                                      GHashTable * sats,
                                      qth_t * qth, guint fields);
void            gtk_multiple_sat_update(GtkWidget * widget, guint index);
void            gtk_multiple_sat_reconf(GtkWidget * widget,
                                        GKeyFile * newcfg,
                                        GHashTable * sats,
                                        qth_t * qth, gboolean local);

void            gtk_multiple_sat_reload_sats(GtkWidget * multiple_sat,
                                             GHashTable * sats);
void            gtk_multiple_sat_select_sat(GtkWidget * multiple_sat, gint catnum,
                                            guint index);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif