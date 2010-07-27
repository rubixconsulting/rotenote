// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_CLIENT_GTK_CLIENT_GTK_H_
#define SRC_CLIENT_GTK_CLIENT_GTK_H_

#include <gtk/gtk.h>

extern "C" {
G_MODULE_EXPORT void on_quit_button_clicked(GtkToolButton *button,
                                            gpointer data);
}

#endif  // SRC_CLIENT_GTK_CLIENT_GTK_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
