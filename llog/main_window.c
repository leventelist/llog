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
	GtkEntry *log_entries[LLOG_ENTRIES];
	GtkButton *log_button;
} app_widgets;


/*Module variables*/
static app_widgets *widgets;
static log_entry_t log_entry_data;


int on_window_main_destroy(void);
void on_utc_btn_clicked(void);

int main_window_draw(void) {

	int ret_val;
	int i;

	ret_val = 0;

	GtkBuilder      *builder;
	GtkWidget       *window;
	widgets = g_slice_new(app_widgets);

	gtk_init(NULL, NULL);

	for (i = 0; i < LLOG_COLUMNS; i++) {
		widgets->logged_list_column[i] = NULL;
		widgets->logged_list_renderer[i] = NULL;
	}

	for (i = 0; i < LLOG_ENTRIES; i++) {
		widgets->log_entries[i] = NULL;
	}

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

	/*Get the pointers of the entries*/
	widgets->log_entries[llog_entry_call] = GTK_ENTRY(gtk_builder_get_object(builder, "call_entry"));
	widgets->log_entries[llog_entry_date] = GTK_ENTRY(gtk_builder_get_object(builder, "date_entry"));
	widgets->log_entries[llog_entry_utc] = GTK_ENTRY(gtk_builder_get_object(builder, "utc_entry"));
	widgets->log_entries[llog_entry_rxrst] = GTK_ENTRY(gtk_builder_get_object(builder, "rx_rst_entry"));
	widgets->log_entries[llog_entry_txrst] = GTK_ENTRY(gtk_builder_get_object(builder, "tx_rst_entry"));
	widgets->log_entries[llog_entry_qth] = GTK_ENTRY(gtk_builder_get_object(builder, "qth_entry"));
	widgets->log_entries[llog_entry_name] = GTK_ENTRY(gtk_builder_get_object(builder, "name_entry"));
	widgets->log_entries[llog_entry_qra] = GTK_ENTRY(gtk_builder_get_object(builder, "qra_entry"));
	widgets->log_entries[llog_entry_qrg] = GTK_ENTRY(gtk_builder_get_object(builder, "qrg_entry"));
	widgets->log_entries[llog_entry_mode] = GTK_ENTRY(gtk_builder_get_object(builder, "mode_entry"));
	widgets->log_entries[llog_entry_power] = GTK_ENTRY(gtk_builder_get_object(builder, "power_entry"));
	widgets->log_entries[llog_entry_rxnr] = GTK_ENTRY(gtk_builder_get_object(builder, "rxnr_entry"));
	widgets->log_entries[llog_entry_txnr] = GTK_ENTRY(gtk_builder_get_object(builder, "txnr_entry"));
	widgets->log_entries[llog_entry_rxextra] = GTK_ENTRY(gtk_builder_get_object(builder, "rxextra_entry"));
	widgets->log_entries[llog_entry_txextra] = GTK_ENTRY(gtk_builder_get_object(builder, "txextra_entry"));
	widgets->log_entries[llog_entry_comment] = GTK_ENTRY(gtk_builder_get_object(builder, "comment_entry"));

	/*Buttons*/
	widgets->log_button = GTK_BUTTON(gtk_builder_get_object(builder, "log_btn"));
	widgets->log_button = GTK_BUTTON(gtk_builder_get_object(builder, "utc_btn"));

	/*Set default user data*/
	gtk_builder_connect_signals(builder, widgets);

	g_object_unref(builder);

	/*Let's rock!*/
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

void on_window_main_entry_changed(GtkEntry *entry) {
	int entry_id;

	/*See which entry box has changed*/
	for (entry_id = 0; entry_id < LLOG_ENTRIES; entry_id++) {
		if (widgets->log_entries[entry_id] == entry) {
			break;
		}
	}

	switch (entry_id)
	{
	case llog_entry_call:
		printf("call entry changed\n");
		snprintf(log_entry_data.call, CALL_LEN, gtk_entry_get_text(entry));
		llog_strupper(log_entry_data.call);
		gtk_entry_set_text(entry, log_entry_data.call);
		on_utc_btn_clicked();
		break;
	case llog_entry_mode:

		break;
	case llog_entry_qra:
		snprintf(log_entry_data.qra, QRA_LEN, gtk_entry_get_text(entry));
		llog_strupper(log_entry_data.qra);
		gtk_entry_set_text(entry, log_entry_data.qra);
	default:
		break;
	}
}

void on_log_btn_clicked(void) {
	/*Gather log data*/
	snprintf(log_entry_data.date, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_date]));
	snprintf(log_entry_data.utc, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_utc]));
	snprintf(log_entry_data.rxrst, RST_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_rxrst]));
	snprintf(log_entry_data.txrst, RST_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_txrst]));
	snprintf(log_entry_data.qth, QTH_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_qth]));
	snprintf(log_entry_data.name, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_name]));
	snprintf(log_entry_data.qra, QRA_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_qra]));
	log_entry_data.qrg = atof(gtk_entry_get_text(widgets->log_entries[llog_entry_qrg]));
	snprintf(log_entry_data.mode.name, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_mode]));
	snprintf(log_entry_data.power, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_power]));
	log_entry_data.rxnr = strtoul(gtk_entry_get_text(widgets->log_entries[llog_entry_rxnr]), NULL, 0);
	log_entry_data.txnr = strtoul(gtk_entry_get_text(widgets->log_entries[llog_entry_txnr]), NULL, 0);
	snprintf(log_entry_data.rxextra, X_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_rxextra]));
	snprintf(log_entry_data.txextra, X_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_txextra]));
	snprintf(log_entry_data.comment, X_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_comment]));

	llog_print_log_data(&log_entry_data);
}

void on_utc_btn_clicked(void) {
	llog_get_time(&log_entry_data);
	gtk_entry_set_text(widgets->log_entries[llog_entry_date], log_entry_data.date);
	gtk_entry_set_text(widgets->log_entries[llog_entry_utc], log_entry_data.utc);
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
	gtk_tree_store_set(widgets->logged_list_store, &iter, 3, entry->qrg, -1);
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
