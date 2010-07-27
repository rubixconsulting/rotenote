// Copyright 2010 Rubix Consulting, Inc.

#include "./client_gtk.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv) {
  GtkBuilder   *builder;
  GtkWidget    *window;
  GError       *error = NULL;
  GtkListStore *note_store;
  GtkListStore *tag_store;
  std::stringstream data;

  gtk_init( &argc, &argv );

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, GLADE_FILE, &error)) {
    g_warning("%s", error->message);
    g_free(error);
    return EXIT_FAILURE;
  }

  window     = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
  note_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "note_list_store"));
  tag_store  = GTK_LIST_STORE(gtk_builder_get_object(builder, "tag_list_store"));
  gtk_builder_connect_signals(builder, NULL);
  g_object_unref(G_OBJECT(builder));

  gtk_widget_show(window);

  for (uint32_t i = 0; i < 10; ++i) {
    data << "Title " << i << "\nBody " << i;
    append_note_to_store(note_store, i, data.str());
    data.str("");
    data << "Tag" << i;
    append_tag_to_store(tag_store, data.str());
    data.str("");
  }

  gtk_main();

  return EXIT_SUCCESS;
}

std::string format_note(const std::string& in) {
  const std::string title_start = "<big><b>";
  const std::string title_end   = "</b></big>";
  const std::string body_start  = "<i>";
  const std::string body_end    = "</i>";

  std::string out = title_start;

  for (uint32_t i=0; i < in.size(); ++i) {
    unsigned char j = in[i];
    if (i > MAX_TITLE_LENGTH) {
      break;
    } else if (j == '\n') {
      break;
    }
    out += j;
  }

  out += title_end + body_start;

  bool found_body = false;
  for (uint32_t i=0; i < in.size(); ++i) {
    unsigned char j = in[i];
    if (!found_body && (j != '\n')) {
      continue;
    }
    found_body = true;
    if (i > MAX_BODY_LENGTH) {
      break;
    }
    out += j;
  }

  out += body_end;

  return out;
}

std::string format_tag(const std::string& in) {
  std::string out = "<small><i>"+in+"</i></small>";
  return out;
}

void append_note_to_store(GtkListStore *note_store, const uint32_t& note_id, const std::string& note) {
  GtkTreeIter iter;
  const std::string markup = format_note(note);
  gtk_list_store_append(note_store, &iter);
  gtk_list_store_set(note_store, &iter, 0, note_id, 1, note.c_str(), 2, markup.c_str(), -1);
}

void append_tag_to_store(GtkListStore *tag_store, const std::string& tag) {
  GtkTreeIter iter;
  const std::string markup = format_tag(tag);
  gtk_list_store_append(tag_store, &iter);
  gtk_list_store_set(tag_store, &iter, 0, tag.c_str(), 1, markup.c_str(), -1);
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
  edit_note("/tmp/note.txt.tmp", 12345);  // TODO(jrubin) use the real note id
}

void on_search_entry_changed() {
  g_warning("on_search_entry_changed");
}

void on_main_window_destroy() {
  gtk_main_quit();
}

GPid edit_note(const std::string& fs, const uint32_t& data) {
  char *fn = new char[fs.size()+1];
  strncpy(fn, fs.c_str(), fs.size()+1);

  GPid pid;
  gchar *argv[6];

  argv[0] = (gchar*)VIM_EDITOR;
  argv[1] = (gchar*)"-f";
  argv[2] = (gchar*)"--cmd";
  argv[3] = (gchar*)"set guioptions-=m guioptions-=T lines=40 columns=100";
  argv[4] = fn;
  argv[5] = NULL;

  GSpawnFlags flags = (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH);

  int* note_id = new int(data);

  g_spawn_async(NULL, argv, NULL, flags, NULL, NULL, &pid, NULL);
  g_child_watch_add(pid, done_editing, note_id);
  g_warning("spawned editor pid %d with data %d", pid, *note_id);

  return pid;
}

void done_editing(GPid pid, gint status, gpointer data) {
  g_warning("editor pid %d returned with status %d and data %d", pid, status, *(int*)data);
  delete (int*)data;
  g_spawn_close_pid(pid);
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
