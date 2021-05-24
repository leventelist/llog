#include <gtk/gtk.h>

#include "main_window.h"
#include "llog.h"


typedef struct {
	GtkWidget *w_txtvw_main;	// Pointer to text view object
	GtkWidget *logfile_choose;	// Pointer to file chooser dialog box
	GtkWidget *log_entry_list;	// Pointer to the log entry list
	GtkTreeView *logged_list;		// Pointer to the logged elemnt list
	GtkTreeStore *logged_list_store;
	GtkTreeSelection *logged_list_selection;
	GtkTreeViewColumn *logged_list_column[LLOG_COLUMNS];
	GtkCellRenderer *logged_list_renderer[LLOG_COLUMNS];
	GtkListStore *station_list_store;
	GtkEntry log_entries[LLOG_COLUMNS];
} app_widgets;


/*Module variables*/
static app_widgets *widgets;


int on_window_main_destroy(void);

int main_window_draw(void) {

	int ret_val;

	ret_val = 0;

	GtkBuilder      *builder;
	GtkWidget       *window;
	widgets = g_slice_new(app_widgets);

	gtk_init(NULL, NULL);

	builder = gtk_builder_new_from_file("./glade/main_window.glade");

	/*Get widget pointers*/
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	g_signal_connect(window, "destroy", G_CALLBACK(on_window_main_destroy), NULL);
	widgets->logfile_choose = GTK_WIDGET(gtk_builder_get_object(builder, "logfile_choose"));
	widgets->logged_list = GTK_TREE_VIEW(gtk_builder_get_object(builder, "logged_list"));
	widgets->logged_list_selection = GTK_TREE_SELECTION(gtk_builder_get_object(builder, "logged_list_selection"));
	widgets->station_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "station_list_store"));

	/*Putting together the log list*/
	widgets->logged_list_store = GTK_TREE_STORE(gtk_builder_get_object(builder, "logged_list_store"));

	widgets->logged_list_column[0] = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "id"));
	widgets->logged_list_column[1] = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "call"));
	widgets->logged_list_column[2] = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "date"));
	widgets->logged_list_column[3] = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "QRG"));
	widgets->logged_list_column[4] = GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(builder, "mode"));

	widgets->logged_list_renderer[0] = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "id_text"));
	widgets->logged_list_renderer[1] = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "call_text"));
	widgets->logged_list_renderer[2] = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "date_text"));
	widgets->logged_list_renderer[3] = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "QRG_text"));
	widgets->logged_list_renderer[4] = GTK_CELL_RENDERER(gtk_builder_get_object(builder, "mode_text"));

	gtk_tree_view_column_add_attribute(widgets->logged_list_column[0], widgets->logged_list_renderer[0], "text", 0);
	gtk_tree_view_column_add_attribute(widgets->logged_list_column[1], widgets->logged_list_renderer[1], "text", 1);
	gtk_tree_view_column_add_attribute(widgets->logged_list_column[2], widgets->logged_list_renderer[2], "text", 2);
	gtk_tree_view_column_add_attribute(widgets->logged_list_column[3], widgets->logged_list_renderer[3], "text", 3);
	gtk_tree_view_column_add_attribute(widgets->logged_list_column[4], widgets->logged_list_renderer[4], "text", 4);

	gtk_builder_connect_signals(builder, widgets);

	g_object_unref(builder);

	gtk_widget_show(window);

	llog_add_log_entries();
	llog_add_station_entries();

	gtk_main();
	g_slice_free(app_widgets, widgets);

	return ret_val;
}

/*Main window callbacks*/

void on_menuitm_open_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts) {
    gchar *file_name = NULL;        // Name of file to open from dialog box
    int file_success;  // File read status

	(void)menuitem;

    // Show the "Open Text File" dialog box
    gtk_widget_show(app_wdgts->logfile_choose);

    // Check return value from Open Text File dialog box to see if user clicked the Open button
    if (gtk_dialog_run(GTK_DIALOG (app_wdgts->logfile_choose)) == GTK_RESPONSE_OK) {
        // Get the file name from the dialog box
        file_name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(app_wdgts->logfile_choose));
        if (file_name != NULL) {
			printf("Yaayyy!!! We know which file to load!\n");
			llog_init(file_name);
			file_success = llog_open_db();
			llog_add_log_entries();
			llog_add_station_entries();
			if (file_success != 0) {
				printf("Error opening database.\n");
				return;
			}
        }

        g_free(file_name);
    }

    // Finished with the "Open Text File" dialog box, so hide it
    gtk_widget_hide(app_wdgts->logfile_choose);
}

int on_window_main_entry_changed(GtkEntry *entry) {
	char buff[LOG_ENTRY_LEN];

	sprintf(buff, gtk_entry_get_text(entry));

	printf("Entry text: %s\n", buff);

}

int on_window_main_destroy(void) {

	gtk_main_quit();
//	llog_shutdown();
	return 0;
}

int on_qrt_activate(void) {

	gtk_main_quit();
//	llog_shutdown();
	return 0;
}


/*Actions*/

int main_window_add_log_entry_to_list(log_entry_t *entry) {
	int ret_val = OK;
	static GtkTreeIter iter;
	char buff[LOG_ENTRY_LEN];

	gtk_tree_store_prepend(widgets->logged_list_store, &iter, NULL);
	/*Create the text*/
	snprintf(buff, LOG_ENTRY_LEN, "%s %s %s", entry->call, entry->rxrst, entry->txrst);

	printf("Adding log item... %s\n", buff);

	sprintf(buff, "%lu", entry->id);

	gtk_tree_store_set(widgets->logged_list_store, &iter, 0, buff, -1);
	gtk_tree_store_set(widgets->logged_list_store, &iter, 1, entry->call, -1);
	gtk_tree_store_set(widgets->logged_list_store, &iter, 2, entry->date, -1);
	gtk_tree_store_set(widgets->logged_list_store, &iter, 3, entry->QRG, -1);
	gtk_tree_store_set(widgets->logged_list_store, &iter, 4, entry->mode.name, -1);


	return ret_val;
}


int main_window_add_station_entry_to_list(station_entry_t *station) {
	int ret_val = OK;
	char buff[LOG_ENTRY_LEN];

	static GtkTreeIter iter;

	sprintf(buff, "%s [%lu]", station->name, station->id);
	printf("Adding %s to the station list.\n", buff);

	gtk_list_store_prepend(widgets->station_list_store, &iter);
	gtk_list_store_set(widgets->station_list_store, &iter, 0, buff, -1);

	return ret_val;
}


void main_window_clear_log_list(void) {
	gtk_tree_store_clear(widgets->logged_list_store);
}

void main_window_clear_station_list(void) {
	gtk_list_store_clear(widgets->station_list_store);
}
