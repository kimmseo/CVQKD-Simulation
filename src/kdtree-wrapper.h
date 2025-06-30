#ifndef __KDTREE_WRAPPER_H__
#define __KDTREE_WRAPPER_H__

#include <glib.h>
#include "kdtree.h"

G_BEGIN_DECLS

// Opaque kd-tree object
typedef struct _Kdtree Kdtree;

struct _Kdtree {
    struct kdtree *tree;
    gint dimensions;
};

// Create a new kd-tree with `k` dimensions
Kdtree *kdtree_new(gint k);

// Free a kd-tree and its contents
void kdtree_free(Kdtree *tree);

// Set a destructor function for user data
void kdtree_set_data_destroy_func(Kdtree *tree, GDestroyNotify func);

// Insert point of arbitrary dimensions
gboolean kdtree_insert(Kdtree *tree, const gdouble *coords, gpointer data);

// Convenience: insert 1D point
gboolean kdtree_insert1(Kdtree *tree, gdouble x, gpointer data);

// Convenience: insert 3D point
gboolean kdtree_insert3(Kdtree *tree, gdouble x, gdouble y, gdouble z, gpointer data);

// Find nearest neighbor to a point (generic)
gpointer kdtree_nearest(Kdtree *tree, const gdouble *coords, gdouble *out_coords);

// Find nearest neighbor to a 1D point
gpointer kdtree_nearest1(Kdtree *tree, gdouble x, gdouble *out_x);

// Find nearest neighbor to a 3D point
gpointer kdtree_nearest3(Kdtree *tree, gdouble x, gdouble y, gdouble z,
                         gdouble *out_x, gdouble *out_y, gdouble *out_z);

G_END_DECLS

#endif // KDTREE_WRAPPER_H
