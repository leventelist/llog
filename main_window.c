/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2024  Levente Kovacs
 *
 *	This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * http://levente.logonex.eu
 * ha5ogl.levente@gmail.com
 */

#include <gtk/gtk.h>
#include <inttypes.h>
#include "main_window.h"
#include "llog.h"
#include "llog_Config.h"

#include "preferences_window.h"

#define LLOG_COLUMNS 16
#define LLOG_ENTRIES 32
#define BUFF_SIZ 1024


enum {
  llog_list_id = 0,
  llog_list_call,
  llog_list_date,
  llog_list_qrg,
  llog_list_mode,
  llog_list_utc
};


char entry_labels[LLOG_ENTRIES][20] = {
  "Call",
  "Date",
  "UTC",
  "RX RST",
  "TX RST",
  "QTH",
  "Name",
  "QRA",
  "QRG",
  "Mode",
  "Power",
  "RX NR",
  "TX NR",
  "RX Extra",
  "TX Extra",
  "Comment",
  "Summit ref",
  "Station"
};

typedef struct {
  GtkWidget *main_window;
  GtkWidget *vertical_box;
  GtkWidget *horizontal_box;
  GtkWidget *w_txtvw_main;              // Pointer to text view object
  GtkWidget *logfile_choose;            // Pointer to file chooser dialog box
  GtkWidget *log_entry_list;            // Pointer to the log entry list

  /*Logged list store*/
  GtkWidget *logged_column_view;                     // Pointer to the logged elemnt list
  GListStore *logged_list_store;
  GtkTreeSelection *logged_list_selection;
//  GtkTreeViewColumn *logged_list_column[LLOG_COLUMNS];
//  GtkCellRenderer *logged_list_renderer[LLOG_COLUMNS];

  /*Station list store*/
  GListStore *station_list_store;
  GListStore *mode_list_store;
  GtkWidget *log_entries[LLOG_ENTRIES];
  GtkEntryBuffer *log_entry_buffers[LLOG_ENTRIES];
  GtkWidget *log_button;
  GtkComboBox *mode_entry;
  GtkComboBox *station_entry;
  GtkWidget *call_label;
  GtkWidget *about_dialog;
} app_widgets_t;


/*String objects*/

typedef struct {
  GObject parent_instance;
  gchar *value;
} StringObject;

typedef struct {
  GObjectClass parent_class;
} StringObjectClass;

G_DEFINE_TYPE(StringObject, string_object, G_TYPE_OBJECT)

static void string_object_init(StringObject *self) {
  self->value = NULL;
}

static void string_object_finalize(GObject *object) {
  StringObject *self = (StringObject *)object;

  g_free(self->value);
  G_OBJECT_CLASS(string_object_parent_class)->finalize(object);
}

static void string_object_class_init(StringObjectClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = string_object_finalize;
}

StringObject *string_object_new(const gchar *str) {
  StringObject *self = g_object_new(string_object_get_type(), NULL);

  self->value = g_strdup(str);
  return self;
}


/*Mode entry*/

#define MODE_ENTRY_TYPE (mode_entry_get_type())
G_DECLARE_FINAL_TYPE(ModeEntry, mode_entry, MODEENTRY, ITEM, GObject)

struct _ModeEntry {
  GObject parent_instance;
  char *id;
  char *name;
  char *default_rst;
  char *comment;
};

struct _ModeEntryClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(ModeEntry, mode_entry, G_TYPE_OBJECT)

static void mode_entry_init(ModeEntry *self) {
  self->id = NULL;
  self->name = NULL;
  self->default_rst = NULL;
  self->comment = NULL;
}

static void mode_entry_finalize(GObject *object) {
  ModeEntry *self = (ModeEntry *)object;

  g_free(self->name);   // Free the dynamically allocated name
  g_free(self->default_rst);   // Free the dynamically allocated name
  g_free(self->comment);   // Free the dynamically allocated name
  g_free(self->id);   // Free the dynamically allocated name
  G_OBJECT_CLASS(mode_entry_parent_class)->finalize(object);   // Chain up
}

static void mode_entry_class_init(ModeEntryClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = mode_entry_finalize;   // Override finalize
}

static ModeEntry *mode_entry_new(mode_entry_t *entry) {
  ModeEntry *self = g_object_new(MODE_ENTRY_TYPE, NULL);

  self->id = g_strdup_printf("%" PRIu64, entry->id);
  self->name = g_strdup(entry->name);
  self->default_rst = g_strdup(entry->default_rst);
  self->comment = g_strdup(entry->comment);
  return self;
}

static char *mode_entry_get_name(ModeEntry *item) {
  return item->name;
}

static void bind_mode_dropdown_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = mode_entry_get_name(MODEENTRY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}



/*Log entry display item*/

#define LOGENTRYDISPLAY_TYPE_ITEM (logentrydisplay_item_get_type())
G_DECLARE_FINAL_TYPE(LogEntryDisplayItem, logentrydisplay_item, LOGENTRYDISPLAY, ITEM, GObject)

struct _LogEntryDisplayItem {
  GObject parent_instance;
  char *id;
  char *call;
  char *qrg;
  char *date;
  char *utc;
  char *mode;
};

struct _LogEntryDisplayItemClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(LogEntryDisplayItem, logentrydisplay_item, G_TYPE_OBJECT)

static void logentrydisplay_item_init(LogEntryDisplayItem *self) {
  self->id = NULL;
  self->call = NULL;
  self->qrg = NULL;
  self->date = NULL;
  self->utc = NULL;
  self->mode = NULL;
}

static void logentrydisplay_item_finalize(GObject *object) {
  LogEntryDisplayItem *self = (LogEntryDisplayItem *)object;

  g_free(self->call);   // Free the dynamically allocated name
  g_free(self->date);   // Free the dynamically allocated name
  g_free(self->utc);    // Free the dynamically allocated name
  g_free(self->id);     // Free the dynamically allocated name
  g_free(self->qrg);    // Free the dynamically allocated name
  g_free(self->mode);   // Free the dynamically allocated name
  G_OBJECT_CLASS(logentrydisplay_item_parent_class)->finalize(object);   // Chain up
}

// Class initialization function
static void logentrydisplay_item_class_init(LogEntryDisplayItemClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = logentrydisplay_item_finalize;   // Override finalize
}

static LogEntryDisplayItem *logentrydisplay_new(log_entry_t *entry) {
  LogEntryDisplayItem *self = g_object_new(LOGENTRYDISPLAY_TYPE_ITEM, NULL);

  self->id = g_strdup_printf("%" PRIu64, entry->id);
  self->call = g_strdup(entry->call);
  self->qrg = g_strdup_printf("%.2f", entry->qrg);
  self->date = g_strdup(entry->date);
  self->utc = g_strdup(entry->utc);
  self->mode = g_strdup(entry->mode.name);
  return self;
}

static void setup_cb(GtkSignalListItemFactory *factory, GObject  *listitem) {
  (void)factory;
  GtkWidget *label = gtk_label_new(NULL);

  gtk_list_item_set_child(GTK_LIST_ITEM(listitem), label);
}

static const char *logentry_item_get_call(LogEntryDisplayItem *item) {
  return item->call;
}

static const char *logentry_item_get_id(LogEntryDisplayItem *item) {
  return item->id;
}

static const char *logentry_item_get_qrg(LogEntryDisplayItem *item) {
  return item->qrg;
}

static const char *logentry_item_get_date(LogEntryDisplayItem *item) {
  return item->date;
}

static const char *logentry_item_get_utc(LogEntryDisplayItem *item) {
  return item->utc;
}

static const char *logentry_item_get_mode(LogEntryDisplayItem *item) {
  return item->mode;
}

static void bind_call_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = logentry_item_get_call(LOGENTRYDISPLAY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}

static void bind_id_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = logentry_item_get_id(LOGENTRYDISPLAY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}

static void bind_qrg_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = logentry_item_get_qrg(LOGENTRYDISPLAY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}

static void bind_date_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = logentry_item_get_date(LOGENTRYDISPLAY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}

static void bind_utc_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = logentry_item_get_utc(LOGENTRYDISPLAY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}

static void bind_mode_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = logentry_item_get_mode(LOGENTRYDISPLAY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
}

/*Module variables*/
static app_widgets_t *widgets;
static log_entry_t log_entry_data;
static GtkCssProvider *provider;
static llog_t *local_llog;

