#ifndef __GTK_MAX_PATH_VIEW_H__
#define __GTK_MAX_PATH_VIEW_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "qth-data.h"


/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define MAX_SATS_PER_ROW 1


/* Symbolic references to columns */
typedef enum {
    MAX_PATH_VIEW_FIELD_AZ = 0,  // Azimuth
    MAX_PATH_VIEW_FIELD_EL,      // Elevation
    MAX_PATH_VIEW_FIELD_DIR,     // Direction, satellite on its way up or down
    MAX_PATH_VIEW_FIELD_RA,      // Right Ascension
    MAX_PATH_VIEW_FIELD_DEC,     // Declination
    MAX_PATH_VIEW_FIELD_LAT,     // Latitude
    MAX_PATH_VIEW_FIELD_LON,     // Longitude
    MAX_PATH_VIEW_FIELD_SSP,     // Sub Satellite Point grid square
    MAX_PATH_VIEW_FIELD_FOOTPRINT,   // Footprint
    MAX_PATH_VIEW_FIELD_ALT,     // Altitude
    MAX_PATH_VIEW_FIELD_VEL,     // Velocity
    MAX_PATH_VIEW_FIELD_DOPPLER, // Doppler shift at 100 Mhz
    MAX_PATH_VIEW_FIELD_LOSS,    // Path loss at 100 Mhz
    MAX_PATH_VIEW_FIELD_MA,      // Mean Anomaly
    MAX_PATH_VIEW_FIELD_PHASE,   // Phase
    MAX_PATH_VIEW_FIELD_ORBIT,   // Orbit number
    MAX_PATH_VIEW_FIELD_NUMBER
} max_path_view_field_t;

/* Fieldnum Flags */
typedef enum {
    MAX_PATH_VIEW_FLAG_AZ = 1 << MAX_PATH_VIEW_FIELD_AZ,
    MAX_PATH_VIEW_FLAG_EL = 1 << MAX_PATH_VIEW_FIELD_EL,
    MAX_PATH_VIEW_FLAG_DIR = 1 << MAX_PATH_VIEW_FIELD_DIR,
    MAX_PATH_VIEW_FLAG_RA = 1 << MAX_PATH_VIEW_FIELD_RA,
    MAX_PATH_VIEW_FLAG_DEC = 1 << MAX_PATH_VIEW_FIELD_DEC,
    MAX_PATH_VIEW_FLAG_LAT = 1 << MAX_PATH_VIEW_FIELD_LAT,
    MAX_PATH_VIEW_FLAG_LON = 1 << MAX_PATH_VIEW_FIELD_LON,
    MAX_PATH_VIEW_FLAG_SSP = 1 << MAX_PATH_VIEW_FIELD_SSP,
    MAX_PATH_VIEW_FLAG_FOOTPRINT = 1 << MAX_PATH_VIEW_FIELD_FOOTPRINT,
    MAX_PATH_VIEW_FLAG_ALT = 1 << MAX_PATH_VIEW_FIELD_ALT,
    MAX_PATH_VIEW_FLAG_VEL = 1 << MAX_PATH_VIEW_FIELD_VEL,
    MAX_PATH_VIEW_FLAG_DOPPLER = 1 << MAX_PATH_VIEW_FIELD_DOPPLER,
    MAX_PATH_VIEW_FLAG_LOSS = 1 << MAX_PATH_VIEW_FIELD_LOSS,
    MAX_PATH_VIEW_FLAG_MA = 1 << MAX_PATH_VIEW_FIELD_MA,
    MAX_PATH_VIEW_FLAG_PHASE = 1 << MAX_PATH_VIEW_FIELD_PHASE,
    MAX_PATH_VIEW_FLAG_ORBIT = 1 << MAX_PATH_VIEW_FIELD_ORBIT,
} max_path_view_flag_t;

// Helper struct to define a satellite panel
typedef struct {
    GtkWidget *header;
    GtkWidget *table;
    GtkWidget *popup_button;
    GtkWidget *labels[MAX_PATH_VIEW_FIELD_NUMBER];
    gint selected_catnum;
} MaxSatPanel;

// Helper structs for callback
typedef struct {
    GtkWidget *parent;
    guint index;
    gint selectedIndex;
} MaxPopupCallbackData;

typedef struct {
    GtkWidget *parent;
    guint index;
} MaxSelectSatCallbackData;


#define GTK_TYPE_MAX_PATH_VIEW           (gtk_max_path_view_get_type())
#define GTK_MAX_PATH_VIEW(obj)           G_TYPE_CHECK_INSTANCE_CAST(obj,\
                                           gtk_max_path_view_get_type (),\
                                           GtkMaxPathView)
#define GTK_MAX_PATH_VIEW_CLASS(klass)   G_TYPE_CHECK_CLASS_CAST (klass,\
                                           gtk_max_path_view_get_type (),\
                                           GtkMaxPathViewClass)
#define IS_GTK_MAX_PATH_VIEW(obj)        G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_max_path_view_get_type ())

typedef struct _gtk_max_path_view GtkMaxPathView;
typedef struct _GtkMaxPathViewClass GtkMaxPathViewClass;

struct _gtk_max_path_view {
    GtkBox          vbox;

    guint            dyn_num_sat;    //number of satellites currently loaded

    //GtkWidget       *labels[NUMBER_OF_SATS][MULTIPLE_SAT_FIELD_NUMBER]; // GtkLabels displaying the data
    GArray          *labels;

    GtkWidget       *textbox;   //temp: test tile or something

    GHashTable      *satsTable;         //ToDo: maybe can fix get-max-capacity-path to use lists instead
    GList           *max_capacity_path;

    GtkWidget       *button;    //temp

    GtkWidget       *swin;      // Scrolled window

    //SatPanel        *panels[NUMBER_OF_SATS];    // SatPanels list (array of SatPanel Pointers)
    GArray          *panels;

    //GtkWidget       *table[NUMBER_OF_SATS];     // Table
    
    //GtkWidget       *popup_button[NUMBER_OF_SATS];  // Popup button

    GKeyFile        *cfgdata;       // Configuration data
    GSList          *sats;      // Satellites
    qth_t           *qth;       // Pointer to current location

    guint32         flags;      // Flags indicating which columns are visible
    guint           refresh;    // Refresh rate
    guint           counter;    // Cycle counter
    
    //gint            selected[NUMBER_OF_SATS];   // Index of selected sat
    GArray          *selected;

    gdouble         tstamp;     // Time stamp of calculations - update by GtkSatModule

    void            (*update) (GtkWidget * widget, guint index);     /*!< update function */
};

struct _GtkMaxPathViewClass {
    GtkBoxClass     parent_class;
};

GType           gtk_max_path_view_get_type(void);
GtkWidget       *gtk_max_path_view_new(GKeyFile * cfgdata,
                                      GHashTable * sats,
                                      qth_t * qth, guint fields);
void            gtk_max_path_view_update(GtkWidget * widget, guint index);
void            gtk_max_path_view_reconf(GtkWidget * widget,
                                        GKeyFile * newcfg,
                                        GHashTable * sats,
                                        qth_t * qth, gboolean local);

void            gtk_max_path_view_reload_sats(GtkWidget * max_path_view,
                                             GHashTable * sats);
void            gtk_max_path_view_select_sat(GtkWidget * max_path_view, gint catnum,
                                            guint index);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif