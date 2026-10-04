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

/* Editor for a single QSO of the log table.
 * Opened by double clicking a row of the logged list.
 */

#include <gtk/gtk.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>

#include "qso_window.h"
#include "llog.h"
#include "db_sqlite.h"


enum {
  qso_kind_text = 0,
  qso_kind_uint,
  qso_kind_qrg,
  qso_kind_mode           /*Dropdown of the aux database's mode table*/
};

typedef struct {
  const char *title;
  size_t offset;          /*Offset of the field in log_entry_t*/
  size_t len;             /*Buffer size for text fields*/
  int kind;
} qso_field_t;

#define TEXT_FIELD(t, m) { t, offsetof(log_entry_t, m), sizeof(((log_entry_t *)0)->m), qso_kind_text }

static const qso_field_t qso_fields[] = {
  TEXT_FIELD("Call", call),
  TEXT_FIELD("Date", date),
  TEXT_FIELD("UTC", utc),
  { "QRG", offsetof(log_entry_t, qrg), 0, qso_kind_qrg },
  { "Mode", offsetof(log_entry_t, mode.name), sizeof(((log_entry_t *)0)->mode.name), qso_kind_mode },
  TEXT_FIELD("Power", power),
  TEXT_FIELD("RX RST", rxrst),
  TEXT_FIELD("TX RST", txrst),
  { "RX NR", offsetof(log_entry_t, rxnr), 0, qso_kind_uint },
  { "TX NR", offsetof(log_entry_t, txnr), 0, qso_kind_uint },
  TEXT_FIELD("RX Extra", rxextra),
  TEXT_FIELD("TX Extra", txextra),
  TEXT_FIELD("Name", name),
  TEXT_FIELD("QTH", qth),
  TEXT_FIELD("QRA", qra),
  TEXT_FIELD("Summit ref", sota_ref),
  TEXT_FIELD("S2S ref", s2s_ref),
  TEXT_FIELD("Park ref", pota_ref),
  TEXT_FIELD("P2P ref", p2p_ref),
  TEXT_FIELD("WWFF ref", wwff_ref),
  TEXT_FIELD("W2W ref", w2w_ref),
  TEXT_FIELD("Comment", comment)
};

#define QSO_FIELDS G_N_ELEMENTS(qso_fields)


typedef struct {
  GtkWidget *window;
  GtkWidget *entries[QSO_FIELDS];       /*GtkEntry, or GtkDropDown for qso_kind_mode*/
  GtkWidget *station_dropdown;
  GArray *station_ids;            /*Station rowids, in dropdown order*/
  llog_t *llog;
  log_entry_t entry;
} qso_widgets_t;

/*One editor per QSO; maps log rowid to its qso_widgets_t*/
static GHashTable *open_editors = NULL;


static void show_alert(qso_widgets_t *w, const char *message, const char *detail) {
  GtkAlertDialog *alert = gtk_alert_dialog_new("%s", message);

  if (detail != NULL) {
    gtk_alert_dialog_set_detail(alert, detail);
  }
  gtk_alert_dialog_show(alert, GTK_WINDOW(w->window));
  g_object_unref(alert);
}


/*Fill a mode dropdown from the mode table, the same list the main window offers,
  and select the given mode. A mode missing from the table is added, so saving keeps it.*/
static void load_modes(qso_widgets_t *w, GtkDropDown *dropdown, const char *current) {
  GtkStringList *names = gtk_string_list_new(NULL);
  mode_entry_t mode;
  guint n = 0, selected = GTK_INVALID_LIST_POSITION;

  mode.data_stat = db_data_init;
  while (mode.data_stat != db_data_last) {
    if (db_get_mode_entry(w->llog, &mode, NULL) != llog_stat_ok || mode.data_stat != db_data_valid) {
      break;
    }
    if (selected == GTK_INVALID_LIST_POSITION && g_ascii_strcasecmp(mode.name, current) == 0) {
      selected = n;
    }
    gtk_string_list_append(names, mode.name);
    n++;
  }

  if (selected == GTK_INVALID_LIST_POSITION) {
    selected = n;
    gtk_string_list_append(names, current);
  }

  gtk_drop_down_set_model(dropdown, G_LIST_MODEL(names));
  gtk_drop_down_set_selected(dropdown, selected);
  g_object_unref(names);
}


static void field_to_entry(qso_widgets_t *w, guint i) {
  const qso_field_t *f = &qso_fields[i];
  char *ptr = (char *)&w->entry + f->offset;
  char buff[G_ASCII_DTOSTR_BUF_SIZE];

  switch (f->kind) {
  case qso_kind_uint:
    snprintf(buff, sizeof(buff), "%" PRIu64, *(uint64_t *)ptr);
    gtk_editable_set_text(GTK_EDITABLE(w->entries[i]), buff);
    break;

  case qso_kind_qrg:
    g_ascii_dtostr(buff, sizeof(buff), *(double *)ptr);
    gtk_editable_set_text(GTK_EDITABLE(w->entries[i]), buff);
    break;

  case qso_kind_mode:
    load_modes(w, GTK_DROP_DOWN(w->entries[i]), ptr);
    break;

  default:
    gtk_editable_set_text(GTK_EDITABLE(w->entries[i]), ptr);
    break;
  }
}


