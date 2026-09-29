#include <gtk/gtk.h>
#include <string.h>

#include "clipboard_manager/clipboard_monitor.h"
#include "clipboard_manager/database.h"

static void on_clipboard_text_received(GtkClipboard *clipboard, const gchar *text, gpointer user_data)
{
    ClipboardMonitor *monitor = user_data;
    GError *error = NULL;

    (void)clipboard;
    monitor->request_pending = FALSE;

    if (monitor->source_id == 0 || monitor->database == NULL || text == NULL)
    {
        return;
    }

    if (text[0] == '\0' || g_strcmp0(text, monitor->last_text) == 0)
    {
        return;
    }

    if (!database_insert_clipboard_text(monitor->database, text, &error))
    {
        g_warning("Could not save clipboard contents: %s",
                  error != NULL ? error->message : "unknown database error");
        g_clear_error(&error);
        return;
    }

    g_free(monitor->last_text);
    monitor->last_text = g_strdup(text);
    if (monitor->changed_callback != NULL)
    {
        monitor->changed_callback(monitor->changed_data);
    }
}

static gboolean request_clipboard_text(gpointer user_data)
{
    ClipboardMonitor *monitor = user_data;

    if (!monitor->request_pending)
    {
        monitor->request_pending = TRUE;
        gtk_clipboard_request_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
                                   on_clipboard_text_received,
                                   monitor);
    }

    return G_SOURCE_CONTINUE;
}

void clipboard_monitor_start(ClipboardMonitor *monitor, sqlite3 *database)
{
    g_return_if_fail(monitor != NULL);
    g_return_if_fail(database != NULL);

    monitor->database = database;
    if (monitor->source_id == 0)
    {
        monitor->source_id = g_timeout_add(500, request_clipboard_text, monitor);
    }
}

void clipboard_monitor_stop(ClipboardMonitor *monitor)
{
    g_return_if_fail(monitor != NULL);

    if (monitor->source_id != 0)
    {
        g_source_remove(monitor->source_id);
        monitor->source_id = 0;
    }

    monitor->request_pending = FALSE;
    g_clear_pointer(&monitor->last_text, g_free);
    monitor->database = NULL;
    monitor->changed_callback = NULL;
    monitor->changed_data = NULL;
}

void clipboard_monitor_set_changed_callback(ClipboardMonitor *monitor,
                                           ClipboardMonitorChangedFunc callback,
                                           gpointer user_data)
{
    g_return_if_fail(monitor != NULL);

    monitor->changed_callback = callback;
    monitor->changed_data = user_data;
}

gboolean clipboard_monitor_is_active(const ClipboardMonitor *monitor)
{
    g_return_val_if_fail(monitor != NULL, FALSE);
    return monitor->source_id != 0;
}

void clipboard_monitor_set_active(ClipboardMonitor *monitor, gboolean active)
{
    g_return_if_fail(monitor != NULL);

    if (active && monitor->source_id == 0 && monitor->database != NULL)
    {
        monitor->source_id = g_timeout_add(500, request_clipboard_text, monitor);
    }
    else if (!active && monitor->source_id != 0)
    {
        g_source_remove(monitor->source_id);
        monitor->source_id = 0;
    }
}

void clipboard_monitor_note_text(ClipboardMonitor *monitor, const gchar *text)
{
    g_return_if_fail(monitor != NULL);

    g_free(monitor->last_text);
    monitor->last_text = g_strdup(text);
}
