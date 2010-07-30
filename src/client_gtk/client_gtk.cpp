// Copyright 2010 Rubix Consulting, Inc.

#include "./client_gtk.h"
#include "../lib/note.h"
#include "../lib/rote_db.h"
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


using ::rubix::rote_db;
using ::rubix::note;
using ::rubix::notes;
using ::rubix::tags;
using ::rubix::MODIFIED_DESC;
using ::std::string;
using ::std::ifstream;
using ::std::stringstream;

GtkListStore  *note_store    = NULL;
GtkListStore  *tag_store     = NULL;
GtkTextBuffer *buffer        = NULL;
GtkToolButton *edit_button   = NULL;
GtkToolButton *delete_button = NULL;
GtkTreeView   *note_view     = NULL;
rote_db       *db            = NULL;
string        *tmp_dir       = new string();
uint32_t       new_note_id   = 0;

int main(int argc, char **argv) {
  GtkWidget        *window         = NULL;
  GtkBuilder       *builder        = NULL;
  GtkTextView      *text_view      = NULL;
  GtkTreeView      *tag_view       = NULL;
  GtkTreeSelection *note_selection = NULL;
  GtkTreeSelection *tag_selection  = NULL;
  GError           *error          = NULL;
  stringstream      data;

  db = new rote_db(dbfile());

  make_temp_dir();

  gtk_init(&argc, &argv);

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, GLADE_FILE, &error)) {
    g_warning("%s", error->message);
    g_free(error);
    return EXIT_FAILURE;
  }

  window        = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  text_view     = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "text_view"));
  note_store    = GTK_LIST_STORE(gtk_builder_get_object(builder, "note_list_store"));
  tag_store     = GTK_LIST_STORE(gtk_builder_get_object(builder, "tag_list_store"));
  note_view     = GTK_TREE_VIEW(gtk_builder_get_object(builder, "note_view"));
  tag_view      = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tag_view"));
  edit_button   = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "edit_button"));
  delete_button = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "delete_button"));

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

  gtk_tree_selection_select_path(note_selection,
                                 gtk_tree_path_new_from_string("0"));

  gtk_widget_show(window);

  gtk_main();

  delete_temp_dir();

  return EXIT_SUCCESS;
}

void make_temp_dir() {
  string tds = TMP_PATH_TEMPLATE;
  char td[tds.size()+1];
  strncpy(td, tds.c_str(), tds.size()+1);

  if (!mkdtemp(td)) {
    g_error("could not get temp dir from template %s", TMP_PATH_TEMPLATE);
  }

  *tmp_dir = td;
}

void delete_temp_dir() {
  if (rmdir(tmp_dir->c_str())) {
    g_error("could not delete temp dir: %d", errno);
  }
}

string dbfile() {
  gchar *home = getenv("HOME");
  stringstream ss;
  ss << home << "/." << DB_NAME;
  return ss.str();
}

string format_note_for_buffer(const note& n) {
  // TODO(jrubin)
  return n.value();
}

string format_note_for_list(const note& n) {
  string out;

  std::string title_out;
  title_out = n.title().substr(0, MAX_TITLE_LENGTH);

  const std::string& body_in = n.body();
  std::string body_out;
  for (uint32_t i = 0; i < MAX_BODY_LENGTH; ++i) {
    if (body_in.size() < i) {
      break;
    } else if (body_in[i] == '\n') {
      body_out += ' ';
      continue;
    }
    body_out += body_in[i];
  }

  out += g_markup_printf_escaped("<big><b>%s</b></big>\n", title_out.c_str());
  out += g_markup_printf_escaped("<i>  %s</i>", body_out.c_str());

  return out;
}

string format_tag(const string& in) {
  return "<small><i>"+in+"</i></small>";
}

void append_note_to_list(const note& n) {
  GtkTreeIter iter;
  const string markup = format_note_for_list(n);
  gtk_list_store_append(note_store, &iter);
  gtk_list_store_set(note_store, &iter,
                     NOTE_ID_COLUMN,     n.id(),
                     NOTE_COLUMN,        n.value().c_str(),
                     NOTE_MARKUP_COLUMN, markup.c_str(),
                     -1);
}

void update_note_in_list(const note& n) {
  if (!n.id()) {
    return;
  }

  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  GtkTreeIter iter;
  uint32_t note_id;
  gboolean valid = gtk_tree_model_get_iter_first(tm, &iter);
  while (valid) {
    gtk_tree_model_get(tm, &iter, NOTE_ID_COLUMN, &note_id, -1);
    if (note_id == n.id()) {
      const string markup = format_note_for_list(n);
      gtk_list_store_set(note_store, &iter,
                         NOTE_COLUMN,        n.value().c_str(),
                         NOTE_MARKUP_COLUMN, markup.c_str(),
                         -1);
      if (n.id() == selected_note().id()) {
        show_note_in_buffer(n.id());
      }
      break;
    }
    valid = gtk_tree_model_iter_next(tm, &iter);
  }
}

void prepend_note_to_list(const note& n) {
  GtkTreeIter iter;
  const string markup = format_note_for_list(n);
  gtk_list_store_prepend(note_store, &iter);
  gtk_list_store_set(note_store, &iter,
                     NOTE_ID_COLUMN,     n.id(),
                     NOTE_COLUMN,        n.value().c_str(),
                     NOTE_MARKUP_COLUMN, markup.c_str(),
                     -1);
}

