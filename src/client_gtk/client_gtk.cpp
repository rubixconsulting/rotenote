// Copyright 2010 Rubix Consulting, Inc.

#include "./client_gtk.h"
#include "../lib/note.h"
#include "../lib/rote_db.h"
#include <sstream>
#include <string.h>
#include <stdlib.h>

using ::rubix::rote_db;
using ::rubix::note;
using ::rubix::notes;
using ::rubix::tags;
using ::rubix::MODIFIED_DESC;

GtkListStore  *note_store = NULL;
GtkListStore  *tag_store  = NULL;
GtkTextBuffer *buffer     = NULL;
rote_db       *db         = NULL;

int main (int argc, char **argv) {
  GtkWidget        *window         = NULL;
  GtkBuilder       *builder        = NULL;
  GtkTextView      *text_view      = NULL;
  GtkTreeView      *note_view      = NULL;
  GtkTreeView      *tag_view       = NULL;
  GtkTreeSelection *note_selection = NULL;
  GtkTreeSelection *tag_selection  = NULL;
  GError           *error          = NULL;
  std::stringstream data;

  db = new rote_db(dbfile());

  gtk_init(&argc, &argv);

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, GLADE_FILE, &error)) {
    g_warning("%s", error->message);
    g_free(error);
    return EXIT_FAILURE;
  }

  window     = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  text_view  = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "text_view"));
  note_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "note_list_store"));
  tag_store  = GTK_LIST_STORE(gtk_builder_get_object(builder, "tag_list_store"));
  note_view  = GTK_TREE_VIEW(gtk_builder_get_object(builder, "note_view"));
  tag_view   = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tag_view"));

  buffer = gtk_text_view_get_buffer(text_view);

  note_selection = gtk_tree_view_get_selection(note_view);
  gtk_tree_selection_set_mode(note_selection, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(note_selection),
                   "changed",
                   G_CALLBACK(on_note_selection_changed),
                   NULL);

  tag_selection  = gtk_tree_view_get_selection(tag_view);
  gtk_tree_selection_set_mode(tag_selection, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(tag_selection),
                   "changed",
                   G_CALLBACK(on_tag_selection_changed),
                   NULL);

  gtk_builder_connect_signals(builder, NULL);
  g_object_unref(G_OBJECT(builder));

  show_notes_in_list(MODIFIED_DESC);
  show_tags_in_list();

  gtk_widget_show(window);

  gtk_main();

  return EXIT_SUCCESS;
}

std::string dbfile() {
  char *home = getenv("HOME");
  std::stringstream ss;
  ss << home << "/." << DB_NAME;
  return ss.str();
}

std::string format_note(const note& n) {
  std::string out;

  out  = "<big><b>";
  out += n.title().substr(0, MAX_TITLE_LENGTH);
  out += "</b></big>\n";
  out += "<i>  ";
  out += n.body().substr(0, MAX_BODY_LENGTH);
  out += "</i>";

  return out;
}

std::string format_tag(const std::string& in) {
  return "<small><i>"+in+"</i></small>";
}

void append_note_to_list(const note& n) {
  GtkTreeIter iter;
  const std::string markup = format_note(n);
  gtk_list_store_append(note_store, &iter);
  gtk_list_store_set(note_store, &iter, 0, n.id(), 1, n.value().c_str(), 2, markup.c_str(), -1);
}

void append_tag_to_list(const std::string& tag) {
  GtkTreeIter iter;
  const std::string markup = format_tag(tag);
  gtk_list_store_append(tag_store, &iter);
  gtk_list_store_set(tag_store, &iter, 0, tag.c_str(), 1, markup.c_str(), -1);
}

void prepend_note_to_list(const note& n) {
  GtkTreeIter iter;
  const std::string markup = format_note(n);
  gtk_list_store_prepend(note_store, &iter);
  gtk_list_store_set(note_store, &iter, 0, n.id(), 1, n.value().c_str(), 2, markup.c_str(), -1);
}

