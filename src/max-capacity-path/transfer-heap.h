#include <glib/gi18n.h>
#include "path-util.h"

typedef struct {
    gdouble time;
    gint *cat_nr;
    tdsp_node *node;
} node_tfr;

typedef struct {
    node_tfr *nodes;
    int size;
    int len;
} heap_tfr;

void pop_tfr(heap_tfr *h, node_tfr *min);

void push_tfr(heap_tfr *h, gdouble time, gint *cat_nr, tdsp_node *point);