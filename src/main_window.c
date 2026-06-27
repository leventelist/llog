/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2026  Levente Kovacs
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
#include <unistd.h>
#include <gps.h>
#include <math.h>

#include "main_window.h"
#include "llog.h"
#include "llog_config.h"
#include "db_sqlite.h"
#include "position.h"
#include "export_window.h"
#include "preferences_window.h"
#include "xml_client.h"

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
  "S2S ref",
  "Station"
};

typedef struct {
  GtkWidget *main_window;
  GtkWidget *vertical_box;
  GtkWidget *horizontal_box;
  GtkWidget *w_txtvw_main;              // Pointer to text view object
  GtkWidget *logfile_choose;            // Pointer to file chooser dialog box
  GtkWidget *log_entry_list;            // Pointer to the log entry list
  GtkWidget *right_box;
  GtkWidget *status_box;

  /*Position data*/
  GtkWidget *qra_label;
  GtkWidget *alt_label;
  GtkWidget *fix_mode_label;
  GtkWidget *distance_label;
  GtkWidget *heading_label;
  GtkWidget *speed_label;
  GtkWidget *programme_name_label;
  GtkWidget *programme_ref_label;
  GtkWidget *spw_qra_label;

  /*Logged list store*/
  GtkWidget *logged_column_view;                     // Pointer to the logged elemnt list
  GListStore *logged_list_store;
  GtkTreeSelection *logged_list_selection;
  GtkWidget *search_entry;
  GtkWidget *programme_dropdown;
  GtkWidget *spw_ref_btn;       /* button label changes with programme selection */
  GtkWidget *spw2spw_ref_label;        /* label changes with programme selection */
  GtkFilterListModel *filtered_list_model;
  GtkCustomFilter *call_filter;

  /*Station list store*/
  GListStore *station_list_store;
  GListStore *mode_list_store;
  GtkWidget *log_entries[LLOG_ENTRIES];
  GtkEntryBuffer *log_entry_buffers[LLOG_ENTRIES];
  GtkWidget *log_button;
  GtkWidget *call_label;
  GtkWidget *about_dialog;
  GtkWidget *get_button;
  GtkSortListModel *sort_list_model;
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
  if (item == NULL || item->name == NULL) {
    return g_strdup("");
  }
  return g_strdup(item->name);
}

static void bind_mode_dropdown_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  char *string = mode_entry_get_name(MODEENTRY_ITEM(item));

  gtk_label_set_text(GTK_LABEL(label), string);
  g_free(string);
}


/*Station entry*/

#define STATION_ENTRY_TYPE (station_entry_get_type())
G_DECLARE_FINAL_TYPE(StationEntry, station_entry, STATIONENTRY, ITEM, GObject)

struct _StationEntry {
  GObject parent_instance;
  char *id;
  char *name;
};

struct _StationEntryClass {
  GObjectClass parent_class;
};

G_DEFINE_TYPE(StationEntry, station_entry, G_TYPE_OBJECT)

static void station_entry_init(StationEntry *self) {
  self->id = NULL;
  self->name = NULL;
}

static void station_entry_finalize(GObject *object) {
  StationEntry *self = (StationEntry *)object;

  g_free(self->name);   // Free the dynamically allocated name
  g_free(self->id);   // Free the dynamically allocated name
  G_OBJECT_CLASS(station_entry_parent_class)->finalize(object);   // Chain up
}

static void station_entry_class_init(StationEntryClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->finalize = station_entry_finalize;   // Override finalize
}

static StationEntry *station_entry_new(station_entry_t *entry) {
  StationEntry *self = g_object_new(STATION_ENTRY_TYPE, NULL);

  self->id = g_strdup_printf("%" PRIu64, entry->id);
  self->name = g_strdup(entry->name);
  return self;
}

static char *station_entry_get_name(StationEntry *item) {
  if (item == NULL) {
    return "NULL";
  }
  return item->name;
}

static char *station_entry_get_id(StationEntry *item) {
  if (item == NULL) {
    return "NULL";
  }
  return item->id;
}

static void bind_station_dropdown_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem) {
  (void)factory;
  GtkWidget *label = gtk_list_item_get_child(listitem);
  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(listitem));
  const char *string = station_entry_get_name(STATIONENTRY_ITEM(item));

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

  double qrg;

  qrg = floor(entry->qrg * 1000) / 1000.0;

  self->id = g_strdup_printf("%" PRIu64, entry->id);
  self->call = g_strdup(entry->call);
  self->qrg = g_strdup_printf("%.3f", qrg);
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
static app_widgets_t *widgets = NULL;
static log_entry_t log_entry_data;
static llog_t *local_llog;
static const char *ref_labels[] = { "Summit ref", "Park ref",  "WWFF ref" };
static const char *x2x_labels[] = { "S2S ref",    "P2P ref",   "W2W ref"  };

/*Callbacks*/
static void on_call_btn_clicked(void);
static void on_utc_btn_clicked(void);
static void on_spw_ref_btn_clicked(void);
static void on_mode_entry_change(GtkWidget *entry, gpointer user_data);
static void on_window_main_entry_changed(GtkEditable *editable, gpointer user_data);
static void set_static_data(void);
static void on_activate(GtkApplication *app, gpointer user_data);
static void on_log_btn_clicked(void);
static void on_get_btn_clicked(void);
static void on_edit_preferences_activate(app_widgets_t *app_wdgts);
static void on_edit_log_db_activate(app_widgets_t *app_wdgts);
static void on_menuitm_new_activate(app_widgets_t *app_wdgts);
static void on_qrt_activate(GtkApplication *app);
static void on_about_menu_activate(app_widgets_t *app_wdgts);
static void on_reload_activate(GMenuItem *menuitem, app_widgets_t *app_wdgts);
static void on_menuitm_open_activate(app_widgets_t *app_wdgts);
static void save_response_cb(GObject *source, GAsyncResult *result, gpointer user_data);
static void on_station_entry_change(GtkEditable *entry, gpointer user_data);
static void on_export_activate(app_widgets_t *app_wdgts);
static void on_insert_text_uppercase(GtkEditable *editable, const gchar *text, int length, int *position, gpointer user_data);
static gboolean on_close_request(GtkWindow *window, gpointer user_data);
static void on_rebuild_aux_db_activate(app_widgets_t *app_wdgts);
static int filter_by_call(void *item, gpointer user_data);
static void on_search_entry_changed(GtkSearchEntry *entry, gpointer user_data);
static void on_programme_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data);



