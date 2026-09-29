#include <gtk/gtk.h>
#include <stdlib.h>

#include "clipboard_manager/app.h"

int main(int argc, char **argv)
{
    GError *error = NULL;
    App *app;

    if (!gtk_init_check(&argc, &argv))
    {
        g_printerr("Unable to initialize GTK. Is a graphical session available?\n");
        return EXIT_FAILURE;
    }

    app = app_create(&error);
    if (app == NULL)
    {
        g_printerr("Failed to start Clipboard Manager: %s\n",
                   error != NULL ? error->message : "unknown error");
        g_clear_error(&error);
        return EXIT_FAILURE;
    }

    gtk_main();
    app_destroy(app);

    return EXIT_SUCCESS;
}
