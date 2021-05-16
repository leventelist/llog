#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "llog.h"
#include "db_sqlite.h"
#include "main_window.h"


static llog_t llog;

int llog_init(void) {

	llog.db = NULL;
	llog.logfileFn[0] = '\0';

	return 0;
}


int llog_open_db(char *fn) {

	int ret;

	strncpy(llog.logfileFn, fn, LOGFILE_LEN);

	printf("Opening log file %s\n", llog.logfileFn);

	ret = db_sqlite_init(&llog);

	return ret;

}


int llog_add_log_entry(void) {

	int ret_val = OK;
	log_entry_t entry;

	entry.data_stat = DATA_STATUS_INIT;

	while(entry.data_stat != DATA_STATUS_LAST) {
		db_get_log_entries(&llog, &entry);
		if (entry.data_stat == DATA_STATUS_VALID) {
			main_window_add_log_entry_to_list(&entry);
		}
	}

	return ret_val;
}


void llog_shutdown(void) {
	int ret_val = EXIT_SUCCESS;

	exit(ret_val);
}