static int filter_by_call(void *item, gpointer user_data) {
  (void)user_data;
  GtkSearchEntry *se = GTK_SEARCH_ENTRY(widgets->search_entry);
  const char *needle = gtk_editable_get_text(GTK_EDITABLE(se));

  if (needle == NULL || needle[0] == '\0')
    return 1;

  LogEntryDisplayItem *entry = LOGENTRYDISPLAY_ITEM(item);
  if (entry->call == NULL)
    return 0;

  /* Case-insensitive substring match */
  char *call_lower  = g_ascii_strdown(entry->call, -1);
  char *needle_lower = g_ascii_strdown(needle, -1);
  gboolean match = strstr(call_lower, needle_lower) != NULL;
  g_free(call_lower);
  g_free(needle_lower);
  return match;
}

static void on_search_entry_changed(GtkSearchEntry *entry, gpointer user_data) {
  (void)entry;
  (void)user_data;
  gtk_filter_changed(GTK_FILTER(widgets->call_filter), GTK_FILTER_CHANGE_DIFFERENT);
}

static void on_programme_changed(GtkDropDown *dropdown, GParamSpec *pspec, gpointer user_data) {
  (void)pspec;
  (void)user_data;

  guint index = gtk_drop_down_get_selected(dropdown);
  if (index >= G_N_ELEMENTS(ref_labels))
    return;

  gtk_button_set_label(GTK_BUTTON(widgets->spw_ref_btn), ref_labels[index]);
  gtk_label_set_text(GTK_LABEL(widgets->spw2spw_ref_label), x2x_labels[index]);
  gtk_label_set_text(GTK_LABEL(widgets->programme_name_label), ref_labels[index]);

  /*Clear the text in the entry fields*/

  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_spw2spw_ref], 0, -1);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_spw_ref], 0, -1);

  g_print("Programme selected: %s\n", ref_labels[index]);
  local_llog->programme_id = index;
  strcpy(local_llog->programme_label, programme_config_labels[index]);
  llog_save_config_file();
}


void main_window_set_llog(llog_t *llog) {
  local_llog = llog;
}

int main_window_draw(int argc, char *argv[]) {
  GtkApplication *app;
  int status;
  (void)argc;
  (void)argv;

  app = gtk_application_new(NULL, G_APPLICATION_NON_UNIQUE);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  status = g_application_run(G_APPLICATION(app), 0, NULL);
  g_object_unref(app);

  return status;
}

void main_window_update_txnr(void) {
  char buff[BUFF_SIZ];

  db_get_max_nr(local_llog, &log_entry_data, log_entry_data.qrg);
  snprintf(buff, BUFF_SIZ, "%04" PRIu64, log_entry_data.txnr);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_txnr], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_txnr], 0, buff, -1);
}

/*Splash screen support*/

typedef struct {
  GtkApplication *app;
  GtkWidget      *splash;
} InitData;

static GtkWidget *build_splash(GtkApplication *app) {
  GtkWidget *win = gtk_application_window_new(app);

  gtk_window_set_title(GTK_WINDOW(win), PROGRAM_NAME);
  gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
  gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
  gtk_window_set_default_size(GTK_WINDOW(win), 300, 120);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_top(box, 24);
  gtk_widget_set_margin_bottom(box, 24);
  gtk_widget_set_margin_start(box, 24);
  gtk_widget_set_margin_end(box, 24);

  GtkWidget *title_label = gtk_label_new(PROGRAM_NAME);
  PangoAttrList *attrs = pango_attr_list_new();
  pango_attr_list_insert(attrs, pango_attr_scale_new(1.5));
  pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
  gtk_label_set_attributes(GTK_LABEL(title_label), attrs);
  pango_attr_list_unref(attrs);

  GtkWidget *spinner = gtk_spinner_new();
  gtk_spinner_start(GTK_SPINNER(spinner));

  GtkWidget *status_label = gtk_label_new("Generating static data, please wait…");

  gtk_box_append(GTK_BOX(box), title_label);
  gtk_box_append(GTK_BOX(box), spinner);
  gtk_box_append(GTK_BOX(box), status_label);
  gtk_window_set_child(GTK_WINDOW(win), box);

  return win;
}

static gboolean init_done_cb(gpointer user_data) {
  InitData *data = user_data;

  /*set_static_data touches GTK widgets — must run on the main thread*/
  set_static_data();

  /*Close splash and present the already-built (but hidden) main window*/
  gtk_window_destroy(GTK_WINDOW(data->splash));
  gtk_window_present(GTK_WINDOW(widgets->main_window));
  llog_load_static_data(&log_entry_data);
  set_static_data();

  gtk_drop_down_set_selected(GTK_DROP_DOWN(widgets->programme_dropdown), local_llog->programme_id);

  g_free(data);
  return G_SOURCE_REMOVE;
}

static gpointer init_thread_func(gpointer user_data) {
  /*This runs off the main thread — no GTK calls allowed here*/
  llog_init();

  /*Schedule the UI update back on the main thread*/
  g_idle_add(init_done_cb, user_data);
  return NULL;
}

