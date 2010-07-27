// Copyright 2010 Rubix Consulting, Inc.

#include "./client_gtk.h"
#include <string.h>
#include <stdlib.h>

int main (int argc, char **argv) {
  GtkBuilder *builder;
  GtkWidget  *window;
  GError     *error = NULL;

  gtk_init( &argc, &argv );

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, GLADE_FILE, &error)) {
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

  argv[0] = (gchar*)"rgvim";
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
