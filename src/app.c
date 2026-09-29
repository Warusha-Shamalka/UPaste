#include <sqlite3.h>
#include <stdlib.h>

#include "clipboard_manager/app.h"
#include "clipboard_manager/clipboard_monitor.h"
#include "clipboard_manager/database.h"
#include "clipboard_manager/window.h"

struct App
{
    sqlite3 *database;
    ClipboardMonitor clipboard_monitor;
};

App *app_create(GError **error)
{
    App *app = g_new0(App, 1);

    if (!database_open(&app->database, error))
    {
        g_free(app);
        return NULL;
    }

    if (!database_create_schema(app->database, error))
    {
        sqlite3_close(app->database);
        g_free(app);
        return NULL;
    }

    window_show_main();
    clipboard_monitor_start(&app->clipboard_monitor, app->database);

    return app;
}

void app_destroy(App *app)
{
    if (app == NULL)
    {
        return;
    }

    clipboard_monitor_stop(&app->clipboard_monitor);
    sqlite3_close(app->database);
    g_free(app);
}
