#include <gtk/gtk.h>
#include "llog.h"


/*Forward declarations for the callbacks*/
static void on_button_ok_clicked(GtkWidget *widget, gpointer data);
static void on_button_apply_clicked(GtkWidget *widget, gpointer data);
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
  widgets->llog = (llog_t *) data;


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

  widgets->button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_append(GTK_BOX(widgets->box), widgets->button_box);

  widgets->button_ok = gtk_button_new_with_label("OK");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_ok);

  widgets->button_apply = gtk_button_new_with_label("Apply");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_apply);

  widgets->button_cancel = gtk_button_new_with_label("Cancel");
  gtk_box_append(GTK_BOX(widgets->button_box), widgets->button_cancel);
  g_signal_connect(widgets->button_ok, "clicked", G_CALLBACK(on_button_ok_clicked), widgets);
  g_signal_connect(widgets->button_apply, "clicked", G_CALLBACK(on_button_apply_clicked), widgets);
  g_signal_connect(widgets->button_cancel, "clicked", G_CALLBACK(on_button_cancel_clicked), widgets);
  gtk_widget_show(widgets->window);
}

static void on_button_ok_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  (void)data;
  g_print("OK button clicked\n");
  // Add your code to handle OK button click
}

static void on_button_apply_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;
  (void)data;
  g_print("Apply button clicked\n");
  // Add your code to handle Apply button click
}

static void on_button_cancel_clicked(GtkWidget *widget, gpointer data) {
  (void)widget;

  g_print("Cancel button clicked\n");
  gtk_window_close(GTK_WINDOW(((app_widgets_t *)data)->window));
}

static void on_preferences_window_destroy(GtkWidget *widget, gpointer data) {
  (void)widget;

  g_free(data);

  g_print("Preferences window destroyed\n");
}
