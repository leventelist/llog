#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "llog.h"
#include "db_sqlite.h"
#include "main_window.h"


static llog_t llog;

int llog_init(char *logfile_name) {

	llog.db = NULL;
	strncpy(llog.logfileFn, logfile_name, LOGFILE_LEN);
	llog.stat = db_closed;

	return 0;
}


int llog_open_db(void) {

	int ret;

	printf("Opening log file %s\n", llog.logfileFn);

	ret = db_sqlite_init(&llog);

	return ret;

}


int llog_add_log_entry(void) {

	int ret_val = OK;
	log_entry_t entry;

	entry.data_stat = db_data_init;

	main_window_clear_log_list();

	while(entry.data_stat != db_data_last) {
		db_get_log_entries(&llog, &entry);
		if (entry.data_stat == db_data_valid) {
			main_window_add_log_entry_to_list(&entry);
		} else {
			break;
		}
	}

	return ret_val;
}


void llog_shutdown(void) {
	int ret_val = EXIT_SUCCESS;

	exit(ret_val);
}