/*Callbacks*/
static void on_window_main_destroy(void);
static void on_utc_btn_clicked(void);
static void on_mode_entry_change(GtkEditable *entry, gpointer user_data);
static void on_window_main_entry_changed(GtkEditable *editable, gpointer user_data);
static void set_static_data(void);
static void on_activate(GtkApplication *app, gpointer user_data);
static void on_log_btn_clicked(void);
static void on_edit_preferences_activate(app_widgets_t *app_wdgts);
static void on_qrt_activate(void);
static void on_about_menu_activate(app_widgets_t *app_wdgts);
static void on_reload_activate(GMenuItem *menuitem, app_widgets_t *app_wdgts);
static void on_menuitm_open_activate(app_widgets_t *app_wdgts);
static void on_open_file_response(GtkDialog *dialog, gint response_id, gpointer user_data);

void main_window_set_llog(llog_t *llog) {
  local_llog = llog;
}

int main_window_draw(int argc, char *argv[]) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("com.example.GtkApplication", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}

static void on_activate(GtkApplication *app, gpointer user_data) {
  int entry_index;
  char buff[BUFF_SIZ];
  station_entry_t *initial_station;

  (void)user_data;

  widgets = g_slice_new(app_widgets_t);

  for (entry_index = 0; entry_index < LLOG_ENTRIES; entry_index++) {
    widgets->log_entry_buffers[entry_index] = NULL;
  }

  /*Build main window*/
  widgets->main_window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(widgets->main_window), "Llog");
  gtk_window_set_default_size(GTK_WINDOW(widgets->main_window), 400, 300);
  g_signal_connect(widgets->main_window, "destroy", G_CALLBACK(on_window_main_destroy), NULL);

  widgets->vertical_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
  widgets->horizontal_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_window_set_child(GTK_WINDOW(widgets->main_window), widgets->horizontal_box);

  GtkWidget *scrolled_window = gtk_scrolled_window_new();

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /*Build the logged list view*/
  widgets->logged_list_store = g_list_store_new(G_TYPE_OBJECT);

  GtkSingleSelection *selection;

  selection = gtk_single_selection_new(G_LIST_MODEL(widgets->logged_list_store));

  widgets->logged_column_view = gtk_column_view_new(GTK_SELECTION_MODEL(selection));

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), widgets->logged_column_view);

  GtkColumnViewColumn *column;
  GtkListItemFactory *factory;


  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_id_cb), NULL);
  column = gtk_column_view_column_new("Id", factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(widgets->logged_column_view), column);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);

  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_call_cb), NULL);
  column = gtk_column_view_column_new("Call", factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(widgets->logged_column_view), column);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);

  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_date_cb), NULL);
  column = gtk_column_view_column_new("Date", factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(widgets->logged_column_view), column);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);

  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_utc_cb), NULL);
  column = gtk_column_view_column_new("UTC", factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(widgets->logged_column_view), column);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);

  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_qrg_cb), NULL);
  column = gtk_column_view_column_new("QRG", factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(widgets->logged_column_view), column);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);

  factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
  g_signal_connect(factory, "bind", G_CALLBACK(bind_mode_cb), NULL);
  column = gtk_column_view_column_new("Mode", factory);
  gtk_column_view_append_column(GTK_COLUMN_VIEW(widgets->logged_column_view), column);
  gtk_column_view_column_set_resizable(column, TRUE);
  gtk_column_view_column_set_expand(column, TRUE);

  gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(widgets->logged_column_view), TRUE);
  gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(widgets->logged_column_view), TRUE);

  // Build the station list store
  widgets->station_list_store = g_list_store_new(G_TYPE_OBJECT);

  // Build the mode list store
  widgets->mode_list_store = g_list_store_new(G_TYPE_OBJECT);

  /*Log button*/
  widgets->log_button = gtk_button_new_with_label("Log");
  g_signal_connect(widgets->log_button, "clicked", G_CALLBACK(on_log_btn_clicked), NULL);

  /*Putting together the log list*/

  /*Build the log entry boxes*/
  GtkWidget *entry_grid = gtk_grid_new();
  GtkWidget *entry_widget;

  for (entry_index = llog_entry_call; entry_index <= llog_entry_station_id; entry_index++) {
    switch (entry_index) {
    case llog_entry_call:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_entry_new();
      widgets->call_label = entry_widget;
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_date:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_entry_new();
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_utc:
      entry_widget = gtk_button_new_with_label(entry_labels[entry_index]);
      g_signal_connect(entry_widget, "clicked", G_CALLBACK(on_utc_btn_clicked), NULL);
      widgets->log_entries[entry_index] = gtk_entry_new();
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_mode:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_drop_down_new(G_LIST_MODEL(widgets->mode_list_store), NULL);
      GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
      gtk_drop_down_set_factory(GTK_DROP_DOWN(widgets->log_entries[entry_index]), factory);
      g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
      g_signal_connect(factory, "bind", G_CALLBACK(bind_mode_dropdown_cb), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "notify::selected", G_CALLBACK(on_mode_entry_change), NULL);
      break;

    case llog_entry_station_id:
      entry_widget = gtk_label_new(entry_labels[entry_index]);

      widgets->log_entries[entry_index] = gtk_combo_box_new_with_entry();
      //gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(widgets->log_entries[entry_index]), 0);
      //gtk_combo_box_set_model(GTK_COMBO_BOX(widgets->log_entries[entry_index]), GTK_TREE_MODEL(widgets->station_list_store));
      //g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      break;

    default:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_entry_new();
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;
    }

    gtk_widget_set_size_request(widgets->log_entries[entry_index], 200, -1); // Set minimum width to 200 pixels
    gtk_widget_set_hexpand(widgets->log_entries[entry_index], FALSE);

    gtk_grid_attach(GTK_GRID(entry_grid), entry_widget, 0, entry_index, 1, 1);
    gtk_grid_attach(GTK_GRID(entry_grid), GTK_WIDGET(widgets->log_entries[entry_index]), 1, entry_index, 1, 1);
  }
  gtk_grid_attach(GTK_GRID(entry_grid), GTK_WIDGET(widgets->log_button), 0, llog_entry_station_id + 1, 2, 1);
  gtk_box_append(GTK_BOX(widgets->vertical_box), GTK_WIDGET(entry_grid));

  llog_get_initial_station(&initial_station);
  if (initial_station->name[0] != '\0') {
    GtkWidget *station_list_entry = gtk_combo_box_get_child(GTK_COMBO_BOX(widgets->log_entries[llog_entry_station_id]));
    GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(station_list_entry));
    sprintf(buff, "%s [%" PRIu64 "]", initial_station->name, initial_station->id);
    gtk_entry_buffer_insert_text(buffer, 0, buff, -1); // Some bug here
  }

  /*Menu*/
  GSimpleAction *act_quit = g_simple_action_new("quit", NULL);
  GSimpleAction *act_reload = g_simple_action_new("reload", NULL);
  GSimpleAction *act_open = g_simple_action_new("open", NULL);

  g_signal_connect(act_reload, "activate", G_CALLBACK(on_reload_activate), widgets);
  g_signal_connect_swapped(act_open, "activate", G_CALLBACK(on_menuitm_open_activate), widgets);
  g_signal_connect_swapped(act_quit, "activate", G_CALLBACK(g_application_quit), app); //Modify this to the QRT function
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_quit));
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_reload));
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_open));

  GMenu *menubar = g_menu_new();
  GMenu *file_menu = g_menu_new();
  GMenu *section1 = g_menu_new();
  GMenu *section3 = g_menu_new();

  GMenuItem *menu_item_open = g_menu_item_new("Open", "app.open");
  GMenuItem *menu_item_reload = g_menu_item_new("Reload", "app.reload");
  GMenuItem *menu_item_quit = g_menu_item_new("QRT", "app.quit");

  g_menu_append_item(section1, menu_item_open);
  g_menu_append_item(section1, menu_item_reload);
  g_menu_append_item(section3, menu_item_quit);
  g_object_unref(menu_item_open);
  g_object_unref(menu_item_reload);
  g_object_unref(menu_item_quit);

  g_menu_append_section(file_menu, NULL, G_MENU_MODEL(section1));
  g_menu_append_section(file_menu, NULL, G_MENU_MODEL(section3));
  g_menu_append_submenu(menubar, "File", G_MENU_MODEL(file_menu));

  g_object_unref(section1);
  g_object_unref(section3);
  g_object_unref(file_menu);

  /*Edit menu*/

  GMenu *edit_menu = g_menu_new();
  GMenu *edit_section = g_menu_new();
  GMenuItem *menu_item_edit_preferences = g_menu_item_new("Preferences", "app.preferences");
  GSimpleAction *act_preferences = g_simple_action_new("preferences", NULL);

  g_menu_append_item(edit_section, menu_item_edit_preferences);
  g_object_unref(menu_item_edit_preferences);
  g_menu_append_section(edit_menu, NULL, G_MENU_MODEL(edit_section));
  g_menu_append_submenu(menubar, "Edit", G_MENU_MODEL(edit_menu));
  g_object_unref(edit_section);
  g_object_unref(edit_menu);

  g_signal_connect_swapped(act_preferences, "activate", G_CALLBACK(on_edit_preferences_activate), widgets);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_preferences));


  /*Help menu*/
  GSimpleAction *act_about = g_simple_action_new("about", NULL);
  GMenu *help_menu = g_menu_new();
  GMenu *help_section = g_menu_new();
  GMenuItem *menu_item_about = g_menu_item_new("About", "app.about");

  g_menu_append_item(help_section, menu_item_about);
  g_object_unref(menu_item_about);
  g_menu_append_section(help_menu, NULL, G_MENU_MODEL(help_section));
  g_menu_append_submenu(menubar, "Help", G_MENU_MODEL(help_menu));
  g_object_unref(help_section);
  g_object_unref(help_menu);

  g_signal_connect_swapped(act_about, "activate", G_CALLBACK(on_about_menu_activate), widgets);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_about));

  gtk_application_set_menubar(GTK_APPLICATION(app), G_MENU_MODEL(menubar));


  provider = gtk_css_provider_new();
  /* Initialize the css data */
  //gtk_css_provider_load_from_data(provider, "label.lb {background-color: red;}", -1);
  /* Add CSS to the default GdkDisplay. */
  gtk_style_context_add_provider_for_display(gdk_display_get_default(),
                                             GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_signal_connect(app, "shutdown", G_CALLBACK(on_qrt_activate), provider);
  g_object_unref(provider);  /* release provider, but it's still alive because the display owns it */


  // Build the window
  gtk_box_append(GTK_BOX(widgets->horizontal_box), GTK_WIDGET(widgets->vertical_box));
  gtk_box_append(GTK_BOX(widgets->horizontal_box), GTK_WIDGET(scrolled_window));

  gtk_widget_set_hexpand(widgets->logged_column_view, TRUE);
  gtk_widget_set_vexpand(widgets->logged_column_view, TRUE);

  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(widgets->main_window), TRUE);
  /*Let's rock!*/
  gtk_window_present(GTK_WINDOW(widgets->main_window));

  llog_load_static_data(&log_entry_data);
  set_static_data();


