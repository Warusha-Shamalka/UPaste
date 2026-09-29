#ifndef CLIPBOARD_MANAGER_APP_H
#define CLIPBOARD_MANAGER_APP_H

#include <glib.h>

typedef struct App App;

App *app_create(GError **error);
void app_destroy(App *app);

#endif