/*Copy one entry back into w->entry. Returns FALSE if the text is not a valid value.*/
static gboolean entry_to_field(qso_widgets_t *w, guint i) {
  const qso_field_t *f = &qso_fields[i];
  char *ptr = (char *)&w->entry + f->offset;
  char *text;
  gboolean ok = TRUE;
  char *end;

  if (f->kind == qso_kind_mode) {
    GObject *item = gtk_drop_down_get_selected_item(GTK_DROP_DOWN(w->entries[i]));

    if (item != NULL) {
      g_strlcpy(ptr, gtk_string_object_get_string(GTK_STRING_OBJECT(item)), f->len);
    }
    return TRUE;
  }

  text = g_strstrip(g_strdup(gtk_editable_get_text(GTK_EDITABLE(w->entries[i]))));

  switch (f->kind) {
  case qso_kind_uint:
    if (text[0] == '\0') {
      *(uint64_t *)ptr = 0;
    } else {
      guint64 value = g_ascii_strtoull(text, &end, 10);

      ok = *end == '\0' && text[0] != '-';
      if (ok) {
        *(uint64_t *)ptr = value;
      }
    }
    break;

  case qso_kind_qrg: {
    double value = g_ascii_strtod(text, &end);

    ok = text[0] != '\0' && *end == '\0' && value > 0;
    if (ok) {
      *(double *)ptr = value;
    }
    break;
  }

  default:
    g_strlcpy(ptr, text, f->len);
    break;
  }

  g_free(text);
  return ok;
}


static void on_save_clicked(GtkButton *button, gpointer data) {
  (void)button;
  qso_widgets_t *w = data;
  guint selected;

  /*Validate into a copy, so a failed save leaves the loaded data intact*/
  log_entry_t backup = w->entry;

  for (guint i = 0; i < QSO_FIELDS; i++) {
    if (!entry_to_field(w, i)) {
      char *msg = g_strdup_printf("Invalid %s.", qso_fields[i].title);

      w->entry = backup;
      gtk_widget_grab_focus(w->entries[i]);
      show_alert(w, msg, qso_fields[i].kind == qso_kind_qrg ? "Enter the frequency in MHz, e.g. 14.074" : "Enter a non-negative whole number.");
      g_free(msg);
      return;
    }
  }

  if (w->entry.call[0] == '\0') {
    w->entry = backup;
    gtk_widget_grab_focus(w->entries[0]);
    show_alert(w, "The call can't be empty.", NULL);
    return;
  }

  selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(w->station_dropdown));
  if (selected != GTK_INVALID_LIST_POSITION && selected < w->station_ids->len) {
    w->entry.station_id = g_array_index(w->station_ids, uint64_t, selected);
  }

  if (db_update_log_entry(w->llog, &w->entry) != llog_stat_ok) {
    w->entry = backup;
    show_alert(w, "Error saving the QSO.", "See the console for details.");
    return;
  }

  llog_add_log_entries();
  gtk_window_close(GTK_WINDOW(w->window));
}


static void on_cancel_clicked(GtkButton *button, gpointer data) {
  (void)button;
  qso_widgets_t *w = data;

  gtk_window_close(GTK_WINDOW(w->window));
}


static void on_qso_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;
  qso_widgets_t *w = data;

  g_hash_table_remove(open_editors, &w->entry.id);
  g_array_free(w->station_ids, TRUE);
  g_free(w);
}


/*Fill the station dropdown, selecting the QSO's station.
  A station that no longer exists shows up as its bare rowid, so saving keeps it.*/
static void load_stations(qso_widgets_t *w) {
  GtkStringList *names = gtk_string_list_new(NULL);
  station_entry_t station;
  guint selected = GTK_INVALID_LIST_POSITION;

  station.data_stat = db_data_init;
  while (station.data_stat != db_data_last) {
    station.id = 0;
    if (db_get_station_entry(w->llog, &station) != llog_stat_ok || station.data_stat != db_data_valid) {
      break;
    }
    if (station.id == w->entry.station_id) {
      selected = w->station_ids->len;
    }
    g_array_append_val(w->station_ids, station.id);
    gtk_string_list_append(names, station.name);
  }

  if (selected == GTK_INVALID_LIST_POSITION) {
    char *name = g_strdup_printf("#%" PRIu64, w->entry.station_id);

    selected = w->station_ids->len;
    g_array_append_val(w->station_ids, w->entry.station_id);
    gtk_string_list_append(names, name);
    g_free(name);
  }

  gtk_drop_down_set_model(GTK_DROP_DOWN(w->station_dropdown), G_LIST_MODEL(names));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(w->station_dropdown), selected);
  g_object_unref(names);
}


