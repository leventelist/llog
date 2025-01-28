/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2025  Levente Kovacs
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
#include "llog.h"
#include "exporter_writer.h"
#include "db_sqlite.h"

/*Forward declarations for the callbacks*/

typedef struct {
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *adif_fn_label;
  GtkWidget *adif_fn_entry;
  GtkWidget *adif_fn_button;
  GtkWidget *adif_fn_chooser;
  GtkWidget *port_label;
  GtkWidget *port_entry;
  GtkWidget *button_box;
  GtkWidget *button_export;
  GtkWidget *button_cancel;
  GtkWidget *grid;
  GtkWidget *export_format;
  GtkWidget *adi_checkbox;
  GtkWidget *adx_checkbox;
  GtkWidget *csv_checkbox;
  GtkWidget *status_label;
  llog_t *llog;
} adif_widgets_t;

static void on_exporter_window_destroy(GtkWidget *widget, gpointer data);
static void on_exporter_file_chose_btn_clicked(GtkWidget *widget, gpointer data);
static void on_exporter_open_file_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static void on_button_export_clicked(GtkWidget *widget, gpointer data);
static void on_button_close_clicked(GtkWidget *widget, gpointer data);


void on_exporter_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  (void)data;

  adif_widgets_t *widgets = g_malloc(sizeof(adif_widgets_t));

  widgets->llog = (llog_t *)data;

  widgets->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(widgets->window), "Export");
  gtk_window_set_default_size(GTK_WINDOW(widgets->window), 700, 300);
  g_signal_connect(widgets->window, "destroy", G_CALLBACK(on_exporter_window_destroy), widgets);

  widgets->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_window_set_child(GTK_WINDOW(widgets->window), widgets->box);

  widgets->grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(widgets->grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(widgets->grid), 10);
  gtk_box_append(GTK_BOX(widgets->box), widgets->grid);

  widgets->adif_fn_label = gtk_label_new("Export file name:");
  widgets->adif_fn_entry = gtk_entry_new();

  if (widgets->llog->export_file_name[0] != '\0') {
    GtkEntryBuffer *adif_fn_buffer = gtk_entry_get_buffer(GTK_ENTRY(widgets->adif_fn_entry));
    gtk_entry_buffer_set_text(adif_fn_buffer, widgets->llog->export_file_name, -1);
  }

  gtk_grid_attach(GTK_GRID(widgets->grid), widgets->adif_fn_label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(widgets->grid), widgets->adif_fn_entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand(widgets->adif_fn_entry, TRUE);

  widgets->export_format = gtk_label_new("Exprt format:");
  gtk_grid_attach(GTK_GRID(widgets->grid), GTK_WIDGET(widgets->export_format), 0, 1, 1, 1);
  widgets->adi_checkbox = gtk_check_button_new_with_label("ADI");
  gtk_grid_attach(GTK_GRID(widgets->grid), GTK_WIDGET(widgets->adi_checkbox), 1, 1, 1, 1);
  widgets->adx_checkbox = gtk_check_button_new_with_label("ADX");
  gtk_grid_attach(GTK_GRID(widgets->grid), GTK_WIDGET(widgets->adx_checkbox), 1, 2, 1, 1);
  widgets->csv_checkbox = gtk_check_button_new_with_label("CSV (SOTA)");
  gtk_grid_attach(GTK_GRID(widgets->grid), GTK_WIDGET(widgets->csv_checkbox), 1, 3, 1, 1);

  gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->adi_checkbox), GTK_CHECK_BUTTON(widgets->adx_checkbox));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->adi_checkbox), GTK_CHECK_BUTTON(widgets->csv_checkbox));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->adx_checkbox), GTK_CHECK_BUTTON(widgets->csv_checkbox));



  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->adi_checkbox), TRUE);

  widgets->adif_fn_button = gtk_button_new_with_label("Choose file");
  g_signal_connect(widgets->adif_fn_button, "clicked", G_CALLBACK(on_exporter_file_chose_btn_clicked), widgets);
  gtk_grid_attach(GTK_GRID(widgets->grid), widgets->adif_fn_button, 3, 0, 1, 1);

  widgets->status_label = gtk_label_new("Idle");
  gtk_grid_attach(GTK_GRID(widgets->grid), widgets->status_label, 0, 4, 1, 1);

  widgets->button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append(GTK_BOX(widgets->box), widgets->button_box);

  widgets->button_export = gtk_button_new_with_label("Export");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_export);

  widgets->button_cancel = gtk_button_new_with_label("Close");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_cancel);
  g_signal_connect(widgets->button_export, "clicked", G_CALLBACK(on_button_export_clicked), widgets);
  g_signal_connect(widgets->button_cancel, "clicked", G_CALLBACK(on_button_close_clicked), widgets);

  gtk_widget_show(widgets->window);
}


