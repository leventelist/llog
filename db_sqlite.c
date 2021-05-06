/*	This is llog, logger for Amateur Radio operations.
 *
 *	Copyright (C) 2017-2021 Levente Kovacs
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
 *
 * http://levente.logonex.eu
 * lev@logonex.eu
 *
 */

/* To aid portability the only SQL statements is used is INSERT, SELECT,
 * DELETE, UPDATE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "db_sqlite.h"
#include "llog.h"

#include <inttypes.h>
#include <string.h>
#include <stdio.h>


int db_sqlite_init(llog_t *llog) {

	int ret;
	int ret_val = OK;

	ret = sqlite3_open(llog->logfileFn, &llog->db);

	do {
		if (ret != SQLITE_OK) {
			printf("Error opening the log database '%s'.\n", llog->logfileFn);
			ret_val = FILE_ERR;
			break;
		}
		sqlite3_busy_timeout(llog->db, DATABASE_TIMEOUT);
	} while (0);

	return ret_val;
}


int db_sqlite_close(llog_t *llog) {

	sqlite3_close(llog->db);

	return OK;
}


int lookupStation(llog_t *llog, station_entry_t *station) {
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret, ret_val = LLOG_ERR;
	int have_work;

	have_work = 1;

	sprintf(buff, "SELECT rowid, name FROM station WHERE name='%s' OR rowid=%lu ORDER BY rowid DESC LIMIT 1;", llog->station, strtoul(llog->station, NULL, 0));
	sqlite3_prepare_v2(llog->db, buff, -1, &sq3_stmt, NULL);
	station->id = 1;

	while (have_work == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		station->id = sqlite3_column_int64(sq3_stmt, 0);
		strncpy(station->name, (char *) sqlite3_column_text(sq3_stmt, 1), NAME_LEN);
		printf("\nUsing staion '%s'.\n", station->name);
		ret_val = OK;
		break;
		case SQLITE_DONE:
		have_work = 0;
		if (ret_val != OK) {
			printf("\nUnkown station '%s'. Using the default.\n", llog->station);
		}
		break;
		case SQLITE_BUSY:
		ret_val = LLOG_ERR;
		have_work = 0;
		break;
		default:
		ret_val = LLOG_ERR;
		have_work = 0;
		printf("Error looking up station: %s\n", sqlite3_errmsg(llog->db));
		break;
		}
	}

	sqlite3_finalize(sq3_stmt);

	return ret_val;
}


/*int setStation(log_entry_t *entry) {
	int ret, ret_val = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int have_work;

	return ret_val;
}*/

