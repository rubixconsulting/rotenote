// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_CLIENT_GTK_CLIENT_GTK_H_
#define SRC_CLIENT_GTK_CLIENT_GTK_H_

#include <stdint.h>
#include <gtk/gtk.h>
#include <string>

#define GLADE_FILE "client_gtk.glade"
#define VIM_EDITOR "rgvim"
#define MAX_TITLE_LENGTH 40
#define MAX_BODY_LENGTH 40

extern "C" {
void on_quit_button_clicked();
void on_preferences_button_clicked();
void on_refresh_button_clicked();
void on_edit_button_clicked();
void on_add_button_clicked();
void on_search_entry_changed();
void on_main_window_destroy();
}

GPid edit_note(const std::string&, const uint32_t&);
void done_editing(GPid, gint, gpointer);
void append_note_to_store(GtkListStore*, const uint32_t&, const std::string&);
void append_tag_to_store(GtkListStore*, const std::string&);
std::string format_note(const std::string&);
std::string format_tag(const std::string&);

#endif  // SRC_CLIENT_GTK_CLIENT_GTK_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
