/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2021  Levente Kovacs
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "llog.h"
#include "db_sqlite.h"
#include "main_window.h"

#define BUF_SIZ 1024

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


int llog_add_log_entries(void) {

	int ret_val = OK;
	log_entry_t entry;

	entry.data_stat = db_data_init;

	main_window_clear_log_list();

	while (entry.data_stat != db_data_last) {
		ret_val = db_get_log_entries(&llog, &entry);
		if (ret_val != OK) {
			break;
		}
		if (entry.data_stat == db_data_valid) {
			main_window_add_log_entry_to_list(&entry);
		} else {
			break;
		}
	}

	return ret_val;
}


int llog_log_entry(log_entry_t *entry) {
	int ret_val = OK;

	ret_val = db_set_log_entry(&llog, entry);

	return ret_val;
}


void llog_reset_entry(log_entry_t *entry) {

	entry->date[0] = '\0';
	entry->utc[0] = '\0';
	entry->rxrst[0] = '\0';
	entry->call[0] = '\0';
	entry->name[0] = '\0';

	if (entry->mode.id !=0 ) {
		strcpy(entry->txrst, entry->mode.default_rst);
	}

	entry->rxrst[0] = '\0';
	entry->qth[0] = '\0';
	entry->qra[0] = '\0';
	entry->rxnr = 0;
	entry->rxextra[0] = '\0';
	entry->txextra[0] = '\0';
	entry->comment[0] = '\0';

}


int llog_add_station_entries(void) {

	int ret_val = OK;
	station_entry_t station;

	station.data_stat = db_data_init;

	main_window_clear_station_list();

	while (station.data_stat != db_data_last) {
		db_get_station_entry(&llog, &station);
		if (station.data_stat == db_data_valid) {
			main_window_add_station_entry_to_list(&station);
		} else {
			break;
		}
	}

	return ret_val;
}


int llog_add_modes_entries(void) {
	int ret_val = OK;
	mode_entry_t mode;

	mode.data_stat = db_data_init;

	main_window_clear_modes_list();

	while (mode.data_stat != db_data_last) {
		db_get_mode_entry(&llog, &mode, NULL);
		if (mode.data_stat == db_data_valid) {
			main_window_add_mode_entry_to_list(&mode);
		} else {
			break;
		}
	}

	return ret_val;
}


int llog_get_default_rst(char *default_rst, char *mode_string) {
	uint64_t mode_id;
	mode_entry_t mode;
	int ret;

	mode.data_stat = db_data_init;

	llog_tokenize(mode_string, NULL, &mode_id);
	ret = db_get_mode_entry(&llog, &mode, &mode_id);
	if (ret == OK) {
		strncpy(default_rst, mode.default_rst, MODE_LEN);
	}
	
	return ret;
}


int llog_get_max_nr(log_entry_t *entry) {
	int ret;

	ret = db_get_max_nr(&llog, entry);

	return ret;
}


void llog_get_time(log_entry_t *entry) {
	struct timeval qso_time;
	struct tm qso_bdt;

	gettimeofday(&qso_time, NULL);
	gmtime_r(&qso_time.tv_sec, &qso_bdt);
	sprintf(entry->date, "%d-%02d-%02d", 1900+qso_bdt.tm_year, 1+qso_bdt.tm_mon, qso_bdt.tm_mday);
	sprintf(entry->utc, "%02d%02d", qso_bdt.tm_hour, qso_bdt.tm_min);
}


int llog_check_dup_qso(log_entry_t *entry) {
	int ret_val;

	entry->data_stat = db_data_init;

	ret_val = db_check_dup_qso(&llog, entry);

	return ret_val;
}


int llog_load_static_data(log_entry_t *entry) {
	int ret;

	do {
		ret = llog_add_log_entries();
		if (ret != OK) {
			break;
		}
		ret = llog_add_station_entries();
		if (ret != OK) {
			break;
		}
		ret = llog_add_modes_entries();
		if (ret != OK) {
			break;
		}
		ret = llog_get_max_nr(entry);
		if (ret != OK) {
			break;
		}
	} while (0);

	return ret;
}


void llog_print_log_data(log_entry_t *entry) {

	printf("\nCall [%s]\nOperator's name: [%s]\n", entry->call, entry->name);
	printf("RXRST [%s]\nTXRST [%s]\n", entry->rxrst, entry->txrst);
	printf("QTH [%s]\nQRA [%s]\n", entry->qth, entry->qra);
	printf("QRG [%f]\nMode [%s]\nPower: [%s]\n", entry->qrg, entry->mode.name, entry->power);
	printf("RXNR [%04"PRIu64"]\nTXNR [%04"PRIu64"]\n", entry->rxnr, entry->txnr);
	printf("RX_EXTRA [%s]\nTX_EXTRA [%s]\n", entry->rxextra, entry->txextra);
	printf("Comment [%s]\n\n", entry->comment);
	return;
}


void llog_strupper(char *s) {
	while (*s) {
		if ((*s >= 'a' ) && (*s <= 'z')) {
			*s -= ('a'-'A');
		}
		s++;
	}
}


void llog_tokenize(const char *in, char *out, uint64_t *id) {
	uint64_t fake_id;
	char buff[BUF_SIZ];

	if (out == NULL) {
		out = buff;
	}

	if (id == NULL) {
		id = &fake_id;
	}

	sscanf(in, "%s [%" PRIu64 "]", out, id);
}


void llog_shutdown(void) {
	db_close(&llog);
}