static void on_activate(GtkApplication *app, gpointer user_data) {
  int entry_index;
  station_entry_t *initial_station;

  (void)user_data;

  widgets = g_slice_new(app_widgets_t);

  for (entry_index = 0; entry_index < LLOG_ENTRIES; entry_index++) {
    widgets->log_entry_buffers[entry_index] = NULL;
  }

  /*Build main window*/
  widgets->main_window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(widgets->main_window), PROGRAM_NAME);
  gtk_window_set_default_size(GTK_WINDOW(widgets->main_window), 400, 300);
  g_signal_connect(widgets->main_window, "close-request", G_CALLBACK(on_close_request), app);

  widgets->vertical_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
  widgets->horizontal_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_window_set_child(GTK_WINDOW(widgets->main_window), widgets->horizontal_box);

  GtkWidget *scrolled_window = gtk_scrolled_window_new();

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  /*Build the logged list view*/
  widgets->logged_list_store = g_list_store_new(G_TYPE_OBJECT);

  GtkSingleSelection *selection;

  widgets->call_filter = gtk_custom_filter_new(filter_by_call, NULL, NULL);
  widgets->filtered_list_model = gtk_filter_list_model_new(
      G_LIST_MODEL(widgets->logged_list_store),
      GTK_FILTER(widgets->call_filter));

  selection = gtk_single_selection_new(G_LIST_MODEL(widgets->filtered_list_model));
  widgets->logged_column_view = gtk_column_view_new(GTK_SELECTION_MODEL(selection));
  g_object_unref(selection);

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
      entry_widget = gtk_button_new_with_label(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_entry_new();
      widgets->call_label = entry_widget;
      g_signal_connect(gtk_editable_get_delegate(GTK_EDITABLE(widgets->log_entries[entry_index])), "insert-text",
                   G_CALLBACK(on_insert_text_uppercase), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      g_signal_connect(entry_widget, "clicked", G_CALLBACK(on_call_btn_clicked), NULL);
      break;

    case llog_entry_date:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_entry_new();
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_utc:
      entry_widget = gtk_button_new_with_label(entry_labels[entry_index]);
      g_signal_connect(entry_widget, "clicked", G_CALLBACK(on_utc_btn_clicked), NULL);
      widgets->log_entries[entry_index] = gtk_entry_new();
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_mode:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_drop_down_new(G_LIST_MODEL(widgets->mode_list_store), NULL);
      gtk_drop_down_set_enable_search(GTK_DROP_DOWN(widgets->log_entries[entry_index]), TRUE);
      GtkExpression *expr = gtk_cclosure_expression_new(G_TYPE_STRING,                          // return type
                                                        NULL,                                   // no extra marshal needed
                                                        0, NULL,                                // no extra params
                                                        G_CALLBACK(mode_entry_get_name),        // your existing getter
                                                        NULL, NULL);
      gtk_drop_down_set_expression(GTK_DROP_DOWN(widgets->log_entries[entry_index]), expr);
      gtk_expression_unref(expr);
      factory = gtk_signal_list_item_factory_new();
      gtk_drop_down_set_factory(GTK_DROP_DOWN(widgets->log_entries[entry_index]), factory);
      g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
      g_signal_connect(factory, "bind", G_CALLBACK(bind_mode_dropdown_cb), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "notify::selected", G_CALLBACK(on_mode_entry_change), NULL);
      break;

    case llog_entry_station_id:
      entry_widget = gtk_label_new(entry_labels[entry_index]);

      widgets->log_entries[entry_index] = gtk_drop_down_new(G_LIST_MODEL(widgets->station_list_store), NULL);
      factory = gtk_signal_list_item_factory_new();
      gtk_drop_down_set_factory(GTK_DROP_DOWN(widgets->log_entries[entry_index]), factory);
      g_signal_connect(factory, "setup", G_CALLBACK(setup_cb), NULL);
      g_signal_connect(factory, "bind", G_CALLBACK(bind_station_dropdown_cb), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "notify::selected", G_CALLBACK(on_station_entry_change), NULL);
      break;

    case llog_entry_spw_ref:
      entry_widget = gtk_button_new_with_label(entry_labels[entry_index]);
      widgets->spw_ref_btn = entry_widget;
      g_signal_connect(entry_widget, "clicked", G_CALLBACK(on_spw_ref_btn_clicked), NULL);
      widgets->log_entries[entry_index] = gtk_entry_new();
      g_signal_connect(gtk_editable_get_delegate(GTK_EDITABLE(widgets->log_entries[entry_index])), "insert-text",
                   G_CALLBACK(on_insert_text_uppercase), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_spw2spw_ref:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->spw2spw_ref_label = entry_widget;
      widgets->log_entries[entry_index] = gtk_entry_new();
      g_signal_connect(gtk_editable_get_delegate(GTK_EDITABLE(widgets->log_entries[entry_index])), "insert-text",
                   G_CALLBACK(on_insert_text_uppercase), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
      break;

    case llog_entry_qra:
      entry_widget = gtk_label_new(entry_labels[entry_index]);
      widgets->log_entries[entry_index] = gtk_entry_new();
      g_signal_connect(gtk_editable_get_delegate(GTK_EDITABLE(widgets->log_entries[entry_index])), "insert-text",
                   G_CALLBACK(on_insert_text_uppercase), NULL);
      g_signal_connect(widgets->log_entries[entry_index], "changed", G_CALLBACK(on_window_main_entry_changed), NULL);
      widgets->log_entry_buffers[entry_index] = gtk_entry_get_buffer(GTK_ENTRY(widgets->log_entries[entry_index]));
      gtk_editable_set_editable(GTK_EDITABLE(widgets->log_entries[entry_index]), true);
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
  gtk_grid_attach(GTK_GRID(entry_grid), GTK_WIDGET(widgets->log_button), 1, llog_entry_station_id + 1, 1, 1);

  widgets->get_button = gtk_button_new_with_label("Get");
  g_signal_connect(widgets->get_button, "clicked", G_CALLBACK(on_get_btn_clicked), NULL);
  gtk_grid_attach(GTK_GRID(entry_grid), widgets->get_button, 0, llog_entry_station_id + 1, 1, 1);

  gtk_box_append(GTK_BOX(widgets->vertical_box), GTK_WIDGET(entry_grid));

  llog_get_initial_station(&initial_station);
  if (initial_station->name[0] != '\0') {
    GListModel *model = G_LIST_MODEL(widgets->station_list_store);
    for (guint i = 0; i < g_list_model_get_n_items(model); i++) {
      GObject *item = g_list_model_get_item(model, i);
      StationEntry *station_entry = STATIONENTRY_ITEM(item);
      if (station_entry != NULL) {
        g_print("Station: %s\n", station_entry->name);
      }
      g_object_unref(item);
    }
  }

  /*Menu*/
  GSimpleAction *act_quit = g_simple_action_new("quit", NULL);
  GSimpleAction *act_reload = g_simple_action_new("reload", NULL);
  GSimpleAction *act_open = g_simple_action_new("open", NULL);
  GSimpleAction *act_new = g_simple_action_new("new", NULL);
  GSimpleAction *act_adif_export = g_simple_action_new("export", NULL);

  g_signal_connect(act_reload, "activate", G_CALLBACK(on_reload_activate), widgets);
  g_signal_connect_swapped(act_open, "activate", G_CALLBACK(on_menuitm_open_activate), widgets);
  g_signal_connect_swapped(act_new, "activate", G_CALLBACK(on_menuitm_new_activate), widgets);
  g_signal_connect_swapped(act_quit, "activate", G_CALLBACK(on_qrt_activate), app);
  g_signal_connect_swapped(act_adif_export, "activate", G_CALLBACK(on_export_activate), widgets);

  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_quit));
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_reload));
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_open));
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_new));
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_adif_export));

  GMenu *menubar = g_menu_new();
  GMenu *file_menu = g_menu_new();
  GMenu *section1 = g_menu_new();
  GMenu *section3 = g_menu_new();

  GMenuItem *menu_item_open = g_menu_item_new("Open", "app.open");
  GMenuItem *menu_item_new = g_menu_item_new("New", "app.new");
  GMenuItem *menu_item_reload = g_menu_item_new("Reload", "app.reload");
  GMenuItem *menu_item_adif_export = g_menu_item_new("Export", "app.export");
  GMenuItem *menu_item_quit = g_menu_item_new("QRT", "app.quit");

  g_menu_append_item(section1, menu_item_open);
  g_menu_append_item(section1, menu_item_new);
  g_menu_append_item(section1, menu_item_reload);
  g_menu_append_item(section1, menu_item_adif_export);
  g_menu_append_item(section3, menu_item_quit);
  g_object_unref(menu_item_open);
  g_object_unref(menu_item_new);
  g_object_unref(menu_item_reload);
  g_object_unref(menu_item_adif_export);
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
  g_object_unref(edit_menu);

  g_signal_connect_swapped(act_preferences, "activate", G_CALLBACK(on_edit_preferences_activate), widgets);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_preferences));

  GMenuItem *menu_item_edit_log_db = g_menu_item_new("Log database", "app.edit_log_db");
  GSimpleAction *act_edit_log_db = g_simple_action_new("edit_log_db", NULL);

  g_menu_append_item(edit_section, menu_item_edit_log_db);
  g_object_unref(menu_item_edit_log_db);

  GMenuItem *menu_item_rebuild_aux_db = g_menu_item_new("Rebuild aux database", "app.rebuild_aux_db");
  GSimpleAction *act_rebuild_aux_db = g_simple_action_new("rebuild_aux_db", NULL);

  g_menu_append_item(edit_section, menu_item_rebuild_aux_db);
  g_object_unref(menu_item_rebuild_aux_db);

  g_object_unref(edit_section);

  g_signal_connect_swapped(act_rebuild_aux_db, "activate", G_CALLBACK(on_rebuild_aux_db_activate), widgets);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_rebuild_aux_db));
  g_signal_connect_swapped(act_edit_log_db, "activate", G_CALLBACK(on_edit_log_db_activate), widgets);
  g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(act_edit_log_db));

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


  // Build the window
  gtk_box_append(GTK_BOX(widgets->horizontal_box), GTK_WIDGET(widgets->vertical_box));

  widgets->right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);

  widgets->search_entry = gtk_search_entry_new();
  gtk_search_entry_set_placeholder_text(GTK_SEARCH_ENTRY(widgets->search_entry), "Search callsign…");
  g_signal_connect(widgets->search_entry, "search-changed",
                   G_CALLBACK(on_search_entry_changed), NULL);
  gtk_widget_set_hexpand(widgets->search_entry, TRUE);

  GtkStringList *programme_list = gtk_string_list_new(
      (const char *[]){"SOTA", "POTA", "WWFF", NULL});
  widgets->programme_dropdown = gtk_drop_down_new(G_LIST_MODEL(programme_list), NULL);
  g_signal_connect(widgets->programme_dropdown, "notify::selected",
                   G_CALLBACK(on_programme_changed), NULL);

  GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(top_bar), GTK_WIDGET(widgets->search_entry));
  gtk_box_append(GTK_BOX(top_bar), GTK_WIDGET(widgets->programme_dropdown));

  gtk_box_append(GTK_BOX(widgets->right_box), GTK_WIDGET(top_bar));
  gtk_box_append(GTK_BOX(widgets->right_box), GTK_WIDGET(scrolled_window));


  gtk_box_append(GTK_BOX(widgets->horizontal_box), GTK_WIDGET(widgets->right_box));

  widgets->status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

  widgets->qra_label = gtk_label_new("------");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->qra_label));

  GtkWidget *label;

  label = gtk_label_new("ASL:");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(label));
  widgets->alt_label = gtk_label_new("---");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->alt_label));

  label = gtk_label_new("Speed:");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(label));
  widgets->speed_label = gtk_label_new("---");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->speed_label));

  label = gtk_label_new("Distance:");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(label));
  widgets->distance_label = gtk_label_new("---");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->distance_label));

  label = gtk_label_new("Heading:");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(label));
  widgets->heading_label = gtk_label_new("---");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->heading_label));

  widgets->fix_mode_label = gtk_label_new("No fix");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->fix_mode_label));

  widgets->programme_name_label = gtk_label_new(ref_labels[local_llog->programme_id]);
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->programme_name_label));
  widgets->programme_ref_label = gtk_label_new("---");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->programme_ref_label));

  label = gtk_label_new("@");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(label));
  widgets->spw_qra_label = gtk_label_new("---");
  gtk_box_append(GTK_BOX(widgets->status_box), GTK_WIDGET(widgets->spw_qra_label));

  gtk_box_append(GTK_BOX(widgets->right_box), widgets->status_box);

  gtk_widget_set_hexpand(widgets->logged_column_view, TRUE);
  gtk_widget_set_vexpand(widgets->logged_column_view, TRUE);

  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(widgets->main_window), TRUE);

  /*Show splash while the slow init runs in the background*/
  GtkWidget *splash = build_splash(app);
  gtk_window_present(GTK_WINDOW(splash));

  InitData *init_data = g_new0(InitData, 1);
  init_data->app    = app;
  init_data->splash = splash;

  GThread *t = g_thread_new("llog-init", init_thread_func, init_data);
  g_thread_unref(t);

  return;
}

