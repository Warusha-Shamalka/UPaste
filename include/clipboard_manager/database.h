#ifndef CLIPBOARD_MANAGER_DATABASE_H
#define CLIPBOARD_MANAGER_DATABASE_H

#include <glib.h>
#include <sqlite3.h>

gboolean database_open(sqlite3 **database, GError **error);
gboolean database_create_schema(sqlite3 *database, GError **error);
gboolean database_insert_clipboard_text(sqlite3 *database, const gchar *text, GError **error);

#endif
