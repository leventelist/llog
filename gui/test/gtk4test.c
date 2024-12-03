#include <gtk/gtk.h>

static void on_button_clicked(GtkWidget *button, gpointer user_data) {
  const char *button_label = gtk_button_get_label(GTK_BUTTON(button));

  g_print("Button %s clicked\n", button_label);
}

static void on_entry_changed(GtkEditable *editable, gpointer user_data) {
    const char *text = gtk_editable_get_text(editable);
    g_print("Entry changed: %s\n", text);
}

static void on_open_file_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
  if (response_id == GTK_RESPONSE_ACCEPT) {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    GFile *file = gtk_file_chooser_get_file(chooser);
    char *filename = g_file_get_path(file);
    g_print("File selected: %s\n", filename);
    g_free(filename);
    g_object_unref(file);
  }
  gtk_window_destroy(GTK_WINDOW(dialog));
}

static void on_open_file_clicked(GtkWidget *button, gpointer user_data) {
  GtkWidget *dialog;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  dialog = gtk_file_chooser_dialog_new("Open File",
                                       GTK_WINDOW(user_data),
                                       action,
                                       "_Cancel",
                                       GTK_RESPONSE_CANCEL,
                                       "_Open",
                                       GTK_RESPONSE_ACCEPT,
                                       NULL);

  g_signal_connect(dialog, "response", G_CALLBACK(on_open_file_response), NULL);
  gtk_widget_show(dialog);
}

static void on_activate(GtkApplication *app, gpointer user_data) {
  GtkWidget *window;
  GtkWidget *box;
  GtkWidget *button1;
  GtkWidget *button2;
  GtkWidget *entry;
  GtkWidget *open_file_button;

  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Hello, GTK4!");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

  box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_window_set_child(GTK_WINDOW(window), box);

  entry = gtk_entry_new();

  GtkEntryBuffer *buffer = gtk_entry_get_buffer(GTK_ENTRY(entry));

  gtk_entry_buffer_set_text(buffer, "Default Text", -1); // Set default text
  gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE); // Make the entry editable
  g_signal_connect(entry, "changed", G_CALLBACK(on_entry_changed), NULL); // Connect the changed signal
  gtk_box_append(GTK_BOX(box), entry);

  button1 = gtk_button_new_with_label("Button 1");
  g_signal_connect(button1, "clicked", G_CALLBACK(on_button_clicked), NULL);
  gtk_box_append(GTK_BOX(box), button1);

  button2 = gtk_button_new_with_label("Button 2");
  g_signal_connect(button2, "clicked", G_CALLBACK(on_button_clicked), NULL);
  gtk_box_append(GTK_BOX(box), button2);

  open_file_button = gtk_button_new_with_label("Open File");
  g_signal_connect(open_file_button, "clicked", G_CALLBACK(on_open_file_clicked), window);
  gtk_box_append(GTK_BOX(box), open_file_button);

  gtk_widget_show(window);
}

int main(int argc, char * *argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("com.example.GtkApplication", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}