void main_window_update_position_labels(position_t *position) {
  char buffer[BUFF_SIZ];
  spw_entry_t spw_entry;
  double distance;
  double heading;

  if (position == NULL || widgets == NULL) {
    return;
  }

  position_to_qra(position, buffer);
  gtk_label_set_text(GTK_LABEL(widgets->qra_label), buffer);

  snprintf(buffer, BUFF_SIZ, "%.2fm", position->alt);
  gtk_label_set_text(GTK_LABEL(widgets->alt_label), buffer);

  snprintf(buffer, BUFF_SIZ, "%.2fkm/h", 3.6 * position->speed);
  gtk_label_set_text(GTK_LABEL(widgets->speed_label), buffer);

  spw_entry.data_stat = db_data_init;
  while (spw_entry.data_stat != db_data_last) {
    int ret;
    switch (local_llog->programme_id)
    {
    case llog_sota:
      ret = db_get_sota_entry(local_llog, &spw_entry, position);
      break;
    case llog_pota:
      ret = db_get_pota_entry(local_llog, &spw_entry, position);
      break;
    case llog_wwff:
    ret = db_get_wwff_entry(local_llog, &spw_entry, position);
      break;
    default:
      ret = db_get_sota_entry(local_llog, &spw_entry, position);
      break;
    }

    if (ret == llog_stat_err) {
      return;
    }
  }

  switch (position->fix) { // Fix mode
    {
    case MODE_NOT_SEEN:
      gtk_label_set_text(GTK_LABEL(widgets->fix_mode_label), "Not seen");
      break;

    case MODE_NO_FIX:
      gtk_label_set_text(GTK_LABEL(widgets->fix_mode_label), "No fix");
      break;

    case MODE_2D:
      gtk_label_set_text(GTK_LABEL(widgets->fix_mode_label), "2D fix");
      break;

    case MODE_3D:
      gtk_label_set_text(GTK_LABEL(widgets->fix_mode_label), "3D fix");
      break;

    default:
      gtk_label_set_text(GTK_LABEL(widgets->fix_mode_label), "Unknown");
      break;
    }
  }

  position_distance_and_heading(position, &spw_entry.position, &distance, &heading);

  snprintf(buffer, BUFF_SIZ, "%.2fkm", distance / 1000);
  gtk_label_set_text(GTK_LABEL(widgets->distance_label), buffer);

  snprintf(buffer, BUFF_SIZ, "%.0f°", heading);
  gtk_label_set_text(GTK_LABEL(widgets->heading_label), buffer);

  if (spw_entry.id != 0) {
    snprintf(buffer, BUFF_SIZ, "%s", spw_entry.ref);
    gtk_label_set_text(GTK_LABEL(widgets->programme_ref_label), buffer);
    position_to_qra(&spw_entry.position, buffer);
    gtk_label_set_text(GTK_LABEL(widgets->spw_qra_label), buffer);
  } else {
    gtk_label_set_text(GTK_LABEL(widgets->programme_ref_label), "---");
    gtk_label_set_text(GTK_LABEL(widgets->spw_qra_label), "------");
  }
}


