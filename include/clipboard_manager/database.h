#ifndef CLIPBOARD_MANAGER_DATABASE_H
#define CLIPBOARD_MANAGER_DATABASE_H

#include <glib.h>
#include <sqlite3.h>

typedef struct
{
	sqlite3_int64 id;
	gchar *content;
} ClipboardEntry;

gboolean database_open(sqlite3 **database, GError **error);
gboolean database_create_schema(sqlite3 *database, GError **error);
gboolean database_insert_clipboard_text(sqlite3 *database, const gchar *text, GError **error);
GPtrArray *database_get_history(sqlite3 *database, GError **error);
gboolean database_delete_entry(sqlite3 *database, sqlite3_int64 id, GError **error);
gboolean database_clear_history(sqlite3 *database, GError **error);

#endif