//  g_slice_free(app_widgets_t, widgets);

  return;
}


void set_static_data(void) {
  char buff[BUFF_SIZ];

  /*Set txnr*/
  snprintf(buff, BUFF_SIZ, "%04" PRIu64, log_entry_data.txnr);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_txnr], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_txnr], 0, buff, -1);
}


/*Main window callbacks*/

static void on_menuitm_open_activate(app_widgets_t *app_wdgts) {
  char *current_log_file_name;

  llog_get_log_file_path(&current_log_file_name);

  app_wdgts->logfile_choose = gtk_file_chooser_dialog_new("Open Log File",
                                                          GTK_WINDOW(app_wdgts->main_window),
                                                          GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
                                                          GTK_RESPONSE_CANCEL,
                                                          "_Open",
                                                          GTK_RESPONSE_OK,
                                                          NULL);

/*Add last loaded filename to the chooser*/
  if (current_log_file_name != NULL && current_log_file_name[0] != '\0') {
    GFile *file = g_file_new_for_path(current_log_file_name);
    gtk_file_chooser_set_file(GTK_FILE_CHOOSER(app_wdgts->logfile_choose), file, NULL);
  }

  g_signal_connect(app_wdgts->logfile_choose, "response", G_CALLBACK(on_open_file_response), app_wdgts);

  // Show the "Open Text File" dialog box
  gtk_window_present(GTK_WINDOW(app_wdgts->logfile_choose));
  return;
}