void append_tag_to_list(const string& tag) {
  GtkTreeIter iter;
  const string markup = format_tag(tag);
  gtk_list_store_append(tag_store, &iter);
  gtk_list_store_set(tag_store, &iter,
                     TAG_COLUMN,        tag.c_str(),
                     TAG_MARKUP_COLUMN, markup.c_str(),
                     -1);
}

void prepend_tag_to_list(const string& tag) {
  GtkTreeIter iter;
  const string markup = format_tag(tag);
  gtk_list_store_prepend(tag_store, &iter);
  gtk_list_store_set(tag_store, &iter,
                     TAG_COLUMN,        tag.c_str(),
                     TAG_MARKUP_COLUMN, markup.c_str(),
                     -1);
}

void on_note_selection_changed(GtkTreeSelection *ts) {
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected(ts, NULL, &iter)) {
    GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
    gint note_id;
    gtk_tree_model_get(tm, &iter, NOTE_ID_COLUMN, &note_id, -1);
    show_note_in_buffer(note_id);
    gtk_widget_set_sensitive(GTK_WIDGET(edit_button),   true);
    gtk_widget_set_sensitive(GTK_WIDGET(delete_button), true);
  } else {
    clear_buffer();
    gtk_widget_set_sensitive(GTK_WIDGET(edit_button),   false);
    gtk_widget_set_sensitive(GTK_WIDGET(delete_button), false);
  }
}

void on_tag_selection_changed(GtkTreeSelection *ts) {
  GtkTreeIter iter;
  if (gtk_tree_selection_get_selected(ts, NULL, &iter)) {
    GtkTreeModel *tm = GTK_TREE_MODEL(tag_store);
    gchar *tag;
    gtk_tree_model_get(tm, &iter, TAG_COLUMN, &tag, -1);
    show_notes_with_tag_in_list(tag);
  } else {
    show_notes_in_list(MODIFIED_DESC);
  }
}

void show_note_in_buffer(const gint& note_id) {
  note n = db->by_id(note_id);
  string text = format_note_for_buffer(n);
  gtk_text_buffer_set_text(buffer, text.c_str(), text.size());
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
  tags ts = db->list_tags();
  for (tags::const_iterator it = ts.begin(); it != ts.end(); ++it) {
    const string& t = *it;
    append_tag_to_list(t);
  }
}

void show_notes_with_tag_in_list(const string& tag) {
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

note selected_note() {
  GtkTreeSelection *ts = gtk_tree_view_get_selection(note_view);
  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected(ts, NULL, &iter)) {
    return note();
  }
  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  gint note_id;
  gtk_tree_model_get(tm, &iter, NOTE_ID_COLUMN, &note_id, -1);
  return db->by_id(note_id);
}

void on_edit_button_clicked() {
  edit_note(selected_note());
}

void on_delete_button_clicked() {
  delete_note(selected_note());
}

void on_add_button_clicked() {
  edit_note(note());
}

void on_search_entry_changed() {
  g_warning("on_search_entry_changed");
}

void on_main_window_destroy() {
  gtk_main_quit();
}

gchar** editor_argv(gchar *fn) {
  gchar **argv = new gchar*[6];

  argv[0] = (gchar*)VIM_EDITOR;
  argv[1] = (gchar*)"-f";
  argv[2] = (gchar*)"--cmd";
  argv[3] = (gchar*)"set guioptions-=m guioptions-=T lines=40 columns=100";
  argv[4] = fn;
  argv[5] = NULL;

  return argv;
}

bool delete_note(const note& n) {
  // TODO(jrubin)
  g_warning("delete note id: %d", n.id());
  return false;
}

GPid edit_note(const note& n) {
  stringstream fns;
  fns << *tmp_dir << "/";
  if (n.id()) {
    fns << "e" << n.id();
  } else {
    fns << "n" << new_note_id;
    ++new_note_id;
  }
  gchar *fn = new char[fns.str().size()+1];
  strncpy(fn, fns.str().c_str(), fns.str().size());

  if (n.id()) {
    n.write_to_file(fn);
  }

  gchar **argv = editor_argv(fn);
  GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);
  GPid pid = 0;

  g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL);

  delete [] argv;

  tmp_note *tn = new tmp_note;
  if (n.id()) {
    tn->note = n;
  }
  tn->file = fn;

  g_child_watch_add(pid, done_editing, tn);

  delete [] fn;

  return pid;
}

void done_editing(GPid pid, gint status, gpointer data) {
  tmp_note *tn = (tmp_note*)data;

  struct stat buf;
  if (!stat(tn->file.c_str(), &buf)) {
    tn->note.load_from_file(tn->file);
    unlink(tn->file.c_str());

    if (tn->note.id()) {
      db->save_note(&tn->note);
      update_note_in_list(tn->note);
    } else if (!tn->note.value().empty()) {
      db->save_note(&tn->note);
      prepend_note_to_list(tn->note);
    }
  }

  delete (tmp_note*)data;
  g_spawn_close_pid(pid);
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
