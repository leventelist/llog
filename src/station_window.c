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

/* Table editor for the station table of the log database.
 * Edits are kept in memory and written to the database on Save.
 */

#include <gtk/gtk.h>
#include <inttypes.h>
#include <string.h>

#include "station_window.h"
#include "llog.h"
#include "db_sqlite.h"
#include "main_window.h"


enum {
  station_field_name = 0,
  station_field_call,
  station_field_qth,
  station_field_qra,
  station_field_asl,
  station_field_rig,
  station_field_ant,
  station_field_operator_call,
  station_field_operator_name,
  station_field_comment,
  station_fields
};

static const char *field_titles[station_fields] = {
  "Name", "Call", "QTH", "QRA", "ASL", "Rig", "Antenna", "Operator call", "Operator name", "Comment"
};

static const int field_lengths[station_fields] = {
  NAME_LEN, CALL_LEN, QTH_LEN, QRA_LEN, ASL_LEN, RIG_LEN, ANT_LEN, CALL_LEN, NAME_LEN, COMMENT_LEN
};


/*One row of the table*/

#define STATION_ROW_TYPE (station_row_get_type())
G_DECLARE_FINAL_TYPE(StationRow, station_row, STATION, ROW, GObject)

struct _StationRow {
  GObject parent_instance;
  uint64_t id;                    /*rowid in the database, 0 for a new row*/
  char *fields[station_fields];
};

G_DEFINE_TYPE(StationRow, station_row, G_TYPE_OBJECT)

static void station_row_init(StationRow *self) {
  self->id = 0;
  for (int i = 0; i < station_fields; i++) {
    self->fields[i] = g_strdup("");
  }
}

static void station_row_finalize(GObject *object) {
  StationRow *self = STATION_ROW(object);

  for (int i = 0; i < station_fields; i++) {
    g_free(self->fields[i]);
  }
  G_OBJECT_CLASS(station_row_parent_class)->finalize(object);
}

static void station_row_class_init(StationRowClass *klass) {
  G_OBJECT_CLASS(klass)->finalize = station_row_finalize;
}

static StationRow *station_row_new(const station_entry_t *station) {
  StationRow *self = g_object_new(STATION_ROW_TYPE, NULL);

  if (station != NULL) {
    const char *values[station_fields] = {
      station->name, station->call, station->QTH, station->QRA, station->ASL, station->rig, station->ant,
      station->operator_call, station->operator_name, station->comment
    };

    self->id = station->id;
    for (int i = 0; i < station_fields; i++) {
      g_free(self->fields[i]);
      self->fields[i] = g_strdup(values[i]);
    }
  }
  return self;
}


typedef struct {
  GtkWidget *window;
  GtkWidget *column_view;
  GListStore *store;
  GtkSingleSelection *selection;
  GArray *deleted_ids;            /*rowids to delete on Save*/
  llog_t *llog;
} station_widgets_t;

static station_widgets_t *station_widgets = NULL;


static void show_alert(const char *message, const char *detail) {
  GtkAlertDialog *alert = gtk_alert_dialog_new("%s", message);

  if (detail != NULL) {
    gtk_alert_dialog_set_detail(alert, detail);
  }
  gtk_alert_dialog_show(alert, GTK_WINDOW(station_widgets->window));
  g_object_unref(alert);
}


/*Cell factory callbacks. The field index is the factory's user data.*/

static void on_cell_changed(GtkEditable *editable, gpointer user_data) {
  StationRow *row = STATION_ROW(g_object_get_data(G_OBJECT(editable), "row"));
  int field = GPOINTER_TO_INT(user_data);

  g_free(row->fields[field]);
  row->fields[field] = g_strdup(gtk_editable_get_text(editable));
}

static void setup_cell_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem, gpointer user_data) {
  (void)factory;
  GtkWidget *entry = gtk_entry_new();

  gtk_entry_set_max_length(GTK_ENTRY(entry), field_lengths[GPOINTER_TO_INT(user_data)] - 1);
  gtk_entry_set_has_frame(GTK_ENTRY(entry), FALSE);
  gtk_list_item_set_child(listitem, entry);
}

static void bind_cell_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem, gpointer user_data) {
  (void)factory;
  GtkWidget *entry = gtk_list_item_get_child(listitem);
  StationRow *row = STATION_ROW(gtk_list_item_get_item(listitem));
  int field = GPOINTER_TO_INT(user_data);

  /*Set the text before connecting, so binding doesn't count as an edit*/
  gtk_editable_set_text(GTK_EDITABLE(entry), row->fields[field]);
  g_object_set_data(G_OBJECT(entry), "row", row);
  g_signal_connect(entry, "changed", G_CALLBACK(on_cell_changed), user_data);
}

