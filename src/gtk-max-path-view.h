#ifndef __GTK_MAX_PATH_VIEW_H__
#define __GTK_MAX_PATH_VIEW_H__ 1

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "qth-data.h"
#include "max-capacity-path/path-util.h"
#include "sat-kdtree-utils.h"


/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define MAX_SATS_PER_ROW 1
#define UNUSED(x) (void)(x)


/* Symbolic references to columns */
typedef enum {
    MAX_PATH_VIEW_FIELD_DIR = 0,     // Direction, satellite on its way up or down
    MAX_PATH_VIEW_FIELD_LAT,     // Latitude
    MAX_PATH_VIEW_FIELD_LON,     // Longitude
    MAX_PATH_VIEW_FIELD_FOOTPRINT,   // Footprint
    MAX_PATH_VIEW_FIELD_ALT,     // Altitude
    MAX_PATH_VIEW_FIELD_VEL,     // Velocity
    MAX_PATH_VIEW_FIELD_SKR_DOWN,    // Secret Key Rate downlink
    MAX_PATH_VIEW_FIELD_SKR_UP,  // Secret Key Rate uplink
    MAX_PATH_VIEW_FIELD_SKR_NEAREST, // Secret Key Rate (inter-sat) to nearest sat
    MAX_PATH_VIEW_FIELD_NUMBER
} max_path_view_field_t;

/* Fieldnum Flags */
typedef enum {
    MAX_PATH_VIEW_FLAG_DIR = 1 << MAX_PATH_VIEW_FIELD_DIR,
    MAX_PATH_VIEW_FLAG_LAT = 1 << MAX_PATH_VIEW_FIELD_LAT,
    MAX_PATH_VIEW_FLAG_LON = 1 << MAX_PATH_VIEW_FIELD_LON,
    MAX_PATH_VIEW_FLAG_FOOTPRINT = 1 << MAX_PATH_VIEW_FIELD_FOOTPRINT,
    MAX_PATH_VIEW_FLAG_ALT = 1 << MAX_PATH_VIEW_FIELD_ALT,
    MAX_PATH_VIEW_FLAG_VEL = 1 << MAX_PATH_VIEW_FIELD_VEL,
    MAX_PATH_VIEW_FLAG_SKR_DOWN = 1 << MAX_PATH_VIEW_FIELD_SKR_DOWN,
    MAX_PATH_VIEW_FLAG_SKR_UP = 1 << MAX_PATH_VIEW_FIELD_SKR_UP,
    MAX_PATH_VIEW_FLAG_SKR_NEAREST = 1 << MAX_PATH_VIEW_FIELD_SKR_NEAREST
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
#define IS_GTK_COMBO_BOX_TEXT(obj)       G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_combo_box_text_get_type ())
#define IS_GTK_SPIN_BUTTON(obj)          G_TYPE_CHECK_INSTANCE_TYPE(obj, gtk_spin_button_get_type ())

typedef struct _gtk_max_path_view GtkMaxPathView;
typedef struct _GtkMaxPathViewClass GtkMaxPathViewClass;

struct _gtk_max_path_view {
    GtkBox          vbox;

    guint            dyn_num_sat;    //number of satellites currently loaded

    max_path_t      *max_capacity_path;
    GList           *path_colors;               //GList of GdkRGBA

    GtkWidget       *search_controls;
    GtkWidget       *display_path;

    gint            num_selected_fields;
    GArray          *labels;

    GKeyFile        *cfgdata;   // Configuration data
    GSList          *sats;      // Satellites
    GSList          *qths;
    qth_t           *qth;       // Pointer to current location
    qth_t           *qth2;      //TEMP: replace this and qth with qth_t GSList

    guint32         flags;      // Flags indicating which columns are visible
    guint           num_flags_selected;
    guint           refresh;    // Refresh rate
    guint           counter;    // Cycle counter
    
    //gint            selected[NUMBER_OF_SATS];   // Index of selected sat
    GArray          *selected;
    Kdtree          *kdtree;    // k-d tree to hold satellites

    gdouble         tstamp;     // Time stamp of calculations - update by GtkSatModule

    void            (*update) (GtkWidget * widget, guint index);     /*!< update function */
};

struct _GtkMaxPathViewClass {
    GtkBoxClass     parent_class;
};

GType           gtk_max_path_view_get_type(void);
GtkWidget       *gtk_max_path_view_new(GKeyFile * cfgdata, GHashTable * sats,
                                GSList *qths, qth_t *qth, guint32 fields);
void            gtk_max_path_view_update(GtkWidget * widget, guint index);
void            gtk_max_path_view_reconf(GtkWidget * widget,
                                        GKeyFile * newcfg,
                                        GHashTable * sats,
                                        qth_t * qth, gboolean local);

void            gtk_max_path_view_reload_sats(GtkWidget * max_path_view,
                                             GHashTable * sats);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif