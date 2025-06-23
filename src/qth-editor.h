#ifndef QTH_EDITOR_H
#define QTH_EDITOR_H 1

#include <gtk/gtk.h>
#include "gtk-sat-data.h"
#include "qth-data.h"

GtkResponseType qth_editor_run(qth_t * qth, GtkWindow * parent);
GtkResponseType qth_editor_run_second(qth_t * qth2, GtkWindow * parent);

#endif
