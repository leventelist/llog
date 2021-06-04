/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2021  Levente Kovacs
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <gtk/gtk.h>
#include <inttypes.h>
#include "main_window.h"
#include "llog.h"
#include "llog_Config.h"

#define BUFF_SIZ 1024

typedef struct {
	GtkWindow *main_window;
	GtkWidget *w_txtvw_main;	// Pointer to text view object
	GtkWidget *logfile_choose;	// Pointer to file chooser dialog box
	GtkWidget *log_entry_list;	// Pointer to the log entry list
	GtkTreeView *logged_list;		// Pointer to the logged elemnt list
	GtkTreeStore *logged_list_store;
	GtkTreeSelection *logged_list_selection;
	GtkTreeViewColumn *logged_list_column[LLOG_COLUMNS];
	GtkCellRenderer *logged_list_renderer[LLOG_COLUMNS];
	GtkListStore *station_list_store;
	GtkListStore *mode_list_store;
	GtkEntry *log_entries[LLOG_ENTRIES];
	GtkButton *log_button;
	GtkComboBox *mode_entry;
	GtkComboBox *station_entry;
	GtkLabel *call_label;
	GtkAboutDialog *about_dialog;
} app_widgets;


/*Module variables*/
static app_widgets *widgets;
static log_entry_t log_entry_data;


void on_window_main_destroy(void);
void on_utc_btn_clicked(void);
void on_mode_entry_change(GtkEntry *entry);
void set_static_data(void);

int main_window_draw(void) {

	int ret_val;
	int i;

	ret_val = 0;

	GtkBuilder      *builder;

	widgets = g_slice_new(app_widgets);

	gtk_init(NULL, NULL);

	for (i = 0; i < LLOG_COLUMNS; i++) {
		widgets->logged_list_column[i] = NULL;
		widgets->logged_list_renderer[i] = NULL;
	}

	for (i = 0; i < LLOG_ENTRIES; i++) {
		widgets->log_entries[i] = NULL;
	}

	builder = gtk_builder_new_from_file(LLOG_GLADE);

	/*Get widget pointers*/
	widgets->main_window = GTK_WINDOW(gtk_builder_get_object(builder, "window_main"));
	g_signal_connect(widgets->main_window, "destroy", G_CALLBACK(on_window_main_destroy), NULL);
	widgets->logfile_choose = GTK_WIDGET(gtk_builder_get_object(builder, "logfile_choose"));
	widgets->logged_list = GTK_TREE_VIEW(gtk_builder_get_object(builder, "logged_list"));
	widgets->logged_list_selection = GTK_TREE_SELECTION(gtk_builder_get_object(builder, "logged_list_selection"));
	widgets->station_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "station_list_store"));
	widgets->mode_list_store = GTK_LIST_STORE(gtk_builder_get_object(builder, "mode_list_store"));

	widgets->about_dialog = GTK_ABOUT_DIALOG(gtk_builder_get_object(builder, "about_dialog"));

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

	/*Get the station ID. This is not really a log entry.*/
	widgets->log_entries[llog_entry_station_id] = GTK_ENTRY(gtk_builder_get_object(builder, "station_select_entry"));

	/*Buttons*/
	widgets->log_button = GTK_BUTTON(gtk_builder_get_object(builder, "log_btn"));
	widgets->log_button = GTK_BUTTON(gtk_builder_get_object(builder, "utc_btn"));

	/*Labels*/
	widgets->call_label = GTK_LABEL(gtk_builder_get_object(builder, "call_label"));

	/*Set default user data*/
	gtk_builder_connect_signals(builder, widgets);

	g_object_unref(builder);

	/*Let's rock!*/
	gtk_widget_show(GTK_WIDGET(widgets->main_window));

	llog_load_static_data(&log_entry_data);
	set_static_data();
	

	gtk_main();
	g_slice_free(app_widgets, widgets);

	return ret_val;
}


