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
#include "position.h"
#include "main_window.h"
#include "xml_client.h"

#define PORT_LEN 16


/*Forward declarations for the callbacks*/
static void on_button_ok_clicked(GtkWidget *widget, gpointer data);
static void on_button_cancel_clicked(GtkWidget *widget, gpointer data);
static void on_preferences_window_destroy(GtkWidget *widget, gpointer data);

typedef struct {
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *gpsd_host_label;
  GtkWidget *gpsd_host_entry;
  GtkWidget *gpsd_port_label;
  GtkWidget *gpsd_port_entry;
  GtkWidget *xml_rpc_host_label;
  GtkWidget *xml_rpc_host_entry;
  GtkWidget *xml_rpc_port_label;
  GtkWidget *xml_rpc_port_entry;
  GtkWidget *button_box;
  GtkWidget *button_ok;
  GtkWidget *button_cancel;
  GtkWidget *band_nr_check;
  llog_t *llog;
} app_widgets_t;

static app_widgets_t *preferences_widgets = NULL;

void on_preferences_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;

  if (preferences_widgets != NULL) {
    gtk_window_present(GTK_WINDOW(preferences_widgets->window));
    return;
  }

  preferences_widgets = g_malloc(sizeof(app_widgets_t));

  preferences_widgets->llog = (llog_t *)data;


  preferences_widgets->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(preferences_widgets->window), "Preferences");
  gtk_window_set_default_size(GTK_WINDOW(preferences_widgets->window), 400, 300);
  g_signal_connect(preferences_widgets->window, "destroy", G_CALLBACK(on_preferences_window_destroy), preferences_widgets);


  preferences_widgets->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_window_set_child(GTK_WINDOW(preferences_widgets->window), preferences_widgets->box);

  GtkWidget *grid;

  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(preferences_widgets->box), grid);

  preferences_widgets->gpsd_host_label = gtk_label_new("GPSD Host:");
  preferences_widgets->gpsd_host_entry = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->gpsd_host_label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->gpsd_host_entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand(preferences_widgets->gpsd_host_entry, TRUE);

  preferences_widgets->gpsd_port_label = gtk_label_new("GPSD Port:");
  preferences_widgets->gpsd_port_entry = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->gpsd_port_label, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->gpsd_port_entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand(preferences_widgets->gpsd_port_entry, TRUE);

  gtk_editable_set_text(GTK_EDITABLE(preferences_widgets->gpsd_host_entry), preferences_widgets->llog->gpsd_host);
  char port[PORT_LEN];

  snprintf(port, PORT_LEN, "%lu", preferences_widgets->llog->gpsd_port);
  gtk_editable_set_text(GTK_EDITABLE(preferences_widgets->gpsd_port_entry), port);

  preferences_widgets->xml_rpc_host_label = gtk_label_new("XML-RPC Host:");
  preferences_widgets->xml_rpc_host_entry = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->xml_rpc_host_label, 0, 2, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->xml_rpc_host_entry, 1, 2, 1, 1);
  gtk_editable_set_text(GTK_EDITABLE(preferences_widgets->xml_rpc_host_entry), preferences_widgets->llog->xmlrpc_host);

  preferences_widgets->xml_rpc_port_label = gtk_label_new("XML-RPC Port:");
  preferences_widgets->xml_rpc_port_entry = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->xml_rpc_port_label, 0, 3, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->xml_rpc_port_entry, 1, 3, 1, 1);
  snprintf(port, PORT_LEN, "%lu", preferences_widgets->llog->xmlrpc_port);
  gtk_editable_set_text(GTK_EDITABLE(preferences_widgets->xml_rpc_port_entry), port);


  GtkWidget *band_nr_label = gtk_label_new("Band serial number:");
  gtk_grid_attach(GTK_GRID(grid), band_nr_label, 0, 4, 1, 1);

  preferences_widgets->band_nr_check = gtk_check_button_new();
  gtk_check_button_set_active(GTK_CHECK_BUTTON(preferences_widgets->band_nr_check),
                            preferences_widgets->llog->band_nr);
  gtk_grid_attach(GTK_GRID(grid), preferences_widgets->band_nr_check, 1, 4, 1, 1);

  preferences_widgets->button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append(GTK_BOX(preferences_widgets->box), preferences_widgets->button_box);

  preferences_widgets->button_ok = gtk_button_new_with_label("OK");
  gtk_box_append(GTK_BOX(preferences_widgets->button_box), preferences_widgets->button_ok);

  preferences_widgets->button_cancel = gtk_button_new_with_label("Cancel");
  gtk_box_append(GTK_BOX(preferences_widgets->button_box), preferences_widgets->button_cancel);
  g_signal_connect(preferences_widgets->button_ok, "clicked", G_CALLBACK(on_button_ok_clicked), preferences_widgets);
  g_signal_connect(preferences_widgets->button_cancel, "clicked", G_CALLBACK(on_button_cancel_clicked), preferences_widgets);
  gtk_window_present(GTK_WINDOW(preferences_widgets->window));
}

static void on_button_ok_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  app_widgets_t *widgets = (app_widgets_t *)data;
  char *endptr;
  unsigned long val;

  const gchar *host = gtk_editable_get_text(GTK_EDITABLE(widgets->gpsd_host_entry));
  const gchar *port = gtk_editable_get_text(GTK_EDITABLE(widgets->gpsd_port_entry));

  g_strlcpy(widgets->llog->gpsd_host, host, sizeof(widgets->llog->gpsd_host));
  val = strtoul(gtk_editable_get_text(GTK_EDITABLE(widgets->gpsd_port_entry)), &endptr, 10);
  if (endptr != port && *endptr == '\0' && val <= 65535) {
    widgets->llog->gpsd_port = val;
  }

  host = gtk_editable_get_text(GTK_EDITABLE(widgets->xml_rpc_host_entry));
  port = gtk_editable_get_text(GTK_EDITABLE(widgets->xml_rpc_port_entry));

  g_strlcpy(widgets->llog->xmlrpc_host, host, sizeof(widgets->llog->xmlrpc_host));
  val = strtoul(gtk_editable_get_text(GTK_EDITABLE(widgets->xml_rpc_port_entry)), &endptr, 10);
  if (endptr != port && *endptr == '\0' && val <= 65535) {
    widgets->llog->xmlrpc_port = val;
  }

  widgets->llog->band_nr = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets->band_nr_check));

  llog_save_config_file();
  main_window_update_txnr();

  position_init(widgets->llog->gpsd_host, widgets->llog->gpsd_port, main_window_update_position_labels);
  main_window_clear_position_labels();

  xml_client_shutdown();
  xml_client_init(widgets->llog->xmlrpc_host, widgets->llog->xmlrpc_port);

  on_button_cancel_clicked(widget, data); // Close the window
}


static void on_button_cancel_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  gtk_window_close(GTK_WINDOW(((app_widgets_t *)data)->window));
}

static void on_preferences_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;

  g_free(data);
  preferences_widgets = NULL;  // clear the guard
}
