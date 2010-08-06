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
#include <boost/algorithm/string.hpp>

using ::std::string;
using ::std::ifstream;
using ::std::stringstream;
using ::std::runtime_error;
using ::rubix::rote_db;
using ::rubix::note;
using ::rubix::notes;
using ::rubix::tags;
using ::rubix::MODIFIED_DESC;
using ::rubix::text_type;
using ::rubix::TEXT_INVALID;
using ::rubix::TEXT_TITLE;
using ::rubix::TEXT_PLAIN;
using ::rubix::TEXT_TAG;
using ::rubix::TEXT_BOLD;
using ::rubix::TEXT_LINK;
using ::rubix::TEXT_LINK_DEFAULT_HTTP;
using ::rubix::TEXT_EMAIL;
using ::rubix::TEXT_TWITTER;

GtkListStore     *note_store            = NULL;
GtkListStore     *tag_store             = NULL;
GtkTextBuffer    *buffer                = NULL;
GtkToolButton    *add_button            = NULL;
GtkToolButton    *edit_button           = NULL;
GtkToolButton    *delete_button         = NULL;
GtkToolButton    *refresh_button        = NULL;
GtkToolButton    *preferences_button    = NULL;
GtkEntry         *search_entry          = NULL;
GtkTreeView      *note_view             = NULL;
GtkTreeView      *tag_view              = NULL;
GtkTreeSelection *note_selection        = NULL;
GtkTreeSelection *tag_selection         = NULL;
GtkTextTag       *title_tag             = NULL;
GtkTextTag       *tag_tag               = NULL;
GtkTextTag       *link_tag              = NULL;
GtkTextTag       *link_default_http_tag = NULL;
GtkTextTag       *email_tag             = NULL;
GtkTextTag       *twitter_tag           = NULL;
GtkTextTag       *bold_tag              = NULL;
GtkAccelGroup    *accel_group           = NULL;
GtkStatusIcon    *tray_icon             = NULL;
GtkMenu          *tray_menu             = NULL;
rote_db          *db                    = NULL;
GtkWindow        *window                = NULL;
gboolean          window_shown          = TRUE;
string           *tmp_dir               = new string();
uint32_t          new_note_id           = 0;