static void on_exporter_file_chose_btn_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  adif_widgets_t *app_wdgts = (adif_widgets_t *)data;

  app_wdgts->adif_fn_chooser = gtk_file_chooser_dialog_new("Export to File",
                                                           GTK_WINDOW(app_wdgts->window),
                                                           GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel",
                                                           GTK_RESPONSE_CANCEL,
                                                           "_Save",
                                                           GTK_RESPONSE_OK,
                                                           NULL);


  if (app_wdgts->llog->export_file_name[0] != '\0') {
    GFile *file = g_file_new_for_path(app_wdgts->llog->export_file_name);
    gtk_file_chooser_set_file(GTK_FILE_CHOOSER(app_wdgts->adif_fn_chooser), file, NULL);
  }

  g_signal_connect(app_wdgts->adif_fn_chooser, "response", G_CALLBACK(on_exporter_open_file_response), app_wdgts);

  gtk_widget_show(app_wdgts->adif_fn_chooser);
}


static void on_exporter_open_file_response(GtkDialog *dialog, gint response_id, gpointer user_data) {

  adif_widgets_t *app_wdgts = (adif_widgets_t *)user_data;
  GtkEntryBuffer *adif_fn_buffer;

  if (response_id == GTK_RESPONSE_OK) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    GFile *file = gtk_file_chooser_get_file(chooser);
    char *filename = g_file_get_path(file);
    if (filename != NULL) {
      strcpy(app_wdgts->llog->export_file_name, filename);
      llog_save_config_file();
    }
    g_print("File selected: %s\n", filename);
    g_free(filename);
    g_object_unref(file);
  }

  adif_fn_buffer = gtk_entry_get_buffer(GTK_ENTRY(app_wdgts->adif_fn_entry));

  gtk_entry_buffer_set_text(adif_fn_buffer,
                            app_wdgts->llog->export_file_name, -1);

  gtk_window_destroy(GTK_WINDOW(dialog));
}


static void on_button_export_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  int ret_val;

  adif_widgets_t *app_wdgts = (adif_widgets_t *)data;

  adif_writer_export_format_t format;

  GtkEntryBuffer * entry_buffer;


  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(app_wdgts->adi_checkbox))) {
    format = export_format_adi;
  } else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(app_wdgts->adx_checkbox))) {
    format = export_format_adx;
  } else {
    format = export_format_csv;
  }

  gtk_label_set_text(GTK_LABEL(app_wdgts->status_label), "Exporting...");

  entry_buffer = gtk_entry_get_buffer(GTK_ENTRY(app_wdgts->adif_fn_entry));

  const char *filename = gtk_entry_buffer_get_text(entry_buffer);

  ret_val = exporter_write_header(filename, format);

  if (ret_val != export_status_ok) {
    gtk_label_set_text(GTK_LABEL(app_wdgts->status_label), "Error writing header");
    return;
  }

  log_entry_t entry;
  station_entry_t station;

  entry.data_stat = db_data_init;

  for (;;) {
    db_get_log_entry_with_station(app_wdgts->llog, &entry, &station);
    if (entry.data_stat != db_data_valid) {
      break;
    }
    ret_val = exporter_add_qso(&entry, &station);
    if (ret_val != export_status_ok) {
      break;
    }
  }

  if (ret_val == export_status_ok) {
    gtk_label_set_text(GTK_LABEL(app_wdgts->status_label), "Export OK");
  } else {
    gtk_label_set_text(GTK_LABEL(app_wdgts->status_label), "Exporting failed");
  }

  exporter_close();
}

static void on_button_close_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  adif_widgets_t *app_wdgts = (adif_widgets_t *)data;

  gtk_window_close(GTK_WINDOW(app_wdgts->window));
}

static void on_exporter_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;

  g_free(data);
}