static void unbind_cell_cb(GtkSignalListItemFactory *factory, GtkListItem *listitem, gpointer user_data) {
  (void)factory;
  GtkWidget *entry = gtk_list_item_get_child(listitem);

  g_signal_handlers_disconnect_by_func(entry, on_cell_changed, user_data);
  g_object_set_data(G_OBJECT(entry), "row", NULL);
}


/*Button callbacks*/

static void on_add_clicked(GtkButton *button, gpointer data) {
  (void)button;
  station_widgets_t *w = data;
  StationRow *row = station_row_new(NULL);

  g_list_store_append(w->store, row);
  g_object_unref(row);
  gtk_single_selection_set_selected(w->selection, g_list_model_get_n_items(G_LIST_MODEL(w->store)) - 1);
}

static void on_delete_clicked(GtkButton *button, gpointer data) {
  (void)button;
  station_widgets_t *w = data;
  guint pos = gtk_single_selection_get_selected(w->selection);
  StationRow *row;
  uint64_t count;

  if (pos == GTK_INVALID_LIST_POSITION) {
    return;
  }

  if (g_list_model_get_n_items(G_LIST_MODEL(w->store)) <= 1) {
    show_alert("Cannot delete the last station.", "The log needs at least one station.");
    return;
  }

  row = g_list_model_get_item(G_LIST_MODEL(w->store), pos);

  if (row->id != 0) {
    if (db_get_station_use_count(w->llog, row->id, &count) != llog_stat_ok) {
      show_alert("Cannot delete the station.", "Error reading the log database.");
      g_object_unref(row);
      return;
    }
    if (count > 0) {
      char *detail = g_strdup_printf("%" PRIu64 " QSO(s) in the log refer to station \"%s\".", count, row->fields[station_field_name]);
      show_alert("Station is in use.", detail);
      g_free(detail);
      g_object_unref(row);
      return;
    }
    g_array_append_val(w->deleted_ids, row->id);
  }

  g_object_unref(row);
  g_list_store_remove(w->store, pos);
}

static void on_save_clicked(GtkButton *button, gpointer data) {
  (void)button;
  station_widgets_t *w = data;
  GListModel *model = G_LIST_MODEL(w->store);
  guint n = g_list_model_get_n_items(model);
  station_entry_t station;

  /*Validate everything before touching the database*/
  for (guint i = 0; i < n; i++) {
    StationRow *row = g_list_model_get_item(model, i);
    gboolean empty = g_strstrip(row->fields[station_field_name])[0] == '\0';

    g_object_unref(row);
    if (empty) {
      gtk_single_selection_set_selected(w->selection, i);
      show_alert("Every station needs a name.", NULL);
      return;
    }
  }

  for (guint i = 0; i < w->deleted_ids->len; i++) {
    uint64_t id = g_array_index(w->deleted_ids, uint64_t, i);

    if (db_delete_station_entry(w->llog, id) != llog_stat_ok) {
      show_alert("Error deleting station.", "See the console for details.");
      return;
    }
  }
  g_array_set_size(w->deleted_ids, 0);

  for (guint i = 0; i < n; i++) {
    StationRow *row = g_list_model_get_item(model, i);

    memset(&station, 0, sizeof(station));
    station.id = row->id;
    g_strlcpy(station.name, row->fields[station_field_name], NAME_LEN);
    g_strlcpy(station.call, row->fields[station_field_call], CALL_LEN);
    g_strlcpy(station.QTH, row->fields[station_field_qth], QTH_LEN);
    g_strlcpy(station.QRA, row->fields[station_field_qra], QRA_LEN);
    g_strlcpy(station.ASL, row->fields[station_field_asl], ASL_LEN);
    g_strlcpy(station.rig, row->fields[station_field_rig], RIG_LEN);
    g_strlcpy(station.ant, row->fields[station_field_ant], ANT_LEN);
    g_strlcpy(station.operator_call, row->fields[station_field_operator_call], CALL_LEN);
    g_strlcpy(station.operator_name, row->fields[station_field_operator_name], NAME_LEN);
    g_strlcpy(station.comment, row->fields[station_field_comment], COMMENT_LEN);

    if (db_set_station_entry(w->llog, &station) != llog_stat_ok) {
      g_object_unref(row);
      show_alert("Error saving station.", "See the console for details.");
      main_window_reload_stations();
      return;
    }
    row->id = station.id;     /*New rows get their rowid, so a second Save updates them*/
    g_object_unref(row);
  }

  main_window_reload_stations();
  gtk_window_close(GTK_WINDOW(w->window));
}

