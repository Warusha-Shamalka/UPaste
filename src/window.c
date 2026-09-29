#include <gtk/gtk.h>
#include <string.h>

#include "clipboard_manager/database.h"
#include "clipboard_manager/window.h"

typedef struct
{
    sqlite3 *database;
    ClipboardMonitor *monitor;
    GtkWidget *list;
    GtkWidget *search;
    GtkWidget *status;
    GtkWidget *monitor_button;
} WindowState;

static void set_status(WindowState *state, const gchar *message)
{
    gtk_label_set_text(GTK_LABEL(state->status), message);
}

static void apply_search_filter(WindowState *state)
{
    GList *rows = gtk_container_get_children(GTK_CONTAINER(state->list));
    const gchar *search_text = gtk_entry_get_text(GTK_ENTRY(state->search));
    gchar *folded_search = g_utf8_casefold(search_text, -1);

    for (GList *item = rows; item != NULL; item = item->next)
    {
        GtkWidget *row = item->data;
        const gchar *content = g_object_get_data(G_OBJECT(row), "history-content");
        gchar *folded_content = g_utf8_casefold(content != NULL ? content : "", -1);
        gboolean matches = folded_search[0] == '\0' || strstr(folded_content, folded_search) != NULL;

        if (matches)
        {
            gtk_widget_show(row);
        }
        else
        {
            gtk_widget_hide(row);
        }

        g_free(folded_content);
    }

    g_list_free(rows);
    g_free(folded_search);
}

static void refresh_history(WindowState *state)
{
    GError *error = NULL;
    GPtrArray *entries = database_get_history(state->database, &error);
    GList *rows;

    if (entries == NULL)
    {
        set_status(state, error != NULL ? error->message : "Unable to load clipboard history.");
        g_clear_error(&error);
        return;
    }

    rows = gtk_container_get_children(GTK_CONTAINER(state->list));
    for (GList *item = rows; item != NULL; item = item->next)
    {
        gtk_widget_destroy(GTK_WIDGET(item->data));
    }
    g_list_free(rows);

    for (guint i = 0; i < entries->len; i++)
    {
        ClipboardEntry *entry = g_ptr_array_index(entries, i);
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(entry->content);
        sqlite3_int64 *entry_id = g_new(sqlite3_int64, 1);

        *entry_id = entry->id;
        gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_max_width_chars(GTK_LABEL(label), 90);
        gtk_widget_set_margin_start(label, 10);
        gtk_widget_set_margin_end(label, 10);
        gtk_widget_set_margin_top(label, 8);
        gtk_widget_set_margin_bottom(label, 8);
        gtk_widget_set_tooltip_text(label, entry->content);
        gtk_container_add(GTK_CONTAINER(row), label);
        g_object_set_data_full(G_OBJECT(row), "history-content", g_strdup(entry->content), g_free);
        g_object_set_data_full(G_OBJECT(row), "history-id", entry_id, g_free);
        gtk_list_box_insert(GTK_LIST_BOX(state->list), row, -1);
        gtk_widget_show_all(row);
    }

    g_ptr_array_unref(entries);
    apply_search_filter(state);
    set_status(state, "Clipboard history is stored locally on this device.");
}

static void on_monitor_changed(gpointer user_data)
{
    refresh_history(user_data);
}

static void copy_selected(GtkButton *button, gpointer user_data)
{
    WindowState *state = user_data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->list));
    const gchar *content;

    (void)button;
    if (row == NULL)
    {
        set_status(state, "Select a clipboard item to copy.");
        return;
    }

    content = g_object_get_data(G_OBJECT(row), "history-content");
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), content, -1);
    clipboard_monitor_note_text(state->monitor, content);
    set_status(state, "Copied the selected item to the clipboard.");
}

static void on_row_activated(GtkListBox *list, GtkListBoxRow *row, gpointer user_data)
{
    (void)list;
    gtk_list_box_select_row(GTK_LIST_BOX(((WindowState *)user_data)->list), row);
    copy_selected(NULL, user_data);
}

static void delete_selected(GtkButton *button, gpointer user_data)
{
    WindowState *state = user_data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(state->list));
    sqlite3_int64 *entry_id;
    GError *error = NULL;

    (void)button;
    if (row == NULL)
    {
        set_status(state, "Select a clipboard item to delete.");
        return;
    }

    entry_id = g_object_get_data(G_OBJECT(row), "history-id");
    if (!database_delete_entry(state->database, *entry_id, &error))
    {
        set_status(state, error != NULL ? error->message : "Unable to delete clipboard item.");
        g_clear_error(&error);
        return;
    }

    refresh_history(state);
    set_status(state, "Deleted the selected clipboard item.");
}

