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

enum {
  NOTE_ID_COLUMN,
  NOTE_COLUMN,
  NOTE_MARKUP_COLUMN
};

enum {
  TAG_COLUMN,
  TAG_MARKUP_COLUMN
};

typedef struct {
  rubix::note note;
  std::string file;
} tmp_note;

extern "C" {
void     on_quit_button_clicked();
void     on_preferences_button_clicked();
void     on_refresh_button_clicked();
void     on_edit_button_clicked();
void     on_delete_button_clicked();
void     on_add_button_clicked();
void     on_search_entry_changed();
void     on_main_window_destroy();
gboolean on_search_entry_focus_in_event();
gboolean on_search_entry_focus_out_event();
}

void        done_editing(GPid, gint, gpointer);
void        append_note_to_list(const rubix::note&);
void        append_tag_to_list(const std::string&);
void        prepend_note_to_list(const rubix::note&);
void        prepend_tag_to_list(const std::string&);
void        update_note_in_list(const rubix::note&);
void        delete_note_from_list(const rubix::note&);
void        clear_notes_list();
void        clear_tags_list();
void        clear_buffer();
void        show_note_in_buffer(const gint&);
void        show_notes_in_list(const rubix::sort&);
void        show_tags_in_list();
void        show_notes_with_tag_in_list(const std::string&, const rubix::sort&);
void        on_note_selection_changed(GtkTreeSelection*);
void        on_tag_selection_changed(GtkTreeSelection*);
void        on_note_select_previous();
void        on_note_select_next();
void        make_temp_dir();
void        delete_temp_dir();
void        delete_note(const rubix::note&);
void        select_first_note();
void        select_last_note();
void        select_note_in_list(const rubix::note&);
void        select_previous_note();
void        select_next_note();
void        select_tag_in_list(const std::string&);
void        enable_hotkeys();
void        disable_hotkeys();
gboolean    get_iter_for_note_in_list(const rubix::note&, GtkTreeIter*);
gboolean    get_iter_for_tag_in_list(const std::string&, GtkTreeIter*);
gboolean    selected_note_iter(GtkTreeIter*);
gboolean    selected_tag_iter(GtkTreeIter*);
GPid        edit_note(const rubix::note&);
gchar**     editor_argv(gchar*);
rubix::note selected_note();
std::string selected_tag();
std::string format_note_for_buffer(const rubix::note&);
std::string format_note_for_list(const rubix::note&);
std::string format_tag(const std::string&);
std::string dbfile();

#endif  // SRC_CLIENT_GTK_CLIENT_GTK_H_

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
