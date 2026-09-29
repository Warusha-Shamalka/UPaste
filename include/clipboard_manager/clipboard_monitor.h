#ifndef CLIPBOARD_MANAGER_CLIPBOARD_MONITOR_H
#define CLIPBOARD_MANAGER_CLIPBOARD_MONITOR_H

#include <glib.h>
#include <sqlite3.h>

typedef struct
{
    sqlite3 *database;
    gchar *last_text;
    guint source_id;
} ClipboardMonitor;

void clipboard_monitor_start(ClipboardMonitor *monitor, sqlite3 *database);
void clipboard_monitor_stop(ClipboardMonitor *monitor);

#endif