static void on_open_file_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
  int file_success;    // File read status

  (void)user_data;

  if (response_id == GTK_RESPONSE_OK) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    GFile *file = gtk_file_chooser_get_file(chooser);
    char *filename = g_file_get_path(file);
    if (filename != NULL) {
      llog_set_log_file(filename);
      file_success = llog_open_db();
      llog_load_static_data(&log_entry_data);
      set_static_data();
      llog_save_config_file();
      if (file_success != 0) {
        printf("Error opening database.\n");
        return;
      }
    }
    g_print("File selected: %s\n", filename);
    g_free(filename);
    g_object_unref(file);
  }
  gtk_window_destroy(GTK_WINDOW(dialog));
}


void on_window_main_entry_changed(GtkEditable *editable, gpointer user_data) {
  uint64_t entry_id;
  int ret;
  static bool entry_changed = false;
  int cursor_position;

  (void)user_data;

  if (entry_changed) {
    return;
  }

  entry_changed = true;

  entry_id = 0xFFFFFFFF;

  GtkWidget *entry = GTK_WIDGET(editable);
  GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));

  /*See which entry box has changed*/
  for (entry_id = 0; entry_id < LLOG_ENTRIES; entry_id++) {
    if (widgets->log_entries[entry_id] == entry) {
      break;
    }
  }

  switch (entry_id) {
  case llog_entry_call:
    snprintf(log_entry_data.call, CALL_LEN, gtk_entry_buffer_get_text(buffer));
    llog_strupper(log_entry_data.call);
    cursor_position = gtk_editable_get_position(GTK_EDITABLE(entry));
    gtk_entry_buffer_delete_text(buffer, 0, -1);
    gtk_entry_buffer_insert_text(buffer, 0, log_entry_data.call, -1);
    gtk_editable_set_position(GTK_EDITABLE(entry), cursor_position);
    /*Get time*/
    on_utc_btn_clicked();
    /*Check for dup QSO*/
    ret = llog_check_dup_qso(&log_entry_data);
    switch (ret) {
    case OK:                     /*New QSO*/
      gtk_label_set_label(GTK_LABEL(widgets->call_label), "Call");
      break;

    case LLOG_DUP:                     /*DUP QSO*/
      gtk_label_set_label(GTK_LABEL(widgets->call_label), "Call [DUP]");
      break;

    default:                     /*ERROR*/
      break;
    }
    break;

  case llog_entry_mode:
    break;

  case llog_entry_qra:
    snprintf(log_entry_data.qra, QRA_LEN, gtk_entry_buffer_get_text(buffer));
    llog_strupper(log_entry_data.qra);
    cursor_position = gtk_editable_get_position(GTK_EDITABLE(entry));
    gtk_entry_buffer_delete_text(buffer, 0, -1);
    gtk_entry_buffer_insert_text(buffer, 0, log_entry_data.qra, -1);
    gtk_editable_set_position(GTK_EDITABLE(entry), cursor_position);

  default:
    break;
  }
  entry_changed = false;
}


