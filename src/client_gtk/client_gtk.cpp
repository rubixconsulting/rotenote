// Copyright 2010 Rubix Consulting, Inc.

#include "./client_gtk.h"
#include <stdlib.h>
#include <iostream>

G_MODULE_EXPORT void on_quit_button_clicked(GtkToolButton *button,
                                            gpointer data) {
  gtk_main_quit();
}

int main (int argc, char **argv) {
  GtkBuilder *builder;
  GtkWidget  *window;
  GError     *error = NULL;

  gtk_init( &argc, &argv );

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, "client_gtk.glade", &error)) {
    g_warning("%s", error->message);
    g_free(error);
    return EXIT_FAILURE;
  }

  window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  gtk_builder_connect_signals(builder, NULL);
  g_object_unref(G_OBJECT(builder));
  gtk_widget_show(window);

  gtk_main();

  return EXIT_SUCCESS;
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