void main_window_clear_position_labels(void) {
  if (widgets == NULL) {
    return;
  }

  gtk_label_set_text(GTK_LABEL(widgets->alt_label), "---");
  gtk_label_set_text(GTK_LABEL(widgets->speed_label), "---");
  gtk_label_set_text(GTK_LABEL(widgets->distance_label), "---");
  gtk_label_set_text(GTK_LABEL(widgets->heading_label), "---");
  gtk_label_set_text(GTK_LABEL(widgets->fix_mode_label), "No fix");
  gtk_label_set_text(GTK_LABEL(widgets->programme_ref_label), "---");
  gtk_label_set_text(GTK_LABEL(widgets->qra_label), "------");
  gtk_label_set_text(GTK_LABEL(widgets->spw_qra_label), "---");
}

void set_static_data(void) {

  /*Set txnr*/
  main_window_update_txnr();

  on_mode_entry_change(widgets->log_entries[llog_entry_mode], NULL);
}


static void
open_response_cb (GObject *source,
                  GAsyncResult *result,
                  gpointer user_data) {
  (void)user_data;
  GtkFileDialog *dialog = GTK_FILE_DIALOG (source);
  GFile *file;
  GError *error = NULL;
  char *filename;


  printf("Open response callback\n");


  file = gtk_file_dialog_open_finish (dialog, result, &error);
  if (file) {
    int file_success;    // File read status
    filename = g_file_get_path(file);
    printf("File selected: %s\n", filename);
    if (filename != NULL) {
      llog_set_log_file(filename, true);
      file_success = llog_open_db();
      llog_load_static_data(&log_entry_data);
      set_static_data();
      llog_save_config_file();
      if (file_success != 0) {
        printf("Error opening database.\n");
        return;
      }
    }
      g_free(filename);
    }

  if (error)
    {
      GtkAlertDialog *alert;

      alert = gtk_alert_dialog_new ("Error loading file: \"%s\"", error->message);
      gtk_alert_dialog_show (alert, NULL);
      g_object_unref (alert);
      g_error_free (error);
    }
  g_object_unref (file);
}




/*Main window callbacks*/

static void on_menuitm_open_activate(app_widgets_t *app_wdgts) {
  char *current_log_file_name;

  llog_get_log_file_path(&current_log_file_name);

  GtkFileDialog *dialog;
  GFile *cwd;

  dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title (dialog, "Open file");
  if (current_log_file_name != NULL && current_log_file_name[0] != '\0') {
    cwd = g_file_new_for_path (current_log_file_name);
    fprintf(stderr, "File: %s\n", current_log_file_name);
  } else {
    cwd = g_file_new_for_path (".");
  }
  gtk_file_dialog_set_initial_folder(dialog, cwd);
  g_object_unref (cwd);
  gtk_file_dialog_open(dialog, GTK_WINDOW (app_wdgts->main_window), NULL, open_response_cb, app_wdgts);
  g_object_unref (dialog);
}


static void on_insert_text_uppercase(GtkEditable *editable, const gchar *text, int length, int *position, gpointer user_data) {
  (void)user_data;
  // Block ourselves to prevent recursion
  g_signal_handlers_block_by_func(editable, on_insert_text_uppercase, NULL);

  // Also block the changed handler to avoid it firing mid-insert
  g_signal_handlers_block_by_func(editable, on_window_main_entry_changed, NULL);

  gchar *upper = g_utf8_strup(text, length);
  gtk_editable_insert_text(editable, upper, -1, position);
  g_free(upper);

  g_signal_handlers_unblock_by_func(editable, on_window_main_entry_changed, NULL);
  g_signal_handlers_unblock_by_func(editable, on_insert_text_uppercase, NULL);

  // Stop the original lowercase insertion — must be last
  g_signal_stop_emission_by_name(editable, "insert-text");
}


static void on_window_main_entry_changed(GtkEditable *editable, gpointer user_data) {
  uint64_t entry_id;
  int ret;

  (void)user_data;


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
    snprintf(log_entry_data.call, CALL_LEN, "%s", gtk_entry_buffer_get_text(buffer));
    /*Get time*/
    on_utc_btn_clicked();
    /*Check for dup QSO*/
    ret = llog_check_dup_qso(&log_entry_data);
    switch (ret) {
    case llog_stat_ok:                     /*New QSO*/
      gtk_button_set_label(GTK_BUTTON(widgets->call_label), entry_labels[llog_entry_call]);
      break;

    case llog_stat_dup:                     /*DUP QSO*/
      gtk_button_set_label(GTK_BUTTON(widgets->call_label), "Call [DUP]");
      break;

    default:                     /*ERROR*/
      break;
    }
    break;

  case llog_entry_spw_ref:
    snprintf(log_entry_data.spw_ref, SPW_REF_LEN, "%s", gtk_entry_buffer_get_text(buffer));
    break;

  case llog_entry_spw2spw_ref:
    snprintf(log_entry_data.spw2spw_ref, SPW_REF_LEN, "%s", gtk_entry_buffer_get_text(buffer));
    break;

  case llog_entry_qra:
    snprintf(log_entry_data.qra, QRA_LEN, "%s", gtk_entry_buffer_get_text(buffer));
    break;
  case llog_entry_qrg:
    log_entry_data.qrg = atof(gtk_entry_buffer_get_text(buffer));
    main_window_update_txnr();
    break;
  default:
    break;
  }
}


static void on_mode_entry_change(GtkWidget *entry, gpointer user_data) {
  (void)user_data;

  GtkDropDown *dropdown = GTK_DROP_DOWN(entry);
  GObject *selected_item = gtk_drop_down_get_selected_item(dropdown);

  if (selected_item != NULL) {
    ModeEntry *mode_entry = MODEENTRY_ITEM(selected_item);
    if (mode_entry->id == NULL || mode_entry->id[0] == '\0') {
      return;
    }
    llog_get_default_rst(log_entry_data.txrst, atoll(mode_entry->id));
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_txrst], 0, -1);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_rxrst], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_txrst], 0, log_entry_data.txrst, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_rxrst], 0, log_entry_data.txrst, -1);
  }
}

static void on_station_entry_change(GtkEditable *entry, gpointer user_data) {
  (void)user_data;

  GtkDropDown *dropdown = GTK_DROP_DOWN(entry);
  GObject *selected_item = gtk_drop_down_get_selected_item(dropdown);

  if (selected_item != NULL) {
    StationEntry *station_entry = STATIONENTRY_ITEM(selected_item);
    g_print("Selected station: %s\n", station_entry->name);
  }
}


static void on_get_btn_clicked(void) {
  int ret;

  double qrg;
  char *buff;

  buff = malloc(BUFF_SIZ);

  ret = xml_client_fldigi_get_frequency(&qrg);

  if (ret == xml_client_stat_ok) {
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_qrg], 0, -1);
    gchar *qrg_str = g_strdup_printf("%.3f", qrg);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_qrg], 0, qrg_str, -1);
    g_free(qrg_str);
  } else {
    printf("Error getting frequency\n");
  }

  ret = xml_client_fldigi_get_call(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_call], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_call], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_call], 0, buff, -1);
    strcpy(log_entry_data.call, buff);
    int ret = llog_check_dup_qso(&log_entry_data);
    switch (ret) {
    case llog_stat_dup:
      gtk_button_set_label(GTK_BUTTON(widgets->call_label), "Call [DUP]");
      break;

    default:
      gtk_button_set_label(GTK_BUTTON(widgets->call_label), entry_labels[llog_entry_call]);
      break;
    }
    on_utc_btn_clicked();

    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_call], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting call\n");
  }

  ret = xml_client_fldigi_get_name(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_name], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_name], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_name], 0, buff, -1);
    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_name], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting name\n");
  }

  ret = xml_client_fldigi_get_rxrst(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_rxrst], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_rxrst], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_rxrst], 0, buff, -1);
    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_rxrst], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting rst\n");
  }

  ret = xml_client_fldigi_get_txrst(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_txrst], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_txrst], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_txrst], 0, buff, -1);
    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_txrst], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting rst\n");
  }

  ret = xml_client_fldigi_get_qth(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_qth], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_qth], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_qth], 0, buff, -1);
    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_qth], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting qth\n");
  }

  ret = xml_client_fldigi_get_qra(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_qra], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_qra], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_qra], 0, buff, -1);
    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_qra], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting qra\n");
  }

  ret = xml_client_fldigi_get_utc(buff);
  if (ret == xml_client_stat_ok) {
    g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_utc], on_window_main_entry_changed, NULL);
    gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_utc], 0, -1);
    gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_utc], 0, buff, -1);
    g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_utc], on_window_main_entry_changed, NULL);
  } else {
    printf("Error getting utc\n");
  }

  free(buff);
}


