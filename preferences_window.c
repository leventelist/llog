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


/*Forward declarations for the callbacks*/
static void on_button_ok_clicked(GtkWidget *widget, gpointer data);
static void on_button_cancel_clicked(GtkWidget *widget, gpointer data);
static void on_preferences_window_destroy(GtkWidget *widget, gpointer data);

typedef struct {
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *host_label;
  GtkWidget *host_entry;
  GtkWidget *port_label;
  GtkWidget *port_entry;
  GtkWidget *button_box;
  GtkWidget *button_ok;
  GtkWidget *button_apply;
  GtkWidget *button_cancel;
  llog_t *llog;
} app_widgets_t;

void on_preferences_window_activate(GtkWidget *widget, gpointer data) {
  (void)widget;

  app_widgets_t *widgets = g_malloc(sizeof(app_widgets_t));

  widgets->llog = (llog_t *)data;


  widgets->window = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(widgets->window), "Preferences");
  gtk_window_set_default_size(GTK_WINDOW(widgets->window), 400, 300);
  g_signal_connect(widgets->window, "destroy", G_CALLBACK(on_preferences_window_destroy), widgets);


  widgets->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 15);
  gtk_window_set_child(GTK_WINDOW(widgets->window), widgets->box);

  GtkWidget *grid;

  grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
  gtk_box_append(GTK_BOX(widgets->box), grid);

  widgets->host_label = gtk_label_new("GPSD Host:");
  widgets->host_entry = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), widgets->host_label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), widgets->host_entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand(widgets->host_entry, TRUE);

  widgets->port_label = gtk_label_new("GPSD Port:");
  widgets->port_entry = gtk_entry_new();
  gtk_grid_attach(GTK_GRID(grid), widgets->port_label, 0, 1, 1, 1);
  gtk_grid_attach(GTK_GRID(grid), widgets->port_entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand(widgets->port_entry, TRUE);

  gtk_editable_set_text(GTK_EDITABLE(widgets->host_entry), widgets->llog->gpsd_host);
  char port[16];

  sprintf(port, "%lu", widgets->llog->gpsd_port);
  gtk_editable_set_text(GTK_EDITABLE(widgets->port_entry), port);

  widgets->button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append(GTK_BOX(widgets->box), widgets->button_box);

  widgets->button_ok = gtk_button_new_with_label("OK");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_ok);

  widgets->button_cancel = gtk_button_new_with_label("Cancel");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_cancel);
  g_signal_connect(widgets->button_ok, "clicked", G_CALLBACK(on_button_ok_clicked), widgets);
  g_signal_connect(widgets->button_cancel, "clicked", G_CALLBACK(on_button_cancel_clicked), widgets);
  gtk_widget_show(widgets->window);
}

static void on_button_ok_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  // Add your code to handle OK button click
  app_widgets_t *widgets = (app_widgets_t *)data;
  const gchar *host = gtk_editable_get_text(GTK_EDITABLE(widgets->host_entry));
  const gchar *port = gtk_editable_get_text(GTK_EDITABLE(widgets->port_entry));

  strcpy(widgets->llog->gpsd_host, host);
  widgets->llog->gpsd_port = atoi(port);

  llog_save_config_file();
  position_init(widgets->llog->gpsd_host, widgets->llog->gpsd_port);

  on_button_cancel_clicked(widget, data); // Close the window
}


static void on_button_cancel_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  gtk_window_close(GTK_WINDOW(((app_widgets_t *)data)->window));
}

static void on_preferences_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;

  g_free(data);

  g_print("Preferences window destroyed\n");
}
