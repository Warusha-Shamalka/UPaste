#include <errno.h>
#include <glib/gstdio.h>
#include <sqlite3.h>

#include "clipboard_manager/database.h"

G_DEFINE_QUARK(clipboard-manager-database-error, database_error)

static gboolean set_sqlite_error(GError **error, sqlite3 *database, gint result)
{
    g_set_error(error,
                database_error_quark(),
                result,
                "%s",
                database != NULL ? sqlite3_errmsg(database) : "SQLite operation failed");
    return FALSE;
}

gboolean database_open(sqlite3 **database, GError **error)
{
    gchar *directory;
    gchar *path;
    gint result;

    g_return_val_if_fail(database != NULL, FALSE);
    *database = NULL;

    directory = g_build_filename(g_get_user_data_dir(), "clipboard-manager", NULL);
    if (g_mkdir_with_parents(directory, 0700) != 0)
    {
        g_set_error(error,
                    G_FILE_ERROR,
                    g_file_error_from_errno(errno),
                    "Could not create application data directory '%s': %s",
                    directory,
                    g_strerror(errno));
        g_free(directory);
        return FALSE;
    }

    path = g_build_filename(directory, "history.sqlite3", NULL);
    result = sqlite3_open(path, database);
    g_free(path);
    g_free(directory);

    if (result != SQLITE_OK)
    {
        set_sqlite_error(error, *database, result);
        if (*database != NULL)
        {
            sqlite3_close(*database);
            *database = NULL;
        }
        return FALSE;
    }

    return TRUE;
}

gboolean database_create_schema(sqlite3 *database, GError **error)
{
    static const gchar sql[] =
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "content TEXT NOT NULL);"
        "CREATE TRIGGER IF NOT EXISTS history_limit AFTER INSERT ON history "
        "BEGIN "
        "DELETE FROM history WHERE id NOT IN "
        "(SELECT id FROM history ORDER BY id DESC LIMIT 500); "
        "END;";
    gchar *message = NULL;
    gint result;

    g_return_val_if_fail(database != NULL, FALSE);

    result = sqlite3_exec(database, sql, NULL, NULL, &message);
    if (result != SQLITE_OK)
    {
        g_set_error(error,
                    database_error_quark(),
                    result,
                    "Could not create clipboard history table: %s",
                    message != NULL ? message : sqlite3_errmsg(database));
        sqlite3_free(message);
        return FALSE;
    }

    return TRUE;
}

gboolean database_insert_clipboard_text(sqlite3 *database, const gchar *text, GError **error)
{
    static const gchar sql[] = "INSERT INTO history (content) VALUES (?);";
    sqlite3_stmt *statement = NULL;
    gint result;

    g_return_val_if_fail(database != NULL, FALSE);
    g_return_val_if_fail(text != NULL, FALSE);

    result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result != SQLITE_OK)
    {
        return set_sqlite_error(error, database, result);
    }

    result = sqlite3_bind_text(statement, 1, text, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK)
    {
        result = sqlite3_step(statement);
    }

    if (result != SQLITE_DONE)
    {
        set_sqlite_error(error, database, result);
        sqlite3_finalize(statement);
        return FALSE;
    }

    sqlite3_finalize(statement);
    return TRUE;
}

static void clipboard_entry_free(gpointer data)
{
    ClipboardEntry *entry = data;
    g_free(entry->content);
    g_free(entry);
}

GPtrArray *database_get_history(sqlite3 *database, GError **error)
{
    static const gchar sql[] = "SELECT id, content FROM history ORDER BY id DESC;";
    sqlite3_stmt *statement = NULL;
    GPtrArray *entries;
    gint result;

    g_return_val_if_fail(database != NULL, NULL);

    result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result != SQLITE_OK)
    {
        set_sqlite_error(error, database, result);
        return NULL;
    }

    entries = g_ptr_array_new_with_free_func(clipboard_entry_free);
    while ((result = sqlite3_step(statement)) == SQLITE_ROW)
    {
        const gchar *content = (const gchar *)sqlite3_column_text(statement, 1);
        ClipboardEntry *entry = g_new0(ClipboardEntry, 1);
        entry->id = sqlite3_column_int64(statement, 0);
        entry->content = g_strdup(content != NULL ? content : "");
        g_ptr_array_add(entries, entry);
    }

    if (result != SQLITE_DONE)
    {
        set_sqlite_error(error, database, result);
        sqlite3_finalize(statement);
        g_ptr_array_unref(entries);
        return NULL;
    }

    sqlite3_finalize(statement);
    return entries;
}

gboolean database_delete_entry(sqlite3 *database, sqlite3_int64 id, GError **error)
{
    static const gchar sql[] = "DELETE FROM history WHERE id = ?;";
    sqlite3_stmt *statement = NULL;
    gint result;

    g_return_val_if_fail(database != NULL, FALSE);

    result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result == SQLITE_OK)
    {
        result = sqlite3_bind_int64(statement, 1, id);
    }
    if (result == SQLITE_OK)
    {
        result = sqlite3_step(statement);
    }
    if (result != SQLITE_DONE)
    {
        set_sqlite_error(error, database, result);
        sqlite3_finalize(statement);
        return FALSE;
    }

    sqlite3_finalize(statement);
    return TRUE;
}

gboolean database_clear_history(sqlite3 *database, GError **error)
{
    static const gchar sql[] = "DELETE FROM history;";
    gint result;

    g_return_val_if_fail(database != NULL, FALSE);

    result = sqlite3_exec(database, sql, NULL, NULL, NULL);
    if (result != SQLITE_OK)
    {
        return set_sqlite_error(error, database, result);
    }

    return TRUE;
}
