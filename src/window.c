#include <gtk/gtk.h>

#include "clipboard_manager/window.h"

static void on_window_destroy(GtkWidget *window, gpointer user_data)
{
    (void)window;
    (void)user_data;
    gtk_main_quit();
}

void window_show_main(void)
{
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(window), "Clipboard Manager");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 500);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    gtk_widget_show_all(window);
}
