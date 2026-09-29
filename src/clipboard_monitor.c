#include <gtk/gtk.h>
#include <string.h>

#include "clipboard_manager/clipboard_monitor.h"
#include "clipboard_manager/database.h"

static gboolean capture_clipboard(gpointer user_data)
{
    ClipboardMonitor *monitor = user_data;
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gchar *text = gtk_clipboard_wait_for_text(clipboard);
    GError *error = NULL;

    if (text == NULL)
    {
        return G_SOURCE_CONTINUE;
    }

    if (text[0] == '\0' || g_strcmp0(text, monitor->last_text) == 0)
    {
        g_free(text);
        return G_SOURCE_CONTINUE;
    }

    if (!database_insert_clipboard_text(monitor->database, text, &error))
    {
        g_warning("Could not save clipboard contents: %s", error->message);
        g_clear_error(&error);
        g_free(text);
        return G_SOURCE_CONTINUE;
    }

    g_free(monitor->last_text);
    monitor->last_text = text;
    return G_SOURCE_CONTINUE;
}

void clipboard_monitor_start(ClipboardMonitor *monitor, sqlite3 *database)
{
    g_return_if_fail(monitor != NULL);
    g_return_if_fail(database != NULL);

    monitor->database = database;
    monitor->source_id = g_timeout_add(500, capture_clipboard, monitor);
}

void clipboard_monitor_stop(ClipboardMonitor *monitor)
{
    g_return_if_fail(monitor != NULL);

    if (monitor->source_id != 0)
    {
        g_source_remove(monitor->source_id);
        monitor->source_id = 0;
    }

    g_clear_pointer(&monitor->last_text, g_free);
    monitor->database = NULL;
}