static void on_cancel_clicked(GtkButton *button, gpointer data) {
  (void)button;
  station_widgets_t *w = data;

  gtk_window_close(GTK_WINDOW(w->window));
}

static void on_station_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;
  station_widgets_t *w = data;

  g_array_free(w->deleted_ids, TRUE);
  g_object_unref(w->selection);
  g_free(w);
  station_widgets = NULL;
}


static void load_stations(station_widgets_t *w) {
  station_entry_t station;
  GPtrArray *rows = g_ptr_array_new_with_free_func(g_object_unref);

  /*The query returns the newest station first; show them in creation order*/
  station.data_stat = db_data_init;
  station.id = 0;
  while (station.data_stat != db_data_last) {
    if (db_get_station_entry(w->llog, &station) != llog_stat_ok || station.data_stat != db_data_valid) {
      break;
    }
    g_ptr_array_add(rows, station_row_new(&station));
  }

  for (guint i = rows->len; i > 0; i--) {
    g_list_store_append(w->store, g_ptr_array_index(rows, i - 1));
  }
  g_ptr_array_free(rows, TRUE);
}


void on_station_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  station_widgets_t *w;

  if (station_widgets != NULL) {
    gtk_window_present(GTK_WINDOW(station_widgets->window));
    return;
  }

  w = g_new0(station_widgets_t, 1);
  station_widgets = w;
  w->llog = (llog_t *)data;
  w->deleted_ids = g_array_new(FALSE, FALSE, sizeof(uint64_t));

  w->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(w->window), "Stations");
  gtk_window_set_default_size(GTK_WINDOW(w->window), 1200, 300);
  g_signal_connect(w->window, "destroy", G_CALLBACK(on_station_window_destroy), w);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_top(box, 10);
  gtk_widget_set_margin_bottom(box, 10);
  gtk_widget_set_margin_start(box, 10);
  gtk_widget_set_margin_end(box, 10);
  gtk_window_set_child(GTK_WINDOW(w->window), box);

  /*Table*/
  w->store = g_list_store_new(STATION_ROW_TYPE);
  w->selection = gtk_single_selection_new(G_LIST_MODEL(w->store));   /*Takes the store*/
  w->column_view = gtk_column_view_new(GTK_SELECTION_MODEL(g_object_ref(w->selection)));
  gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(w->column_view), TRUE);
  gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(w->column_view), TRUE);

  for (int i = 0; i < station_fields; i++) {
    GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
    GtkColumnViewColumn *column;

    g_signal_connect(factory, "setup", G_CALLBACK(setup_cell_cb), GINT_TO_POINTER(i));
    g_signal_connect(factory, "bind", G_CALLBACK(bind_cell_cb), GINT_TO_POINTER(i));
    g_signal_connect(factory, "unbind", G_CALLBACK(unbind_cell_cb), GINT_TO_POINTER(i));
    column = gtk_column_view_column_new(field_titles[i], factory);
    gtk_column_view_column_set_resizable(column, TRUE);
    gtk_column_view_column_set_expand(column, TRUE);
    gtk_column_view_append_column(GTK_COLUMN_VIEW(w->column_view), column);
    g_object_unref(column);
  }

  GtkWidget *scrolled_window = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), w->column_view);
  gtk_widget_set_vexpand(scrolled_window, TRUE);
  gtk_box_append(GTK_BOX(box), scrolled_window);

  /*Buttons*/
  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget *button;

  button = gtk_button_new_with_label("Add");
  g_signal_connect(button, "clicked", G_CALLBACK(on_add_clicked), w);
  gtk_box_append(GTK_BOX(button_box), button);

  button = gtk_button_new_with_label("Delete");
  g_signal_connect(button, "clicked", G_CALLBACK(on_delete_clicked), w);
  gtk_box_append(GTK_BOX(button_box), button);

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(button_box), spacer);

  button = gtk_button_new_with_label("Save");
  g_signal_connect(button, "clicked", G_CALLBACK(on_save_clicked), w);
  gtk_box_append(GTK_BOX(button_box), button);

  button = gtk_button_new_with_label("Cancel");
  g_signal_connect(button, "clicked", G_CALLBACK(on_cancel_clicked), w);
  gtk_box_append(GTK_BOX(button_box), button);

  gtk_box_append(GTK_BOX(box), button_box);

  load_stations(w);

  gtk_window_present(GTK_WINDOW(w->window));
}
