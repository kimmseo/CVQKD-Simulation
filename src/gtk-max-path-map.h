#ifndef __GTK_MAX_PATH_MAP_H__
#define __GTK_MAX_PATH_MAP_H__ 1

#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <goocanvas.h>
#include <gtk/gtk.h>

#include "gtk-sat-data.h"
#include "calc-dist-two-sat.h"
#include "sat-graph.h"
#include "qth-data.h"

/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

#define MAX_PATH_MAP_RANGE_CIRCLE_POINTS    180      /*!< Number of points used to plot a satellite range half circle. */

#define GTK_MAX_PATH_MAP(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, gtk_max_path_map_get_type (), GtkMaxPathMap)
#define GTK_MAX_PATH_MAP_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, gtk_max_path_map_get_type (), GtkMaxPathMapClass)
#define GTK_IS_MAX_PATH_MAP(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_max_path_map_get_type ())
#define GTK_TYPE_MAX_PATH_MAP          (gtk_max_path_map_get_type ())
#define IS_GTK_MAX_PATH_MAP(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, gtk_max_path_map_get_type ())

typedef struct _GtkMaxPathMapClass GtkMaxPathMapClass;

/** Structure that define a sub-satellite point. */
typedef struct {
    double          lat;        /*!< Latitude in decimal degrees North. */
    double          lon;        /*!< Longitude in decimal degrees West. */
} mpm_ssp_t;

/** Data storage for ground tracks */
typedef struct {
    GSList         *latlon;     /*!< List of ssp_t */
    GSList         *lines;      /*!< List of GooCanvasPolyLine */
} mpm_ground_track_t;

/**
 * Satellite object.
 *
 * This data structure represents a satellite object on the map. It consists of a
 * small square representing the position, a label showinf the satellite name, and
 * the range circle. The range circle can have one or two parts, depending on
 * whether it is split or not. The oldrcnum and newrcnum fields are used for
 * keeping track of whether the range circle has one or two parts.
 *
 */
typedef struct {
    /* flags */
    gboolean        selected;   /*!< Is satellite selected? */
    gboolean        showtrack;  /*!< Show ground track */
    gboolean        showcov;    /*!< Show coverage area. */
    gboolean        istarget;   /*!< is this object the target */

    /* graphical elements */
    GooCanvasItemModel *marker; /*!< A small rectangle showing sat pos. */
    GooCanvasItemModel *shadowm;        /*!< Shadow under satellite marker. */
    GooCanvasItemModel *label;  /*!< Satellite name. */
    GooCanvasItemModel *shadowl;        /*!< Shadow under satellite name */
    GooCanvasItemModel *range1; /*!< First part of the range circle. */
    GooCanvasItemModel *range2; /*!< Second part of the range circle. */

    /* book keeping */
    guint           oldrcnum;   /*!< Number of RC parts in prev. cycle. */
    guint           newrcnum;   /*!< Number of RC parts in this cycle. */
    gint            catnum;     /*!< Catalogue number of satellite. */

    mpm_ground_track_t  track_data; /*!< Ground track data. */
    long            track_orbit;        /*!< Orbit when the ground track has been updated. */

} max_path_map_obj_t;

#define MAX_PATH_MAP_OBJ(obj) ((max_path_map_obj_t *)obj)