static void on_log_btn_clicked(void) {
  int ret;
  GObject *item;

  /*Gather log data*/
  snprintf(log_entry_data.date, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_date]));
  snprintf(log_entry_data.utc, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_utc]));
  snprintf(log_entry_data.rxrst, RST_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_rxrst]));
  snprintf(log_entry_data.txrst, RST_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_txrst]));
  snprintf(log_entry_data.qth, QTH_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_qth]));
  snprintf(log_entry_data.name, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_name]));
  snprintf(log_entry_data.qra, QRA_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_qra]));
  log_entry_data.qrg = atof(gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_qrg]));

  item = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(widgets->log_entries[llog_entry_mode]));

  strcpy(log_entry_data.mode.name, mode_entry_get_name(MODEENTRY_ITEM(item)));

  snprintf(log_entry_data.power, NAME_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_power]));
  log_entry_data.rxnr = strtoul(gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_rxnr]), NULL, 10);
  log_entry_data.txnr = strtoul(gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_txnr]), NULL, 10);
  snprintf(log_entry_data.rxextra, X_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_rxextra]));
  snprintf(log_entry_data.txextra, X_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_txextra]));
  snprintf(log_entry_data.comment, X_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_comment]));

  item = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(widgets->log_entries[llog_entry_station_id]));
  log_entry_data.station_id = strtoull(station_entry_get_id(STATIONENTRY_ITEM(item)), NULL, 0);

  snprintf(log_entry_data.spw_ref, SPW_REF_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_spw_ref]));
  snprintf(log_entry_data.spw2spw_ref, SPW_REF_LEN, gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_spw2spw_ref]));

  /*This is for debug. Print log data to stdout*/
  //llog_print_log_data(&log_entry_data);

  /*Sanity check of the data*/

  if (strlen(log_entry_data.call) < 2) {
    printf("Not logging. Call too short.\n");
    GtkAlertDialog *alert = gtk_alert_dialog_new("Call must be longer than 2 characters.");
    gtk_alert_dialog_show(alert, GTK_WINDOW(widgets->main_window));
    g_object_unref(alert);

    return;
  }

  /*Write log entry to the DB */
  ret = llog_log_entry(&log_entry_data);

  if (ret != llog_stat_ok) {
    /*Display some error message.*/
    GtkAlertDialog *alert = gtk_alert_dialog_new("Error logging the QSO!");
    gtk_alert_dialog_set_detail(alert, "Database is not ready.");
    gtk_alert_dialog_show(alert, GTK_WINDOW(widgets->main_window));
    g_object_unref(alert);
    return;
  }

  llog_reset_entry(&log_entry_data);

  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_call], 0, log_entry_data.call, -1);

  /*Clear the entry boxes*/
  for (uint64_t i = llog_entry_call; i <= llog_entry_station_id; i++) {
    switch (i) {
    case llog_entry_station_id:
      break;

    case llog_entry_mode:
      break;

    case llog_entry_power:
      break;

    case llog_entry_spw_ref:
      break;

    case llog_entry_qrg:
      break;

    default:
      gtk_entry_buffer_delete_text(widgets->log_entry_buffers[i], 0, -1);
      break;
    }
  }

  /*Refresh the log list*/
  llog_add_log_entries();
  set_static_data();
}


static void on_call_btn_clicked(void) {
  const char *call;

  call = gtk_entry_buffer_get_text(widgets->log_entry_buffers[llog_entry_call]);

  if (call != NULL && call[0] != '\0') {
    llog_open_qrz_url(call);
  }
}


static void on_utc_btn_clicked(void) {
  llog_get_time(&log_entry_data);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_date], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_date], 0, log_entry_data.date, -1);
  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_utc], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_utc], 0, log_entry_data.utc, -1);
}


static void on_spw_ref_btn_clicked(void) {
  /*Get text from the reference label*/

  g_signal_handlers_block_by_func(widgets->log_entries[llog_entry_spw_ref], on_window_main_entry_changed, NULL);

  const char *spw_ref = gtk_label_get_text(GTK_LABEL(widgets->programme_ref_label));

  gtk_entry_buffer_delete_text(widgets->log_entry_buffers[llog_entry_spw_ref], 0, -1);
  gtk_entry_buffer_insert_text(widgets->log_entry_buffers[llog_entry_spw_ref], 0, spw_ref, -1);

  g_signal_handlers_unblock_by_func(widgets->log_entries[llog_entry_spw_ref], on_window_main_entry_changed, NULL);
}


static void save_response_cb(GObject *source, GAsyncResult *result, gpointer user_data) {
  (void)user_data;
  GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
  GError *error = NULL;

  GFile *file = gtk_file_dialog_save_finish(dialog, result, &error);
  if (file) {
    char *filename = g_file_get_path(file);
    if (filename != NULL) {
      db_close(local_llog);
      g_print("File selected: %s\n", filename);
      llog_set_log_file(filename, false);
      db_create_from_schema(local_llog, LLOG_DB_PATH);
      llog_load_static_data(&log_entry_data);
      set_static_data();
      llog_save_config_file();
      g_free(filename);
    }
    g_object_unref(file);
  }

  if (error) {
    GtkAlertDialog *alert = gtk_alert_dialog_new("Error creating file: \"%s\"", error->message);
    gtk_alert_dialog_show(alert, GTK_WINDOW(widgets->main_window));
    g_object_unref(alert);
    g_error_free(error);
  }
}