int checkDupQSO(llog_t *log, log_entry_t *entry) {
	int ret, ret_val = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int have_work = 1;


	sprintf(buff, "SELECT date, UTC FROM log WHERE call='%s';", entry->call);
	sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

	while (have_work == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		printf("\nDUP QSO on %s at %sUTC.\n", sqlite3_column_text(sq3_stmt, 0), sqlite3_column_text(sq3_stmt, 1));
		ret_val = OK;
		break;
		case SQLITE_DONE:
		have_work = 0;
		ret_val = OK;
		break;
		case SQLITE_BUSY:
		ret_val = LLOG_ERR;
		have_work = 0;
		break;
		default:
		ret_val = LLOG_ERR;
		have_work = 0;
		printf("Error looking up DUP QSOs: %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	return ret_val;
}

int get_log_entries(llog_t *log, log_entry_t *entry) {
	int ret, ret_val = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int have_work = 1;

	if (entry->data_stat == DATA_STATUS_INIT) {
		snprintf(buff, BUF_SIZ, "SELECT date, UTC, call, rxrst, txrst FROM log;");
		sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);
	}

	while (have_work == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		strncpy(entry->date, (char *)sqlite3_column_text(sq3_stmt, 0), NAME_LEN);
		strncpy(entry->utc, (char *)sqlite3_column_text(sq3_stmt, 1), NAME_LEN);
		strncpy(entry->call, (char *)sqlite3_column_text(sq3_stmt, 2), CALL_LEN);
		strncpy(entry->rxrst, (char *)sqlite3_column_text(sq3_stmt, 3), X_LEN);
		strncpy(entry->txrst, (char *)sqlite3_column_text(sq3_stmt, 4), X_LEN);

		ret_val = OK;
		entry->data_stat = DATA_STATUS_VALID;
		break;
		case SQLITE_DONE:
		have_work = 0;
		ret_val = OK;
		entry->data_stat = DATA_STATUS_LAST;
		break;
		case SQLITE_BUSY:
		ret_val = LLOG_ERR;
		have_work = 0;
		break;
		default:
		ret_val = LLOG_ERR;
		have_work = 0;
		printf("Error looking up DUP QSOs: %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	return ret_val;
}


int getMaxNr(llog_t *log, log_entry_t *entry) {
	int ret, ret_val = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int have_work = 1;

	entry->tx_nr = 0;

	sprintf(buff, "SELECT txnr FROM log ORDER BY rowid DESC LIMIT 1;");
	sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

	while (have_work == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		entry->tx_nr = sqlite3_column_int64(sq3_stmt, 0) + 1;
		ret_val = OK;
		break;
		case SQLITE_DONE:
		have_work = 0;
		ret_val = OK;
		break;
		case SQLITE_BUSY:
		ret_val = LLOG_ERR;
		have_work = 0;
		break;
		default:
		ret_val = LLOG_ERR;
		have_work = 0;
		printf("Error looking up serial number: %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	return ret_val;
}


int setLogEntry(llog_t *log, log_entry_t *entry) {
	int ret, ret_val = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int have_work = 1;

	snprintf(buff, BUF_SIZ, "INSERT INTO log (date, UTC, call, rxrst, txrst, rxnr, txnr, rxextra, txextra, QTH, name, QRA, QRG, mode, pwr, rxQSL, txQSL, comment, station) VALUES ('%s', '%s', '%s', '%s', '%s', %"PRIu64", %"PRIu64", '%s', '%s', '%s', '%s', '%s', %f, '%s', '%s', %"PRIu64", %"PRIu64", '%s', %"PRIu64");", entry->date, entry->utc, entry->call, entry->rxrst, entry->txrst, entry->rx_nr, entry->tx_nr, entry->rx_x, entry->tx_x, entry->QTH, entry->name, entry->QRA, entry->QRG, entry->mode.name, entry->pwr, (uint64_t)0U, (uint64_t)0U, entry->comment, entry->stationId);
	sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

	while (have_work == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		break;
		case SQLITE_DONE:
		have_work = 0;
		ret_val = OK;
		break;
		case SQLITE_BUSY:
		ret_val = LLOG_ERR;
		have_work = 0;
		break;
		default:
		ret_val = LLOG_ERR;
		have_work = 0;
		printf("Error inserting log entry: %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	sqlite3_finalize(sq3_stmt);

	return ret_val;
}


int list_stations(llog_t *llog) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret, ret_val = LLOG_ERR;
	int have_work;

	have_work = 1;

	sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant FROM station ORDER BY rowid ASC;");
	sqlite3_prepare_v2(llog->db, buff, -1, &sq3_stmt, NULL);

	printf("\n\n\nAvailable stations: \n");

	while (have_work == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		printf("\n");
		printf("ID: %llu", sqlite3_column_int64(sq3_stmt, 0));
		printf("\tname: %s", sqlite3_column_text(sq3_stmt, 1));
		printf("\tCall: %s", sqlite3_column_text(sq3_stmt, 2));
		printf("\tQTH: %s", sqlite3_column_text(sq3_stmt, 3));
		printf("\tQRA: %s", sqlite3_column_text(sq3_stmt, 4));
		printf("\tASL: %s", sqlite3_column_text(sq3_stmt, 5));
		printf("\tRIG: %s", sqlite3_column_text(sq3_stmt, 6));
		printf("\tANT: %s", sqlite3_column_text(sq3_stmt, 7));
		ret_val = OK;
		break;
		case SQLITE_DONE:
		have_work = 0;
		printf("\n\n");
		break;
		case SQLITE_BUSY:
		ret_val = LLOG_ERR;
		have_work = 0;
		break;
		default:
		ret_val = LLOG_ERR;
		have_work = 0;
		printf("Error looking up stations: %s\n", sqlite3_errmsg(llog->db));
		break;
		}
	}

	sqlite3_finalize(sq3_stmt);

	return ret_val;
}