int main(int argc, char **argv) {
  GtkBuilder      *builder   = NULL;
  GtkTextView     *text_view = NULL;
  GtkTextTagTable *tag_table = NULL;
  GError          *error     = NULL;
  GtkWidget       *tray_quit = NULL;
  GtkWidget       *tray_add  = NULL;
  GtkWidget       *tray_show = NULL;
  stringstream  data;

  db = new rote_db(dbfile());

  make_temp_dir();

  gtk_init(&argc, &argv);

  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file(builder, GLADE_FILE, &error)) {
    g_error("%s", error->message);
  }

  window             = GTK_WINDOW(gtk_builder_get_object(builder, "main_window"));
  text_view          = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "text_view"));
  note_store         = GTK_LIST_STORE(gtk_builder_get_object(builder, "note_list_store"));
  tag_store          = GTK_LIST_STORE(gtk_builder_get_object(builder, "tag_list_store"));
  note_view          = GTK_TREE_VIEW(gtk_builder_get_object(builder, "note_view"));
  tag_view           = GTK_TREE_VIEW(gtk_builder_get_object(builder, "tag_view"));
  add_button         = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "add_button"));
  edit_button        = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "edit_button"));
  delete_button      = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "delete_button"));
  refresh_button     = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "refresh_button"));
  preferences_button = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "preferences_button"));
  search_entry       = GTK_ENTRY(gtk_builder_get_object(builder, "search_entry"));

  buffer = gtk_text_view_get_buffer(text_view);
  tag_table = gtk_text_buffer_get_tag_table(buffer);

  title_tag             = gtk_text_tag_new("title_tag");
  tag_tag               = gtk_text_tag_new("tag_tag");
  link_tag              = gtk_text_tag_new("link_tag");
  link_default_http_tag = gtk_text_tag_new("link_default_http_tag");
  email_tag             = gtk_text_tag_new("email_tag");
  twitter_tag           = gtk_text_tag_new("twitter_tag");
  bold_tag              = gtk_text_tag_new("bold_tag");

  g_object_set(title_tag, "weight", PANGO_WEIGHT_BOLD,    "weight-set", TRUE, NULL);
  g_object_set(title_tag, "scale",  PANGO_SCALE_XX_LARGE, "scale-set",  TRUE, NULL);

  g_object_set(tag_tag, "style",     PANGO_STYLE_ITALIC,     "style-set",     TRUE, NULL);
  g_object_set(tag_tag, "underline", PANGO_UNDERLINE_SINGLE, "underline-set", TRUE, NULL);
  g_signal_connect(G_OBJECT(tag_tag), "event", G_CALLBACK(on_tag_event), NULL);

  g_object_set(link_tag, "style",     PANGO_STYLE_ITALIC,     "style-set",     TRUE, NULL);
  g_object_set(link_tag, "underline", PANGO_UNDERLINE_SINGLE, "underline-set", TRUE, NULL);
  g_signal_connect(G_OBJECT(link_tag), "event", G_CALLBACK(on_tag_event), NULL);

  g_object_set(link_default_http_tag, "style",     PANGO_STYLE_ITALIC,     "style-set",     TRUE, NULL);
  g_object_set(link_default_http_tag, "underline", PANGO_UNDERLINE_SINGLE, "underline-set", TRUE, NULL);
  g_signal_connect(G_OBJECT(link_default_http_tag), "event", G_CALLBACK(on_tag_event), NULL);

  g_object_set(email_tag, "style",     PANGO_STYLE_ITALIC,     "style-set",     TRUE, NULL);
  g_object_set(email_tag, "underline", PANGO_UNDERLINE_SINGLE, "underline-set", TRUE, NULL);
  g_signal_connect(G_OBJECT(email_tag), "event", G_CALLBACK(on_tag_event), NULL);

  g_object_set(twitter_tag, "style",     PANGO_STYLE_ITALIC,     "style-set",     TRUE, NULL);
  g_object_set(twitter_tag, "underline", PANGO_UNDERLINE_SINGLE, "underline-set", TRUE, NULL);
  g_signal_connect(G_OBJECT(twitter_tag), "event", G_CALLBACK(on_tag_event), NULL);

  g_object_set(bold_tag, "weight", PANGO_WEIGHT_BOLD, "weight-set", TRUE, NULL);

  gtk_text_tag_table_add(tag_table, title_tag);
  gtk_text_tag_table_add(tag_table, tag_tag);
  gtk_text_tag_table_add(tag_table, link_tag);
  gtk_text_tag_table_add(tag_table, link_default_http_tag);
  gtk_text_tag_table_add(tag_table, email_tag);
  gtk_text_tag_table_add(tag_table, twitter_tag);
  gtk_text_tag_table_add(tag_table, bold_tag);

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

  g_signal_new("select-previous",
               GTK_TYPE_TREE_VIEW,
               (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
               0,
               NULL,
               NULL,
               g_cclosure_marshal_VOID__VOID,
               G_TYPE_NONE,
               0);

  g_signal_new("select-next",
               GTK_TYPE_TREE_VIEW,
               (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
               0,
               NULL,
               NULL,
               g_cclosure_marshal_VOID__VOID,
               G_TYPE_NONE,
               0);

  g_signal_connect(G_OBJECT(note_view),
                   "select-previous",
                   G_CALLBACK(on_note_select_previous),
                   NULL);

  g_signal_connect(G_OBJECT(note_view),
                   "select-next",
                   G_CALLBACK(on_note_select_next),
                   NULL);

  gtk_builder_connect_signals(builder, NULL);
  g_object_unref(G_OBJECT(builder));

  accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);

  tray_icon = gtk_status_icon_new_from_stock(GTK_STOCK_EDIT);
  gtk_status_icon_set_tooltip(tray_icon, "Rote Note");
  g_signal_connect(tray_icon, "activate",   G_CALLBACK(on_tray_icon_activate),   NULL);
  g_signal_connect(tray_icon, "popup-menu", G_CALLBACK(on_tray_icon_popup_menu), NULL);

  tray_menu = GTK_MENU(gtk_menu_new());

  tray_show = gtk_menu_item_new_with_label("Show");
  tray_add  = gtk_menu_item_new_with_label("Add Note");
  tray_quit = gtk_menu_item_new_with_label("Quit");

  g_signal_connect(tray_show, "activate", G_CALLBACK(on_tray_show_activate), NULL);
  g_signal_connect(tray_add,  "activate", G_CALLBACK(on_tray_add_activate),  NULL);
  g_signal_connect(tray_quit, "activate", G_CALLBACK(on_tray_quit_activate), NULL);

  gtk_menu_shell_append(GTK_MENU_SHELL(tray_menu), tray_show);
  gtk_menu_shell_append(GTK_MENU_SHELL(tray_menu), tray_add);
  gtk_menu_shell_append(GTK_MENU_SHELL(tray_menu), tray_quit);

  gtk_widget_show_all(GTK_WIDGET(tray_menu));

  show_all_notes_in_list(MODIFIED_DESC);
  show_all_tags_in_list();
  select_tag_in_list(TAG_ALL);
  enable_hotkeys();
  gtk_widget_show(GTK_WIDGET(window));
  gtk_main();
  delete_temp_dir();

  return EXIT_SUCCESS;
}