static void on_menuitm_new_activate(app_widgets_t *app_wdgts) {
  (void)app_wdgts;

  char *current_log_file_name;

  llog_get_log_file_path(&current_log_file_name);

  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "New Log File");

  if (current_log_file_name != NULL && current_log_file_name[0] != '\0') {
    GFile *initial = g_file_new_for_path(current_log_file_name);
    gtk_file_dialog_set_initial_file(dialog, initial);
    g_object_unref(initial);
  }

  gtk_file_dialog_save(dialog, GTK_WINDOW(app_wdgts->main_window), NULL, save_response_cb, NULL);
  g_object_unref(dialog);

}

static void on_export_activate(app_widgets_t *app_wdgts) {
  (void)app_wdgts;

  on_exporter_window_activate(NULL, local_llog);
}


static void on_reload_activate(GMenuItem *menuitem, app_widgets_t *app_wdgts) {
  (void)menuitem;
  (void)app_wdgts;

  llog_shutdown();
  llog_open_db();
  xml_client_init(local_llog->xmlrpc_host, local_llog->xmlrpc_port);
  llog_load_static_data(&log_entry_data);
  set_static_data();
  position_init(local_llog->gpsd_host, local_llog->gpsd_port, main_window_update_position_labels);
}


static void on_about_menu_activate(app_widgets_t *app_wdgts) {
  /*About window*/
  widgets->about_dialog = gtk_about_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(app_wdgts->about_dialog), GTK_WINDOW(app_wdgts->main_window));
  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), PROGRAM_NAME);
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), VERSION);
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), "Logging program for SOTA.");
  gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), "https://github.com/leventelist/llog");
  gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), (const char *[]){ "Levente Kovacs", NULL });
  gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), GTK_LICENSE_GPL_3_0);
  gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(app_wdgts->about_dialog), PROGRAM_NAME);
  g_signal_connect(app_wdgts->about_dialog, "destroy", G_CALLBACK(gtk_window_destroy), NULL);
  gtk_window_present(GTK_WINDOW(app_wdgts->about_dialog));
}


static void on_edit_preferences_activate(app_widgets_t *app_wdgts) {
  (void)app_wdgts;

  on_preferences_window_activate(NULL, local_llog);
}


static void on_edit_log_db_activate(app_widgets_t *app_wdgts) {
  (void)app_wdgts;

  /*It would be nice if the editing were done inside the application,
     but for now I just launch the sqlitebrowser, and we can edit the
     database from there. A solution from a lazy programmer.
   */

  int ret;

  ret = fork();

  switch (ret) {
  case 0:
    ret = execlp("sqlitebrowser", "sqlitebrowser", local_llog->log_file_name, (char *)NULL);
    perror("execlp");
    break;

  case -1:
    perror("fork");
    break;

  default:
    printf("Forked PID: (%d)\n", ret);
    break;
  }
}


static void on_qrt_activate(GtkApplication *app) {
  llog_shutdown();
  g_application_quit(G_APPLICATION(app));
}


/*Actions*/

void main_window_add_log_entry_to_list(log_entry_t *entry) {
  LogEntryDisplayItem *item = logentrydisplay_new(entry);
  g_list_store_append(widgets->logged_list_store, G_OBJECT(item));
  g_object_unref(item);  // drop our ref; the store holds its own
}


void main_window_add_station_entry_to_list(station_entry_t *station) {
  StationEntry *item = station_entry_new(station);
  g_list_store_append(widgets->station_list_store, G_OBJECT(item));
  g_object_unref(item);
}


void main_window_add_mode_entry_to_list(mode_entry_t *mode) {
  ModeEntry *item = mode_entry_new(mode);
  g_list_store_append(widgets->mode_list_store, G_OBJECT(item));
  g_object_unref(item);
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


static gboolean on_close_request(GtkWindow *window, gpointer user_data) {
  (void)window;
  GtkApplication *app = GTK_APPLICATION(user_data);

  llog_shutdown();
  widgets = NULL;         // prevent any remaining callbacks from touching widgets
  on_qrt_activate(app);

  return TRUE;  // TRUE means "I handled it, don't destroy the window"
}


static gboolean rebuild_done_cb(gpointer user_data) {
  InitData *data = user_data;

  /*Reload static data on the main thread after background rebuild*/
  llog_load_static_data(&log_entry_data);
  set_static_data();

  gtk_window_destroy(GTK_WINDOW(data->splash));
  gtk_widget_set_sensitive(widgets->main_window, TRUE);

  g_free(data);
  return G_SOURCE_REMOVE;
}

static gpointer rebuild_thread_func(gpointer user_data) {
  /*Slow call — runs off the main thread*/
  llog_ensure_aux_db(true);

  g_idle_add(rebuild_done_cb, user_data);
  return NULL;
}

static void on_rebuild_aux_db_activate(app_widgets_t *app_wdgts) {
  (void)app_wdgts;

  GtkApplication *app = GTK_APPLICATION(
      gtk_window_get_application(GTK_WINDOW(widgets->main_window)));

  /*Show splash and block the main window while rebuilding*/
  GtkWidget *splash = build_splash(app);
  gtk_window_set_transient_for(GTK_WINDOW(splash), GTK_WINDOW(widgets->main_window));
  gtk_window_present(GTK_WINDOW(splash));
  gtk_widget_set_sensitive(widgets->main_window, FALSE);

  InitData *data = g_new0(InitData, 1);
  data->app    = app;
  data->splash = splash;

  GThread *t = g_thread_new("rebuild-aux-db", rebuild_thread_func, data);
  g_thread_unref(t);
}