/** The satellite map data structure. */
typedef struct {
    GtkBox          vbox;

    GtkWidget      *canvas;     /*!< The canvas widget. */

    GooCanvasItemModel *map;    /*!< The canvas map item. */

    gdouble         left_side_lon;      /*!< Left-most longitude (used when center is not 0 lon). */

    GooCanvasItemModel *qthmark;        /*!< QTH marker, e.g. small rectangle. */
    GooCanvasItemModel *qth2mark;       /*!< Second QTH Marker */
    GooCanvasItemModel *qthlabel;       /*!< Label showing the QTH name. */
    GooCanvasItemModel *qth2label;      /*!< Label showing second QTH name */

    GooCanvasItemModel *locnam; /*!< Location name. */
    GooCanvasItemModel *locnam2; /*!< Second location name */
    GooCanvasItemModel *curs;   /*!< Cursor tracking text. */
    GooCanvasItemModel *next;   /*!< Next event text. */
    GooCanvasItemModel *sel;    /*!< Text showing info about the selected satellite. */

    GooCanvasItemModel *gridv[11];      /*!< Vertical grid lines, 30 deg resolution. */
    GooCanvasItemModel *gridvlab[11];   /*!< Vertical grid  labels. */
    GooCanvasItemModel *gridh[5];       /*!< Horizontal grid lines, 30 deg resolution. */
    GooCanvasItemModel *gridhlab[5];    /*!< Horizontal grid labels. */

    GooCanvasItemModel *capacity_path_line;     /*!< Stroke properties for drawing line */
    GooCanvasPoints *capacity_points;           /*!< X,Y coordinates of points along line */
    GList *capacity_path_nodes;                 /*!< X,Y coordinates of points along line */

    GooCanvasItemModel *terminator;     /*!< Outline of sun shadow on Earth. */

    gdouble         terminator_last_tstamp;     /*!< Timestamp of the last terminator drawn. Used to prevent redrawing the terminator too often. */

    gdouble         naos;       /*!< Next event time. */
    gint            ncat;       /*!< Next event catnum. */

    gdouble         tstamp;     /*!< Time stamp for calculations; set by GtkSatModule */

    GKeyFile       *cfgdata;    /*!< Module configuration data. */
    GHashTable     *sats;       /*!< Pointer to satellites (owned by parent GtkSatModule). */
    qth_t          *qth;        /*!< Pointer to current location. */
    qth_t          *qth2;       /*!< Pointer to the second QTH. */

    GHashTable     *obj;        /*!< Canvas items representing each satellite. */
    GHashTable     *showtracks; /*!< A hash of satellites to show tracks for. */
    GHashTable     *hidecovs;   /*!< A hash of satellites to hide coverage for. */

    guint           x0;         /*!< X0 of the canvas map. */
    guint           y0;         /*!< Y0 of the canvas map. */
    guint           width;      /*!< Map width. */
    guint           height;     /*!< Map height. */

    guint           refresh;    /*!< Refresh rate. */
    guint           counter;    /*!< Cycle counter. */

    gboolean        show_terminator;    // show solar terminator
    gboolean        qthinfo;    /*!< Show the QTH info. */
    gboolean        qth2info;   /*<! Show the second QTH info. */
    gboolean        eventinfo;  /*!< Show info about the next event. */
    gboolean        cursinfo;   /*!< Track the mouse cursor. */
    gboolean        showgrid;   /*!< Show grid on map. */
    gboolean        keepratio;  /*!< Keep map aspect ratio. */
    gboolean        resize;     /*!< Flag indicating that the map has been resized. */

    gchar          *infobgd;    /*!< Background color of info text. */

    GdkPixbuf      *origmap;    /*!< Original map kept here for high quality scaling. */

} GtkMaxPathMap;

struct _GtkMaxPathMapClass {
    GtkBoxClass     parent_class;
};

GType           gtk_max_path_map_get_type(void);
GtkWidget      *gtk_max_path_map_new(GKeyFile * cfgdata,
                                GHashTable * sats, qth_t * qth, qth_t * qth2);
void            gtk_max_path_map_update(GtkWidget * widget);
void            gtk_max_path_map_reconf(GtkWidget * widget, GKeyFile * cfgdat);
void            gtk_max_path_map_lonlat_to_xy(GtkMaxPathMap * m,
                                         gdouble lon, gdouble lat,
                                         gdouble * x, gdouble * y);

void            gtk_max_path_map_reload_sats(GtkWidget * satmap, GHashTable * sats);
void            gtk_max_path_map_select_sat(GtkWidget * satmap, gint catnum);

void            set_max_capacity_path(GtkMaxPathMap *map, GList *path);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
