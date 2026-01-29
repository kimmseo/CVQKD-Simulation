#include "transfer-heap.h"
#include <glib/gi18n.h>

/**
 * Code from: https://rosettacode.org/wiki/Priority_queue#C
 * Create heap by 
 * heap_tfr *h = (heap_tfr *)calloc(1, sizeof(heap_tfr))
 */

void push_tfr(heap_tfr *h, gdouble time, gint *cat_nr) {
    
    if (h->len + 1 >= h->size) {
        //if h->size is 0, set it 4. Else double it.
        h->size = h->size ? h->size * 2 : 4;
        h->nodes = (node_tfr *)realloc(h->nodes, h->size * sizeof(node_tfr));
    }

    int i = h->len + 1;
    int j = i / 2;

    while (i > 1 && h->nodes[j].time > time) {
        h->nodes[i] = h->nodes[j];
        i = j;
        j = j / 2;
    }

    h->nodes[i].time = time;
    h->nodes[i].cat_nr = cat_nr;
    h->len++;
}

void pop_tfr (heap_tfr *h, node_tfr *min) {
    int i, j, k;
    if (!h->len) {
        min = NULL;
    }

    memcpy(min, &h->nodes[1], sizeof(node_tfr));
    
    h->nodes[1] = h->nodes[h->len];
    
    h->len--;
    
    i = 1;
    while (i!=h->len+1) {
        k = h->len+1;
        j = 2 * i;
        if (j <= h->len && h->nodes[j].time < h->nodes[k].time) {
            k = j;
        }
        if (j + 1 <= h->len && h->nodes[j + 1].time < h->nodes[k].time) {
            k = j + 1;
        }
        h->nodes[i] = h->nodes[k];
        i = k;
    }
}