void prepend_tag_to_list(const std::string& tag) {
  GtkTreeIter iter;
  const std::string markup = format_tag(tag);
  gtk_list_store_prepend(tag_store, &iter);
  gtk_list_store_set(tag_store, &iter, 0, tag.c_str(), 1, markup.c_str(), -1);
}

void on_note_selection_changed(GtkTreeSelection *ts) {
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected(ts, NULL, &iter)) {
    GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
    gint note_id;
    gtk_tree_model_get(tm, &iter, 0, &note_id, -1);
    show_note_in_buffer(note_id);
  } else {
    clear_buffer();
  }
}

void on_tag_selection_changed(GtkTreeSelection *ts) {
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected(ts, NULL, &iter)) {
    GtkTreeModel *tm = GTK_TREE_MODEL(tag_store);
    gchar *tag;
    gtk_tree_model_get(tm, &iter, 0, &tag, -1);
    show_notes_with_tag_in_list(tag);
  } else {
    show_notes_in_list(MODIFIED_DESC);
  }
}

void show_note_in_buffer(const gint& note_id) {
  // TODO(jrubin) show note
  g_warning("show note %d", note_id);
}

void clear_buffer() {
  gtk_text_buffer_set_text(buffer, "", -1);
}

void show_notes_in_list(const rubix::sort& sort) {
  notes ns = db->list_notes(sort);
  for (notes::const_iterator it = ns.begin(); it != ns.end(); ++it) {
    const note& n = *it;
    append_note_to_list(n);
  }
}

void show_tags_in_list() {
  // TODO(jrubin)
  g_warning("showing all tags");
}

void show_notes_with_tag_in_list(const std::string& tag) {
  // TODO(jrubin)
  g_warning("showing notes with tag %s", tag.c_str());
}

void clear_notes_list() {
  gtk_list_store_clear(note_store);
}

void clear_tags_list() {
  gtk_list_store_clear(tag_store);
}

void on_quit_button_clicked() {
  gtk_main_quit();
}

void on_preferences_button_clicked() {
  g_warning("on_preferences_button_clicked");
}

void on_refresh_button_clicked() {
  g_warning("on_refresh_button_clicked");
}

void on_edit_button_clicked() {
  g_warning("on_edit_button_clicked");
}

void on_add_button_clicked() {
  edit_note(NULL);
}

void on_search_entry_changed() {
  g_warning("on_search_entry_changed");
}

void on_main_window_destroy() {
  gtk_main_quit();
}

GPid edit_note(const note* n) {
  char* fn = new char[L_tmpnam];
  GPid pid;
  gchar *argv[6];

  argv[0] = (gchar*)VIM_EDITOR;
  argv[1] = (gchar*)"-f";
  argv[2] = (gchar*)"--cmd";
  argv[3] = (gchar*)"set guioptions-=m guioptions-=T lines=40 columns=100";
  argv[4] = tmpnam_r(fn);
  argv[5] = NULL;

  if (!fn) {
    g_error("could not get temp file name");
  }

  g_warning("using temp file %s", fn);

  GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);

  tmp_note *tn = new tmp_note;
  if (n) {
    tn->note = *n;
  }
  tn->file = fn;

  g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL);
  g_child_watch_add(pid, done_editing, tn);

  if (n) {
    g_warning("spawned editor pid %d with data %d and file %s", pid, n->id(), fn);
  } else {
    g_warning("spawned editor pid %d with data 0 and file %s", pid, fn);
  }

  delete [] fn;

  return pid;
}

void done_editing(GPid pid, gint status, gpointer data) {
  tmp_note *tn = (tmp_note*)data;
  g_warning("editor pid %d returned with status %d, data %d and file %s", pid, status, tn->note.id(), tn->file.c_str());
  delete (tmp_note*)data;
  g_spawn_close_pid(pid);
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
