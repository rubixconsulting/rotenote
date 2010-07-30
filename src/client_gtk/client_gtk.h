// Copyright 2010 Rubix Consulting, Inc.

#ifndef SRC_CLIENT_GTK_CLIENT_GTK_H_
#define SRC_CLIENT_GTK_CLIENT_GTK_H_

#include "../lib/note.h"
#include <stdint.h>
#include <gtk/gtk.h>
#include <string>

#define TMP_PATH_TEMPLATE "/tmp/rotenote_XXXXXX"
#define DB_NAME           "rotedb"
#define GLADE_FILE        "client_gtk.glade"
#define VIM_EDITOR        "rgvim"

#define MAX_TITLE_LENGTH 40
#define MAX_BODY_LENGTH  40

typedef struct {
  rubix::note note;
  std::string file;
} tmp_note;

extern "C" {
void on_quit_button_clicked();
void on_preferences_button_clicked();
void on_refresh_button_clicked();
void on_edit_button_clicked();
void on_add_button_clicked();
void on_search_entry_changed();
void on_main_window_destroy();
}

GPid        edit_note(const rubix::note*);
void        done_editing(GPid, gint, gpointer);
void        append_note_to_list(const rubix::note&);
void        append_tag_to_list(const std::string&);
void        prepend_note_to_list(const rubix::note&);
void        prepend_tag_to_list(const std::string&);
void        clear_notes_list();
void        clear_tags_list();
void        clear_buffer();
void        show_note_in_buffer(const gint&);
void        show_notes_in_list(const rubix::sort&);
void        show_tags_in_list();
void        show_notes_with_tag_in_list(const std::string&);
void        on_note_selection_changed(GtkTreeSelection*);
void        on_tag_selection_changed(GtkTreeSelection*);
void        make_temp_dir();
void        delete_temp_dir();
gchar**     editor_argv(gchar*);
std::string format_note_for_buffer(const rubix::note&);
std::string format_note_for_list(const rubix::note&);
std::string format_tag(const std::string&);
std::string dbfile();

#endif  // SRC_CLIENT_GTK_CLIENT_GTK_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