gboolean on_main_window_key_press_event(
    __attribute__((unused))GtkWidget *widget,
    GdkEventKey *event) {
  // hide the window on ctrl-w
  if ((event->keyval == 'w') && (event->state & GDK_CONTROL_MASK)) {
    gtk_widget_hide(GTK_WIDGET(window));
    window_shown = FALSE;
  }
  return FALSE;
}

void on_tray_icon_activate() {
  if (window_shown) {
    gtk_widget_hide(GTK_WIDGET(window));
    window_shown = FALSE;
  } else {
    gtk_widget_show(GTK_WIDGET(window));
    gtk_window_deiconify(window);
    window_shown = TRUE;
  }
}

void on_tray_icon_popup_menu(GtkStatusIcon *status_icon,
                             guint button,
                             guint activate_time) {
  gtk_menu_popup(tray_menu,
                 NULL,
                 NULL,
                 gtk_status_icon_position_menu,
                 status_icon,
                 button,
                 activate_time);
}

void on_tray_quit_activate() {
  gtk_main_quit();
}

void on_tray_show_activate() {
  gtk_widget_show(GTK_WIDGET(window));
  gtk_window_deiconify(window);
  window_shown = TRUE;
}

void on_tray_add_activate() {
  edit_note(note());
}

void on_note_select_previous() {
  select_previous_note();
}

void on_note_select_next() {
  select_next_note();
}

void select_previous_note() {
  GtkTreeIter iter;
  if (!selected_note_iter(&iter)) {
    select_last_note();
    return;
  }
  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  GtkTreePath *tp = gtk_tree_model_get_path(tm, &iter);

  if (gtk_tree_path_prev(tp)) {
    gtk_tree_selection_select_path(note_selection, tp);
  }

  gtk_tree_path_free(tp);
}

void select_next_note() {
  GtkTreeIter iter;
  if (!selected_note_iter(&iter)) {
    select_first_note();
    return;
  }
  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  GtkTreePath *tp = gtk_tree_model_get_path(tm, &iter);

  gtk_tree_path_next(tp);
  gtk_tree_selection_select_path(note_selection, tp);

  gtk_tree_path_free(tp);
}

void select_first_note() {
  gtk_tree_selection_select_path(note_selection,
                                 gtk_tree_path_new_first());
}