void set_static_data(void) {
	char buff[BUFF_SIZ];

	/*Set txnr*/
	snprintf(buff, BUFF_SIZ, "%04" PRIu64, log_entry_data.txnr);
	gtk_entry_set_text(widgets->log_entries[llog_entry_txnr], buff);
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
			llog_load_static_data(&log_entry_data);
			set_static_data();
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
	int ret;

	/*See which entry box has changed*/
	for (entry_id = 0; entry_id < LLOG_ENTRIES; entry_id++) {
		if (widgets->log_entries[entry_id] == entry) {
			break;
		}
	}

	//printf("Entry changed\n");

	switch (entry_id) {
		case llog_entry_call:
			snprintf(log_entry_data.call, CALL_LEN, gtk_entry_get_text(entry));
			llog_strupper(log_entry_data.call);
			gtk_entry_set_text(entry, log_entry_data.call);
			/*Get time*/
			on_utc_btn_clicked();
			/*Check for dup QSO*/
			ret = llog_check_dup_qso(&log_entry_data);
			switch (ret)
			{
			case OK: /*New QSO*/
				gtk_label_set_label(widgets->call_label, "Call");
				break;
			case LLOG_DUP: /*DUP QSO*/
				gtk_label_set_label(widgets->call_label, "Call [DUP]");
			break;
			default: /*ERROR*/
				break;
			}
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


void on_mode_entry_change(GtkEntry *entry) {

	llog_get_default_rst(log_entry_data.txrst, (char *)gtk_entry_get_text(entry));

	gtk_entry_set_text(widgets->log_entries[llog_entry_txrst], log_entry_data.txrst);

}


void on_log_btn_clicked(void) {
	int ret;
	char buff[BUFF_SIZ];
	GtkDialogFlags 	flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	GtkWidget *error_dialog;

	/*Gather log data*/
	snprintf(log_entry_data.date, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_date]));
	snprintf(log_entry_data.utc, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_utc]));
	snprintf(log_entry_data.rxrst, RST_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_rxrst]));
	snprintf(log_entry_data.txrst, RST_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_txrst]));
	snprintf(log_entry_data.qth, QTH_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_qth]));
	snprintf(log_entry_data.name, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_name]));
	snprintf(log_entry_data.qra, QRA_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_qra]));
	log_entry_data.qrg = atof(gtk_entry_get_text(widgets->log_entries[llog_entry_qrg]));

	llog_tokenize(gtk_entry_get_text(widgets->log_entries[llog_entry_mode]), log_entry_data.mode.name, NULL);

	snprintf(log_entry_data.power, NAME_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_power]));
	log_entry_data.rxnr = strtoul(gtk_entry_get_text(widgets->log_entries[llog_entry_rxnr]), NULL, 0);
	log_entry_data.txnr = strtoul(gtk_entry_get_text(widgets->log_entries[llog_entry_txnr]), NULL, 0);
	snprintf(log_entry_data.rxextra, X_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_rxextra]));
	snprintf(log_entry_data.txextra, X_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_txextra]));
	snprintf(log_entry_data.comment, X_LEN, gtk_entry_get_text(widgets->log_entries[llog_entry_comment]));

	llog_tokenize(gtk_entry_get_text(widgets->log_entries[llog_entry_station_id]), NULL, &log_entry_data.station_id);

	/*This is for debug. Print log data to stdout*/
	//llog_print_log_data(&log_entry_data);

	/*Sanity chack of the data*/

	if (strlen(log_entry_data.call) < 2) {
		printf("Not logging. Call too short.\n");
		error_dialog = gtk_message_dialog_new(widgets->main_window, flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Call must be longer then 2 characters.");
		gtk_dialog_run(GTK_DIALOG(error_dialog));
		gtk_widget_destroy (error_dialog);
		return;
	}

	/*Write log entry to the DB */
	ret = llog_log_entry(&log_entry_data);

	switch (ret) {
		case OK:
			/* Increment the conter. */
			log_entry_data.txnr++;
			break;
		case LLOG_ERR:
			/*Display some error message.*/
			error_dialog = gtk_message_dialog_new(widgets->main_window, flags, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error logging the QSO!");
			gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(error_dialog), "Database is not ready.");
			gtk_dialog_run(GTK_DIALOG(error_dialog));
			gtk_widget_destroy(error_dialog);
			break;
		break;
		default:
		break;
	}

	llog_reset_entry(&log_entry_data);

	gtk_entry_set_text(widgets->log_entries[llog_entry_call], log_entry_data.call);
	gtk_entry_set_text(widgets->log_entries[llog_entry_date], log_entry_data.date);
	gtk_entry_set_text(widgets->log_entries[llog_entry_utc], log_entry_data.utc);
	gtk_entry_set_text(widgets->log_entries[llog_entry_rxrst], log_entry_data.rxrst);

	on_mode_entry_change(widgets->log_entries[llog_entry_mode]);

	gtk_entry_set_text(widgets->log_entries[llog_entry_qth], log_entry_data.qth);
	gtk_entry_set_text(widgets->log_entries[llog_entry_name], log_entry_data.name);
	gtk_entry_set_text(widgets->log_entries[llog_entry_qra], log_entry_data.qra);
	gtk_entry_set_text(widgets->log_entries[llog_entry_power], log_entry_data.power);
	gtk_entry_set_text(widgets->log_entries[llog_entry_rxextra], log_entry_data.rxextra);
	gtk_entry_set_text(widgets->log_entries[llog_entry_comment], log_entry_data.comment);

	snprintf(buff, BUFF_SIZ, "%04" PRIu64, log_entry_data.txnr);
	gtk_entry_set_text(widgets->log_entries[llog_entry_txnr], buff);

	/*Refresh the log list*/
	llog_add_log_entries();
}


void on_utc_btn_clicked(void) {
	llog_get_time(&log_entry_data);
	gtk_entry_set_text(widgets->log_entries[llog_entry_date], log_entry_data.date);
	gtk_entry_set_text(widgets->log_entries[llog_entry_utc], log_entry_data.utc);
}


void on_window_main_destroy(void) {

	gtk_main_quit();
}


void on_qrt_activate(void) {

	gtk_main_quit();
}


void on_reload_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts) {

	(void)menuitem;
	(void)app_wdgts;

	llog_shutdown();
	llog_open_db();
	llog_load_static_data(&log_entry_data);
	set_static_data();
}


void on_about_dialog_response(GtkMenuItem *menuitem, gint response_id, app_widgets *app_wdgts) {

	(void)response_id;
	(void)menuitem;

	gtk_widget_hide((GtkWidget*)app_wdgts->about_dialog);
}


void on_about_menu_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts) {

	(void)menuitem;

	gtk_widget_show((GtkWidget*)app_wdgts->about_dialog);

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


int main_window_add_mode_entry_to_list(mode_entry_t *mode) {
	int ret_val = OK;
	char buff[LOG_ENTRY_LEN];

	static GtkTreeIter iter;

	sprintf(buff, "%s [%lu]", mode->name, mode->id);
	printf("Adding %s to the mode list.\n", buff);

	gtk_list_store_prepend(widgets->mode_list_store, &iter);
	gtk_list_store_set(widgets->mode_list_store, &iter, 0, buff, -1);

	return ret_val;
}


void main_window_clear_log_list(void) {
	gtk_tree_store_clear(widgets->logged_list_store);
}


void main_window_clear_station_list(void) {
	gtk_list_store_clear(widgets->station_list_store);
}


void main_window_clear_modes_list(void) {
	gtk_list_store_clear(widgets->mode_list_store);
}