void on_mode_entry_change(GtkEditable *entry, gpointer user_data) {
  (void)user_data;

  GtkDropDown *dropdown = GTK_DROP_DOWN(entry);
  GObject *selected_item = gtk_drop_down_get_selected_item(dropdown);

  if (selected_item != NULL) {
    ModeEntry *mode_entry = MODEENTRY_ITEM(selected_item);
    g_print("Selected mode: %s\n", mode_entry->name);
    llog_get_default_rst(log_entry_data.txrst, atoll(mode_entry->id));
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_txrst], 0, -1);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_rxrst], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_txrst], 0, log_entry_data.txrst, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_rxrst], 0, log_entry_data.txrst, -1);
  }
}


static void on_log_btn_clicked(void) {
  int ret;
  char buff[BUFF_SIZ];
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT; // This might be changed
  GtkWidget *error_dialog;

  /*Gather log data*/
  snprintf(log_entry_data.date, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_date]));
  snprintf(log_entry_data.utc, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_utc]));
  snprintf(log_entry_data.rxrst, RST_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_rxrst]));
  snprintf(log_entry_data.txrst, RST_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_txrst]));
  snprintf(log_entry_data.qth, QTH_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_qth]));
  snprintf(log_entry_data.name, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_name]));
  snprintf(log_entry_data.qra, QRA_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_qra]));
  log_entry_data.qrg = atof(gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_qrg]));


  //Todo: Get the mode from the dropdown


  snprintf(log_entry_data.power, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_power]));
  log_entry_data.rxnr = strtoul(gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_rxnr]), NULL, 10);
  log_entry_data.txnr = strtoul(gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_txnr]), NULL, 10);
  snprintf(log_entry_data.rxextra, X_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_rxextra]));
  snprintf(log_entry_data.txextra, X_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_txextra]));
  snprintf(log_entry_data.comment, X_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_comment]));

  //Todo: Get the station id from the combo box


  /*This is for debug. Print log data to stdout*/
  //llog_print_log_data(&log_entry_data);

  /*Sanity check of the data*/

  if (strlen(log_entry_data.call) < 2) {
    printf("Not logging. Call too short.\n");
    error_dialog = gtk_message_dialog_new(GTK_WINDOW(widgets->main_window), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Call must be longer then 2 characters.");
    g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_widget_show(error_dialog);
    //gtk_widget_destroy(error_dialog);
    return;
  }

  /*Write log entry to the DB */
  ret = llog_log_entry(&log_entry_data);

  switch (ret) {
  case OK:
    /* Increment the counter. */
    log_entry_data.txnr++;
    break;

  case LLOG_ERR:
    /*Display some error message.*/
    error_dialog = gtk_message_dialog_new(GTK_WINDOW(widgets->main_window), flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error logging the QSO!");
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), "Database is not ready.");
    g_signal_connect(error_dialog, "response", G_CALLBACK(gtk_window_destroy), NULL);
    gtk_widget_show(error_dialog);
    //gtk_widget_destroy(error_dialog);
    return;

  default:
    break;
  }

  llog_reset_entry(&log_entry_data);

  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_call], 0, log_entry_data.call, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_date], 0, log_entry_data.date, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_utc], 0, log_entry_data.utc, -1);

  //on_mode_entry_change(widgets->log_entry_buffers[llog_entry_mode]);

  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_qth], 0, log_entry_data.qth, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_name], 0, log_entry_data.name, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_qra], 0, log_entry_data.qra, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_power], 0, log_entry_data.power, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_rxextra], 0, log_entry_data.rxextra, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_comment], 0, log_entry_data.comment, -1);

  snprintf(buff, BUFF_SIZ, "%04" PRIu64, log_entry_data.txnr);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_txnr], 0, buff, -1);

  /*Refresh the log list*/
  llog_add_log_entries();
}


