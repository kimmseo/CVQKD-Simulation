#ifndef MOD_CFG_H
#define MOD_CFG_H 1

#include <glib.h>

typedef enum {
    MOD_CFG_OK = 0,
    MOD_CFG_CANCEL,
    MOD_CFG_ERROR
} mod_cfg_status_t;

typedef struct {
    GtkWidget *parent;
    GtkWidget *q_list;
} mod_cfg_qlist_cb_data;

#define MOD_CFG_TEMP_QTHS_FOLDER "TempModule546048"

gchar          *mod_cfg_new(void);
mod_cfg_status_t mod_cfg_edit(gchar * modname, GKeyFile * cfgdata,
                              GtkWidget * toplevel);
mod_cfg_status_t mod_cfg_save(gchar * modname, GKeyFile * cfgdata);
mod_cfg_status_t mod_cfg_delete(gchar * modname, gboolean needcfm);

void rm_non_empty_dir(const gchar *path);

#endif
