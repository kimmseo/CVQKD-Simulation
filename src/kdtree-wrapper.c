#include "kdtree-wrapper.h"
#include "kdtree.h"

struct _Kdtree {
    struct kdtree *tree;
    gint dimensions;
};

Kdtree *kdtree_new(gint k) {
    g_return_val_if_fail(k > 0, NULL);

    Kdtree *wrapper = g_new0(Kdtree, 1);
    wrapper->tree = kd_create(k);
    wrapper->dimensions = k;
    return wrapper;
}

void kdtree_set_data_destroy_func(Kdtree *tree, GDestroyNotify func) {
    g_return_if_fail(tree != NULL);
    kd_data_destructor(tree->tree, func);
}

void kdtree_free(Kdtree *tree) {
    if (tree) {
        kd_free(tree->tree);
        g_free(tree);
    }
}

gboolean kdtree_insert(Kdtree *tree, const gdouble *coords, gpointer data) {
    g_return_val_if_fail(tree != NULL && coords != NULL, FALSE);
    return kd_insert(tree->tree, coords, data) == 0;
}

gboolean kdtree_insert1(Kdtree *tree, gdouble x, gpointer data) {
    gdouble pos[1] = { x };
    return kdtree_insert(tree, pos, data);
}

gboolean kdtree_insert3(Kdtree *tree, gdouble x, gdouble y, gdouble z, gpointer data) {
    g_return_val_if_fail(tree != NULL && tree->dimensions == 3, FALSE);
    return kd_insert3(tree->tree, x, y, z, data) == 0;
}

gpointer kdtree_nearest(Kdtree *tree, const gdouble *coords, gdouble *out_coords) {
    g_return_val_if_fail(tree != NULL && coords != NULL, NULL);

    struct kdres *res = kd_nearest(tree->tree, coords);
    if (!res || kd_res_end(res)) {
        if (res) kd_res_free(res);
        return NULL;
    }

    gpointer data = kd_res_item(res, out_coords);
    kd_res_free(res);
    return data;
}

gpointer kdtree_nearest1(Kdtree *tree, gdouble x, gdouble *out_x) {
    gdouble input[1] = { x };
    gdouble result[1];
    gpointer data = kdtree_nearest(tree, input, result);
    if (out_x && data) *out_x = result[0];
    return data;
}

gpointer kdtree_nearest3(Kdtree *tree, gdouble x, gdouble y, gdouble z,
                         gdouble *out_x, gdouble *out_y, gdouble *out_z) {
    g_return_val_if_fail(tree != NULL && tree->dimensions == 3, NULL);

    struct kdres *res = kd_nearest3(tree->tree, x, y, z);
    if (!res || kd_res_end(res)) {
        if (res) kd_res_free(res);
        return NULL;
    }

    gdouble pos[3];
    gpointer data = kd_res_item(res, pos);
    if (data) {
        if (out_x) *out_x = pos[0];
        if (out_y) *out_y = pos[1];
        if (out_z) *out_z = pos[2];
    }

    kd_res_free(res);
    return data;
}