void select_last_note() {
  GtkTreeIter iter, prev;
  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  gboolean valid = gtk_tree_model_get_iter_first(tm, &iter);
  while (valid) {
    prev = iter;
    valid = gtk_tree_model_iter_next(tm, &iter);
  }
  gtk_tree_selection_select_iter(note_selection, &prev);
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

string format_note_for_list(const note& n) {
  string out;

  string title_out;
  title_out = n.title().substr(0, MAX_TITLE_LENGTH);

  const string& body_in = n.body();
  string body_out;
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
  return g_markup_printf_escaped("<i>%s</i>", in.c_str());
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

gboolean get_iter_for_note_in_list(const note& n, GtkTreeIter *iter) {
  if (!n.id()) {
    return FALSE;
  }

  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  uint32_t note_id;
  gboolean valid = gtk_tree_model_get_iter_first(tm, iter);
  while (valid) {
    gtk_tree_model_get(tm, iter, NOTE_ID_COLUMN, &note_id, -1);
    if (note_id == n.id()) {
      return TRUE;
    }
    valid = gtk_tree_model_iter_next(tm, iter);
  }

  return FALSE;
}

gboolean get_iter_for_tag_in_list(const string& t, GtkTreeIter *iter) {
  if (t.empty()) {
    return FALSE;
  }

  GtkTreeModel *tm = GTK_TREE_MODEL(tag_store);
  gchar *tag = NULL;
  gboolean valid = gtk_tree_model_get_iter_first(tm, iter);
  while (valid) {
    gtk_tree_model_get(tm, iter, TAG_COLUMN, &tag, -1);
    if (boost::algorithm::equals(tag, t)) {
      return TRUE;
    }
    valid = gtk_tree_model_iter_next(tm, iter);
  }

  return FALSE;
}

void update_note_in_list(const note& n) {
  if (!n.id()) {
    return;
  }

  GtkTreeIter iter;
  if (!get_iter_for_note_in_list(n, &iter)) {
    return;
  }

  const string markup = format_note_for_list(n);

  gtk_list_store_set(note_store, &iter,
                     NOTE_COLUMN,        n.value().c_str(),
                     NOTE_MARKUP_COLUMN, markup.c_str(),
                     -1);

  if (n.id() == selected_note().id()) {
    show_note_in_buffer(n.id());
  }
}

void delete_note_from_list(const note& n) {
  if (!n.id()) {
    return;
  }

  GtkTreeIter iter;
  if (!get_iter_for_note_in_list(n, &iter)) {
    return;
  }

  bool selected = false;
  if (n.id() == selected_note().id()) {
    selected = true;
  }

  gtk_list_store_remove(note_store, &iter);

  if (selected) {
    select_first_note();
  }
}

void select_tag_in_list(const string& t) {
  if (t.empty()) {
    return;
  }

  GtkTreeIter iter;
  if (!get_iter_for_tag_in_list(t, &iter)) {
    if (!get_iter_for_tag_in_list(TAG_ALL, &iter)) {
      return;
    }
  }

  gtk_tree_selection_select_iter(tag_selection, &iter);
  select_first_note();
}

void select_note_in_list(const note& n) {
  if (!n.id()) {
    return;
  }

  GtkTreeIter iter;
  if (!get_iter_for_note_in_list(n, &iter)) {
    return;
  }

  gtk_tree_selection_select_iter(note_selection, &iter);
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
  if (!selected_note().id()) {
    select_first_note();
  }
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
    gtk_entry_set_text(search_entry, "");
    show_notes_with_tag_in_list(tag, MODIFIED_DESC);
  } else {
    if (search_text().empty()) {
      show_all_notes_in_list(MODIFIED_DESC);
      select_tag_in_list(TAG_ALL);
    }
  }
  select_first_note();
}

string search_text() {
  return gtk_entry_get_text(search_entry);
}

void show_note_in_buffer(const gint& note_id) {
  const note n = db->by_id(note_id);
  clear_buffer();
  string::size_type iter = 0;
  string val_part;
  text_type type;
  while ((type = n.part(&iter, &val_part)) != TEXT_INVALID) {
    switch (type) {
      case TEXT_TITLE:
        append_tag_text_to_buffer(val_part, title_tag);
        break;
      case TEXT_TAG:
        append_tag_text_to_buffer(val_part, tag_tag);
        break;
      case TEXT_LINK:
        append_tag_text_to_buffer(val_part, link_tag);
        break;
      case TEXT_LINK_DEFAULT_HTTP:
        append_tag_text_to_buffer(val_part, link_default_http_tag);
        break;
      case TEXT_EMAIL:
        append_tag_text_to_buffer(val_part, email_tag);
        break;
      case TEXT_TWITTER:
        append_tag_text_to_buffer(val_part, twitter_tag);
        break;
      case TEXT_BOLD:
        append_tag_text_to_buffer(val_part, bold_tag);
        break;
      default:
        append_text_to_buffer(val_part);
    }
  }
}

void append_text_to_buffer(const std::string& val) {
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(buffer, &iter);
  gtk_text_buffer_insert(buffer, &iter, val.c_str(), -1);
}

void append_tag_text_to_buffer(const std::string& val, GtkTextTag *tag) {
  GtkTextMark *start_mark = NULL;
  GtkTextIter start, end;

  gtk_text_buffer_get_end_iter(buffer, &start);

  start_mark = gtk_text_buffer_create_mark(buffer, NULL, &start, true);

  append_text_to_buffer(val);

  gtk_text_buffer_get_end_iter(buffer, &end);
  gtk_text_buffer_get_iter_at_mark(buffer, &start, start_mark);
  gtk_text_buffer_apply_tag(buffer, tag, &start, &end);
  gtk_text_buffer_delete_mark(buffer, start_mark);
}

void clear_buffer() {
  gtk_text_buffer_set_text(buffer, "", -1);
}

void show_all_notes_in_list(const rubix::sort& sort) {
  clear_notes_list();
  notes ns = db->list_notes(sort);
  for (notes::const_iterator it = ns.begin(); it != ns.end(); ++it) {
    const note& n = *it;
    append_note_to_list(n);
  }
  select_first_note();
}

void search(const string& text, const rubix::sort& sort) {
  clear_notes_list();
  notes ns = db->search(text, sort);
  for (notes::const_iterator it = ns.begin(); it != ns.end(); ++it) {
    const note& n = *it;
    append_note_to_list(n);
  }
  select_first_note();
}

void show_all_tags_in_list() {
  clear_tags_list();
  append_tag_to_list(TAG_ALL);
  tags ts = db->list_tags();
  for (tags::const_iterator it = ts.begin(); it != ts.end(); ++it) {
    const string& t = *it;
    if (!boost::algorithm::equals(TAG_ALL, t)) {
      append_tag_to_list(t);
    }
  }
}

void show_notes_with_tag_in_list(const string& tag, const rubix::sort& sort) {
  clear_notes_list();
  notes ns = db->by_tag(tag, sort);
  for (notes::const_iterator it = ns.begin(); it != ns.end(); ++it) {
    const note& n = *it;
    append_note_to_list(n);
  }
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
  // TODO(jrubin)
  g_warning("on_preferences_button_clicked");
}

void on_refresh_button_clicked() {
  // TODO(jrubin)
  g_warning("on_refresh_button_clicked");
}

gboolean selected_note_iter(GtkTreeIter *iter) {
  GtkTreeSelection *ts = gtk_tree_view_get_selection(note_view);
  return gtk_tree_selection_get_selected(ts, NULL, iter);
}

gboolean selected_tag_iter(GtkTreeIter *iter) {
  GtkTreeSelection *ts = gtk_tree_view_get_selection(tag_view);
  return gtk_tree_selection_get_selected(ts, NULL, iter);
}

note selected_note() {
  GtkTreeIter iter;
  if (!selected_note_iter(&iter)) {
    return note();
  }
  GtkTreeModel *tm = GTK_TREE_MODEL(note_store);
  gint note_id;
  gtk_tree_model_get(tm, &iter, NOTE_ID_COLUMN, &note_id, -1);
  return db->by_id(note_id);
}

void deselect_tag_in_list() {
  GtkTreeIter iter;
  if (!selected_tag_iter(&iter)) {
    return;
  }

  gtk_tree_selection_unselect_iter(tag_selection, &iter);
}

string selected_tag() {
  GtkTreeIter iter;
  if (!selected_tag_iter(&iter)) {
    return string();
  }
  GtkTreeModel *tm = GTK_TREE_MODEL(tag_store);
  gchar *ctag;
  gtk_tree_model_get(tm, &iter, TAG_COLUMN, &ctag, -1);
  return ctag;
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
  const string text = search_text();
  if (text.empty()) {
    gtk_entry_set_icon_from_stock(search_entry,
                                  GTK_ENTRY_ICON_SECONDARY,
                                  NULL);
    gtk_entry_set_icon_tooltip_markup(search_entry,
                                      GTK_ENTRY_ICON_SECONDARY,
                                      NULL);
  } else {
    gtk_entry_set_icon_from_stock(search_entry,
                                  GTK_ENTRY_ICON_SECONDARY,
                                  GTK_STOCK_CLEAR);
    gtk_entry_set_icon_tooltip_markup(search_entry,
                                      GTK_ENTRY_ICON_SECONDARY,
                                      "Clear");
  }
}

void on_search_entry_icon_release(__attribute__((unused))GtkEntry *entry, GtkEntryIconPosition pos) {
  switch (pos) {
    case GTK_ENTRY_ICON_PRIMARY:
      deselect_tag_in_list();
      break;
    case GTK_ENTRY_ICON_SECONDARY:
      clear_search();
      break;
  }
}

void on_search_entry_activate() {
  const string text = search_text();
  if (text.empty()) {
    clear_search();
    return;
  }
  deselect_tag_in_list();
  search(text, MODIFIED_DESC);
}

void clear_search() {
  gtk_entry_set_text(search_entry, "");
  show_all_notes_in_list(MODIFIED_DESC);
  select_tag_in_list(TAG_ALL);
}

gboolean on_main_window_delete_event() {
  // prevent window destruction
  gtk_widget_hide(GTK_WIDGET(window));
  window_shown = FALSE;
  return TRUE;
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
  tn->mtime = 0;
  if (n.id()) {
    tn->mtime = get_mtime(fn);
  }

  g_child_watch_add(pid, done_editing, tn);

  delete [] fn;

  return pid;
}

time_t get_mtime(const gchar *fn) {
  struct stat buf;
  if (!stat(fn, &buf)) {
    return buf.st_mtime;
  }
  return 0;
}

void done_editing(GPid pid, __attribute__((unused))gint status, gpointer data) {
  tmp_note *tn = (tmp_note*)data;

  struct stat buf;
  if (!stat(tn->file.c_str(), &buf)) {
    // the file exists

    if (tn->note.id()) {
      time_t new_mtime = get_mtime(tn->file.c_str());
      if (new_mtime <= tn->mtime) {
        // the file was not modified
        unlink(tn->file.c_str());
        delete (tmp_note*)data;
        g_spawn_close_pid(pid);
        return;
      }
    }

    tn->note.load_from_file(tn->file);
    unlink(tn->file.c_str());

    if (tn->note.id()) {
      db->save_note(&tn->note);
      update_note_in_list(tn->note);
      show_all_tags_in_list();
      select_tag_in_list(TAG_ALL);
      select_note_in_list(tn->note);
    } else if (!tn->note.value().empty()) {
      db->save_note(&tn->note);
      prepend_note_to_list(tn->note);
      show_all_tags_in_list();
      select_tag_in_list(TAG_ALL);
      select_note_in_list(tn->note);
    }
  }

  delete (tmp_note*)data;
  g_spawn_close_pid(pid);
}

void delete_note(const note& n) {
  delete_note_from_list(n);
  db->delete_note(n);

  note   note = selected_note();
  string tag  = selected_tag();

  show_all_tags_in_list();
  select_tag_in_list(tag);
  select_note_in_list(note);
}

gboolean on_search_entry_focus_in_event() {
  disable_hotkeys();
  return FALSE;
}

gboolean on_search_entry_focus_out_event() {
  enable_hotkeys();
  return FALSE;
}

void enable_hotkeys() {
  gtk_widget_add_accelerator(GTK_WIDGET(add_button),
                             "clicked",
                             accel_group,
                             'a',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(GTK_WIDGET(edit_button),
                             "clicked",
                             accel_group,
                             'e',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(GTK_WIDGET(refresh_button),
                             "clicked",
                             accel_group,
                             'r',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(GTK_WIDGET(preferences_button),
                             "clicked",
                             accel_group,
                             'p',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(GTK_WIDGET(search_entry),
                             "grab-focus",
                             accel_group,
                             '/',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(GTK_WIDGET(note_view),
                             "select-previous",
                             accel_group,
                             'k',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator(GTK_WIDGET(note_view),
                             "select-next",
                             accel_group,
                             'j',
                             (GdkModifierType)0,
                             GTK_ACCEL_VISIBLE);
}

void disable_hotkeys() {
  gtk_widget_remove_accelerator(GTK_WIDGET(add_button),
                                accel_group,
                                'a',
                                (GdkModifierType)0);
  gtk_widget_remove_accelerator(GTK_WIDGET(edit_button),
                                accel_group,
                                'e',
                                (GdkModifierType)0);
  gtk_widget_remove_accelerator(GTK_WIDGET(refresh_button),
                                accel_group,
                                'r',
                                (GdkModifierType)0);
  gtk_widget_remove_accelerator(GTK_WIDGET(preferences_button),
                                accel_group,
                                'p',
                                (GdkModifierType)0);
  gtk_widget_remove_accelerator(GTK_WIDGET(search_entry),
                                accel_group,
                                '/',
                                (GdkModifierType)0);
  gtk_widget_remove_accelerator(GTK_WIDGET(note_view),
                                accel_group,
                                'j',
                                (GdkModifierType)0);
  gtk_widget_remove_accelerator(GTK_WIDGET(note_view),
                                accel_group,
                                'k',
                                (GdkModifierType)0);
}

gboolean on_tag_event(GtkTextTag *tag,
                      __attribute__((unused))GObject *object,
                      GdkEvent *event,
                      GtkTextIter *iter) {
  switch (event->type) {
    case GDK_BUTTON_RELEASE:
      click_tag(iter, tag);
      break;
    default:
      break;
  }

  return FALSE;
}

void click_tag(const GtkTextIter *iter, GtkTextTag *tag) {
  string text = get_tag_text(iter, tag);

  if (tag == tag_tag) {
    select_tag_in_list(text);
  } else if (tag == link_tag) {
    GError *error = NULL;
    if (!gtk_show_uri(NULL, text.c_str(), gtk_get_current_event_time(), &error)) {
      g_warning("could not open link \"%s\" -- %s", text.c_str(), error->message);
    }
  } else if (tag == link_default_http_tag) {
    GError *error = NULL;
    text = HTTP_SCHEME+text;
    if (!gtk_show_uri(NULL, text.c_str(), gtk_get_current_event_time(), &error)) {
      g_warning("could not open link \"%s\" -- %s", text.c_str(), error->message);
    }
  } else if (tag == email_tag) {
    GError *error = NULL;
    text = MAILTO_SCHEME+text;
    if (!gtk_show_uri(NULL, text.c_str(), gtk_get_current_event_time(), &error)) {
      g_warning("could not open email \"%s\" -- %s", text.c_str(), error->message);
    }
  } else if (tag == twitter_tag) {
    GError *error = NULL;
    text = TWITTER_URL+text.substr(1);
    if (!gtk_show_uri(NULL, text.c_str(), gtk_get_current_event_time(), &error)) {
      g_warning("could not open email \"%s\" -- %s", text.c_str(), error->message);
    }
  }
}

string get_tag_text(const GtkTextIter *iter, GtkTextTag *tag) {
  if (gtk_text_iter_toggles_tag(iter, tag)) {
    return "";
  }

  GtkTextIter begin = *iter;
  GtkTextIter end   = *iter;

  while (gtk_text_iter_begins_tag(&begin, tag) == false) {
    gtk_text_iter_backward_char(&begin);
  }

  while (gtk_text_iter_ends_tag(&end, tag) == false) {
    gtk_text_iter_forward_char(&end);
  }

  return gtk_text_iter_get_text(&begin, &end);
}

gboolean on_text_view_motion_notify_event(GtkWidget *text_view, GdkEventMotion *event) {
  gint x = 0;
  gint y = 0;
  GtkTextIter iter;
  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view),
                                        GTK_TEXT_WINDOW_WIDGET,
                                        event->x,
                                        event->y,
                                        &x,
                                        &y);
  gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text_view), &iter, x, y);
  gdk_window_get_pointer(event->window, 0, 0, 0);
  GdkCursor *cursor = gdk_cursor_new(GDK_XTERM);
  if (gtk_text_iter_has_tag(&iter, tag_tag)) {
    cursor = gdk_cursor_new(GDK_HAND2);
  } else if (gtk_text_iter_has_tag(&iter, link_tag)) {
    cursor = gdk_cursor_new(GDK_HAND2);
  } else if (gtk_text_iter_has_tag(&iter, link_default_http_tag)) {
    cursor = gdk_cursor_new(GDK_HAND2);
  } else if (gtk_text_iter_has_tag(&iter, email_tag)) {
    cursor = gdk_cursor_new(GDK_HAND2);
  } else if (gtk_text_iter_has_tag(&iter, twitter_tag)) {
    cursor = gdk_cursor_new(GDK_HAND2);
  }
  gdk_window_set_cursor(event->window, cursor);
  return FALSE;
}

// vim: textwidth=80:wrap:expandtab:tabstop=2:formatoptions=croqlt:shiftwidth=2