static void attach_row(GtkGrid *grid, int col, int row, const char *title, GtkWidget *widget) {
  GtkWidget *label = gtk_label_new(title);

  gtk_label_set_xalign(GTK_LABEL(label), 1.0);
  gtk_widget_set_hexpand(widget, TRUE);
  gtk_grid_attach(grid, label, col * 2, row, 1, 1);
  gtk_grid_attach(grid, widget, col * 2 + 1, row, 1, 1);
}


void qso_window_open(GtkWindow *parent, llog_t *llog, uint64_t log_id) {
  qso_widgets_t *w;
  int ret;

  if (open_editors == NULL) {
    open_editors = g_hash_table_new(g_int64_hash, g_int64_equal);
  }

  w = g_hash_table_lookup(open_editors, &log_id);
  if (w != NULL) {
    gtk_window_present(GTK_WINDOW(w->window));
    return;
  }

  w = g_new0(qso_widgets_t, 1);
  w->llog = llog;
  w->entry.id = log_id;

  ret = db_get_log_entry_by_id(llog, &w->entry);
  if (ret != llog_stat_ok) {
    GtkAlertDialog *alert = gtk_alert_dialog_new("Cannot open QSO #%" PRIu64 ".", log_id);

    gtk_alert_dialog_set_detail(alert, ret == llog_no_data ? "It is no longer in the log." : "Error reading the log database.");
    gtk_alert_dialog_show(alert, parent);
    g_object_unref(alert);
    g_free(w);
    return;
  }

  w->station_ids = g_array_new(FALSE, FALSE, sizeof(uint64_t));
  g_hash_table_insert(open_editors, &w->entry.id, w);

  w->window = gtk_window_new();
  char *title = g_strdup_printf("QSO #%" PRIu64 " — %s", log_id, w->entry.call);
  gtk_window_set_title(GTK_WINDOW(w->window), title);
  g_free(title);
  gtk_window_set_transient_for(GTK_WINDOW(w->window), parent);
  gtk_window_set_default_size(GTK_WINDOW(w->window), 700, -1);
  g_signal_connect(w->window, "destroy", G_CALLBACK(on_qso_window_destroy), w);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_top(box, 10);
  gtk_widget_set_margin_bottom(box, 10);
  gtk_widget_set_margin_start(box, 10);
  gtk_widget_set_margin_end(box, 10);
  gtk_window_set_child(GTK_WINDOW(w->window), box);

  /*Two columns of label/entry pairs; the comment spans the full width*/
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(box), grid);

  guint paired = QSO_FIELDS - 1;
  guint rows = (paired + 1) / 2;

  for (guint i = 0; i < QSO_FIELDS; i++) {
    if (qso_fields[i].kind == qso_kind_mode) {
      GtkExpression *expr = gtk_property_expression_new(GTK_TYPE_STRING_OBJECT, NULL, "string");

      w->entries[i] = gtk_drop_down_new(NULL, expr);
      gtk_drop_down_set_enable_search(GTK_DROP_DOWN(w->entries[i]), TRUE);
    } else {
      w->entries[i] = gtk_entry_new();
      if (qso_fields[i].kind == qso_kind_text) {
        gtk_entry_set_max_length(GTK_ENTRY(w->entries[i]), (int)qso_fields[i].len - 1);
      }
      gtk_entry_set_activates_default(GTK_ENTRY(w->entries[i]), TRUE);
    }
    field_to_entry(w, i);

    if (i < paired) {
      attach_row(GTK_GRID(grid), i / rows, i % rows, qso_fields[i].title, w->entries[i]);
    }
  }

  w->station_dropdown = gtk_drop_down_new(NULL, NULL);
  load_stations(w);
  /*Station goes below the last field of the second column*/
  attach_row(GTK_GRID(grid), 1, paired - rows, "Station", w->station_dropdown);
  rows = MAX(rows, paired - rows + 1);

  GtkWidget *comment_label = gtk_label_new(qso_fields[QSO_FIELDS - 1].title);
  gtk_label_set_xalign(GTK_LABEL(comment_label), 1.0);
  gtk_grid_attach(GTK_GRID(grid), comment_label, 0, rows, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), w->entries[QSO_FIELDS - 1], 1, rows, 3, 1);

  /*Buttons*/
  GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_widget_set_halign(button_box, GTK_ALIGN_END);
  GtkWidget *button;

  button = gtk_button_new_with_label("Cancel");
  g_signal_connect(button, "clicked", G_CALLBACK(on_cancel_clicked), w);
  gtk_box_append(GTK_BOX(button_box), button);

  button = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(button, "suggested-action");
  g_signal_connect(button, "clicked", G_CALLBACK(on_save_clicked), w);
  gtk_box_append(GTK_BOX(button_box), button);
  gtk_window_set_default_widget(GTK_WINDOW(w->window), button);

  gtk_box_append(GTK_BOX(box), button_box);

  gtk_window_present(GTK_WINDOW(w->window));
}
