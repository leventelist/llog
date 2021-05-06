#include <gtk/gtk.h>

#include "main_window.h"
#include "llog.h"


typedef struct {
	GtkWidget *w_txtvw_main;	// Pointer to text view object
	GtkWidget *logfile_choose;	// Pointer to file chooser dialog box
	GtkWidget *log_entry_list;	// Pointer to the log entry list
	GtkWidget *logged_list;		// Pointer to the logged elemnt list
} app_widgets;


/*Module variables*/
static app_widgets *widgets;


int main_window_draw(void) {

	int ret_val;

	ret_val = 0;

	GtkBuilder      *builder;
	GtkWidget       *window;
	widgets = g_slice_new(app_widgets);

	gtk_init(NULL, NULL);

	builder = gtk_builder_new_from_file("glade/main_window.glade");

	/*Get widget pointers*/
	window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
	widgets->logfile_choose = GTK_WIDGET(gtk_builder_get_object(builder, "logfile_choose"));
	widgets->logged_list = GTK_WIDGET(gtk_builder_get_object(builder, "logged_list"));

	gtk_builder_connect_signals(builder, widgets);

	g_object_unref(builder);

	gtk_widget_show(window);
	gtk_main();
	g_slice_free(app_widgets, widgets);

	return ret_val;
}

/*Main window callbacks*/

void on_menuitm_open_activate(GtkMenuItem *menuitem, app_widgets *app_wdgts)
{
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
			file_success = llog_open_db(file_name);
			if (file_success != 0) {
				printf("Error opening database.\n");
			}
        }
        g_free(file_name);
    }

    // Finished with the "Open Text File" dialog box, so hide it
    gtk_widget_hide(app_wdgts->logfile_choose);
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

int add_log_entry_to_list(log_entry_t *entry) {
	int ret_val = OK;

	char buff[LOG_ENTRY_LEN];
	GtkWidget *combobox;

	/*Create the text*/
	snprintf(buff, LOG_ENTRY_LEN, "%s %s %s", entry->call, entry->rxrst, entry->txrst);

	/*Create the textbox*/
	combobox = gtk_combo_box_text_new();

	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combobox), "wrwer", buff);

	/*Add to the listbox*/
	gtk_list_box_prepend(GTK_LIST_BOX(widgets->logged_list), NULL);

	return ret_val;
}

