#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "llog.h"
#include "db_sqlite.h"


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


void llog_shutdown(void) {
	int ret_val = EXIT_SUCCESS;

	exit(ret_val);
}