static void on_utc_btn_clicked(void) {
  llog_get_time(&log_entry_data);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_date], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_date], 0, log_entry_data.date, -1);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_utc], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_utc], 0, log_entry_data.utc, -1);
}


static void on_window_main_destroy(void) {
  //g_main_loop_quit();
}


static void on_qrt_activate(void) {
  //g_main_loop_quit();
}


static void on_reload_activate(GMenuItem *menuitem, app_widgets_t *app_wdgts) {
  (void)menuitem;
  (void)app_wdgts;

  llog_shutdown();
  llog_open_db();
  llog_load_static_data(&log_entry_data);
  set_static_data();
}


static void on_about_menu_activate(app_widgets_t *app_wdgts) {
  /*About window*/
  widgets->about_dialog = gtk_about_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(app_wdgts->about_dialog), GTK_WINDOW(app_wdgts->main_window));
  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), "Llog");
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), VERSION);
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), "A simple logging application.");
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), "https://example.com");
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), (const char *[]){ "Levente Kovacs", NULL });
  gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), GTK_LICENSE_GPL_3_0);
  gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), "llog");
  gtk_window_present(GTK_WINDOW(app_wdgts->about_dialog));
}

static void on_edit_preferences_activate(app_widgets_t *app_wdgts) {
  (void)app_wdgts;
  printf("Preferences activated\n");
  on_preferences_window_activate(NULL, local_llog);
}

/*Actions*/

void main_window_add_log_entry_to_list(log_entry_t *entry) {
  g_list_store_append(widgets->logged_list_store, G_OBJECT(logentrydisplay_new(entry)));
}


void main_window_add_station_entry_to_list(station_entry_t *station) {
  char buff[BUFF_SIZ];

  snprintf(buff, BUFF_SIZ, "%s [%" PRIu64 "]", station->name, station->id);

  g_list_store_append(widgets->station_list_store, string_object_new(buff));
}


void main_window_add_mode_entry_to_list(mode_entry_t *mode) {
  char buff[BUFF_SIZ];

  snprintf(buff, BUFF_SIZ, "%s [%" PRIu64 "]", mode->name, mode->id);

  g_list_store_append(widgets->mode_list_store, G_OBJECT(mode_entry_new(mode)));
}


void main_window_clear_log_list(void) {
  g_list_store_remove_all(widgets->logged_list_store);
}


void main_window_clear_station_list(void) {
  g_list_store_remove_all(widgets->station_list_store);
}


void main_window_clear_modes_list(void) {
  g_list_store_remove_all(widgets->mode_list_store);
}
