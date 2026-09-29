#ifndef CLIPBOARD_MANAGER_CLIPBOARD_MONITOR_H
#define CLIPBOARD_MANAGER_CLIPBOARD_MONITOR_H

#include <glib.h>
#include <sqlite3.h>

typedef void (*ClipboardMonitorChangedFunc)(gpointer user_data);

typedef struct
{
    sqlite3 *database;
    gchar *last_text;
    guint source_id;
    gboolean request_pending;
    ClipboardMonitorChangedFunc changed_callback;
    gpointer changed_data;
} ClipboardMonitor;

void clipboard_monitor_start(ClipboardMonitor *monitor, sqlite3 *database);
void clipboard_monitor_stop(ClipboardMonitor *monitor);
void clipboard_monitor_set_changed_callback(ClipboardMonitor *monitor,
                                           ClipboardMonitorChangedFunc callback,
                                           gpointer user_data);
gboolean clipboard_monitor_is_active(const ClipboardMonitor *monitor);
void clipboard_monitor_set_active(ClipboardMonitor *monitor, gboolean active);
void clipboard_monitor_note_text(ClipboardMonitor *monitor, const gchar *text);

#endif