static void clear_history(GtkButton *button, gpointer user_data)
{
    WindowState *state = user_data;
    GtkWidget *dialog;
    gint response;
    GError *error = NULL;

    (void)button;
    dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(state->list)),
                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_WARNING,
                                    GTK_BUTTONS_CANCEL,
                                    "Permanently clear all clipboard history?");
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Clear History", GTK_RESPONSE_ACCEPT);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response != GTK_RESPONSE_ACCEPT)
    {
        return;
    }

    if (!database_clear_history(state->database, &error))
    {
        set_status(state, error != NULL ? error->message : "Unable to clear clipboard history.");
        g_clear_error(&error);
        return;
    }

    refresh_history(state);
    set_status(state, "Clipboard history cleared.");
}

static void toggle_monitoring(GtkButton *button, gpointer user_data)
{
    WindowState *state = user_data;
    gboolean active = !clipboard_monitor_is_active(state->monitor);

    (void)button;
    clipboard_monitor_set_active(state->monitor, active);
    gtk_button_set_label(GTK_BUTTON(state->monitor_button), active ? "Pause Monitoring" : "Resume Monitoring");
    set_status(state, active ? "Clipboard monitoring is active." : "Clipboard monitoring is paused.");
}

static void on_search_changed(GtkSearchEntry *search, gpointer user_data)
{
    (void)search;
    apply_search_filter(user_data);
}

static void on_window_destroy(GtkWidget *window, gpointer user_data)
{
    (void)window;
    (void)user_data;
    gtk_main_quit();
}

void window_show_main(sqlite3 *database, ClipboardMonitor *monitor)
{
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *title = gtk_label_new(NULL);
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget *actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *copy_button = gtk_button_new_with_label("Copy");
    GtkWidget *delete_button = gtk_button_new_with_label("Delete");
    GtkWidget *clear_button = gtk_button_new_with_label("Clear History");
    GtkWidget *placeholder = gtk_label_new("No clipboard history yet. Copy some text to get started.");
    WindowState *state = g_new0(WindowState, 1);

    state->database = database;
    state->monitor = monitor;
    state->search = gtk_search_entry_new();
    state->list = gtk_list_box_new();
    state->status = gtk_label_new("Clipboard monitoring is active.");
    state->monitor_button = gtk_button_new_with_label("Pause Monitoring");

    gtk_label_set_markup(GTK_LABEL(title), "<b>UPaste</b>");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_widget_set_hexpand(title, TRUE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->search), "Search clipboard history");
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state->list), GTK_SELECTION_SINGLE);
    gtk_list_box_set_placeholder(GTK_LIST_BOX(state->list), placeholder);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), state->list);
    gtk_widget_set_size_request(scroll, 360, 300);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_halign(state->status, GTK_ALIGN_START);

    gtk_container_add(GTK_CONTAINER(window), layout);
    gtk_container_set_border_width(GTK_CONTAINER(layout), 12);
    gtk_box_pack_start(GTK_BOX(layout), toolbar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), title, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(toolbar), state->monitor_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(layout), state->search, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(layout), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), actions, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(actions), copy_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(actions), delete_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(actions), clear_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(layout), state->status, FALSE, FALSE, 0);

    gtk_window_set_title(GTK_WINDOW(window), "UPaste — Clipboard Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_object_set_data_full(G_OBJECT(window), "window-state", state, g_free);
    g_signal_connect(copy_button, "clicked", G_CALLBACK(copy_selected), state);
    g_signal_connect(delete_button, "clicked", G_CALLBACK(delete_selected), state);
    g_signal_connect(clear_button, "clicked", G_CALLBACK(clear_history), state);
    g_signal_connect(state->monitor_button, "clicked", G_CALLBACK(toggle_monitoring), state);
    g_signal_connect(state->search, "search-changed", G_CALLBACK(on_search_changed), state);
    g_signal_connect(state->list, "row-activated", G_CALLBACK(on_row_activated), state);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    clipboard_monitor_set_changed_callback(monitor, on_monitor_changed, state);
    refresh_history(state);
    gtk_widget_show_all(window);
}
