// Copyright 2010 Rubix Consulting, Inc.

#include "./client_gtk.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>

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
  edit_file("/tmp/note.txt.tmp");
}

void on_search_entry_changed() {
  g_warning("on_search_entry_changed");
}

void on_main_window_destroy() {
  gtk_main_quit();
}

void setup_signals() {
  struct sigaction sa;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = quit_handler;

  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
}

void quit_handler(int signum) {
  // TODO(jrubin) cleanup temp files
  gtk_main_quit();
}

int edit_file(const std::string& filename) {
  int status;
  pid_t pid, rv;

  switch (pid = fork()) {
    case -1:
      g_error("unable to run %s", VIM_EDITOR);
      break;
    case 0:
      if (execlp(VIM_EDITOR, "-f", "-g", filename.c_str(), NULL)) {
        int errsv = errno;
        g_warning("error executing %s: %d", VIM_EDITOR, errsv);
      }
      exit(EXIT_SUCCESS);
      break;
  }

  do {
    rv = waitpid(pid, &status, 0);
  } while (rv == -1 && errno == EINTR);

  if (rv == -1 || !WIFEXITED(status)) {
    return -1;
  }
  return WEXITSTATUS(status);
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
