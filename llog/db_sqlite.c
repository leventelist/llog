/*	This is llog, logger for Amateur Radio operations.
 *
 *	Copyright (C) 2017-2019 Levente Kovacs
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
 * leva@logonex.eu
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

	ret = sqlite3_open(llog->logfileFn, &llog->db);

	do {
		if (ret != SQLITE_OK) {
			printf("Error opening the log database '%s'.\n", llog->logfileFn);
			break;
		}
		sqlite3_busy_timeout(llog->db, DATABASE_TIMEOUT);
	} while (0);

	return ret;
}


int db_sqlite_close(llog_t *llog) {

	sqlite3_close(llog->db);

	return OK;
}


int lookupStation(llog_t *llog, stationEntry_t *station) {
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret, retVal = LLOG_ERR;
	int haveWork;

	haveWork = 1;

	sprintf(buff, "SELECT rowid, name FROM station WHERE name='%s' OR rowid=%lu ORDER BY rowid DESC LIMIT 1;", llog->station, strtoul(llog->station, NULL, 0));
	sqlite3_prepare_v2(llog->db, buff, -1, &sq3_stmt, NULL);
	station->id = 1;

	while (haveWork == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		station->id = sqlite3_column_int64(sq3_stmt, 0);
		strncpy(station->name, (char *) sqlite3_column_text(sq3_stmt, 1), NAME_LEN);
		printf("Using staion '%s'.", station->name);
		retVal = OK;
		break;
		case SQLITE_DONE:
		haveWork = 0;
		if (retVal != OK) {
			printf("\nUnkown station '%s'. Using the default.\n", llog->station);
		}
		break;
		case SQLITE_BUSY:
		retVal = LLOG_ERR;
		haveWork = 0;
		break;
		default:
		retVal = LLOG_ERR;
		haveWork = 0;
		printf("Error looking up statin. %s\n", sqlite3_errmsg(llog->db));
		break;
		}
	}

	sqlite3_finalize(sq3_stmt);

	return retVal;
}


/*int setStation(logEntry_t *entry) {
	int ret, retVal = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int haveWork;

	return retVal;
}*/


int checkDupQSO(llog_t *log, logEntry_t *entry) {
	int ret, retVal = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int haveWork = 1;


	sprintf(buff, "SELECT date, UTC FROM log WHERE call='%s';", entry->call);
	sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

	while (haveWork == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		printf("\nDUP QSO on %s at %sUTC.\n", sqlite3_column_text(sq3_stmt, 0), sqlite3_column_text(sq3_stmt, 1));
		retVal = OK;
		break;
		case SQLITE_DONE:
		haveWork = 0;
		retVal = OK;
		break;
		case SQLITE_BUSY:
		retVal = LLOG_ERR;
		haveWork = 0;
		break;
		default:
		retVal = LLOG_ERR;
		haveWork = 0;
		printf("Error looking up statin. %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	return retVal;
}


int getMaxNr(llog_t *log, logEntry_t *entry) {
	int ret, retVal = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int haveWork = 1;

	entry->tx_nr = 0;

	sprintf(buff, "SELECT txnr FROM log ORDER BY rowid DESC LIMIT 1;");
	sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

	while (haveWork == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		entry->tx_nr = sqlite3_column_int64(sq3_stmt, 0) + 1;
		retVal = OK;
		break;
		case SQLITE_DONE:
		haveWork = 0;
		retVal = OK;
		break;
		case SQLITE_BUSY:
		retVal = LLOG_ERR;
		haveWork = 0;
		break;
		default:
		retVal = LLOG_ERR;
		haveWork = 0;
		printf("Error looking up statin. %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	return retVal;
}

int setLogEntry(llog_t *log, logEntry_t *entry) {
	int ret, retVal = OK;
	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int haveWork = 1;
	struct tm bdt;
	char date[64];
	char time[64];

	gmtime_r(&(entry->tv.tv_sec), &bdt);
	sprintf(date, "%d-%02d-%02d", 1900+bdt.tm_year, 1+bdt.tm_mon, bdt.tm_mday);
	sprintf(time, "%02d%02d", bdt.tm_hour, bdt.tm_min);

	sprintf(buff, "INSERT INTO log (date, UTC, call, rxrst, txrst, rxnr, txnr, rxextra, txextra, QTH, name, QRA, QRG, mode, pwr, rxQSL, txQSL, comment, station) VALUES ('%s', '%s', '%s', '%s', '%s', %"PRIu64", %"PRIu64", '%s', '%s', '%s', '%s', '%s', %f, '%s', '%s', %"PRIu64", %"PRIu64", '%s', %"PRIu64");", date, time, entry->call, entry->rxrst, entry->txrst, entry->rx_nr, entry->tx_nr, entry->rx_x, entry->tx_x, entry->QTH, entry->name, entry->QRA, entry->QRG, entry->mode, entry->pwr, 0L, 0L, entry->comment, entry->stationId);
	sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

	while (haveWork == 1) {
		ret = sqlite3_step(sq3_stmt);
		switch (ret) {
		case SQLITE_ROW:
		break;
		case SQLITE_DONE:
		haveWork = 0;
		retVal = OK;
		break;
		case SQLITE_BUSY:
		retVal = LLOG_ERR;
		haveWork = 0;
		break;
		default:
		retVal = LLOG_ERR;
		haveWork = 0;
		printf("Error inserting log entry. %s\n", sqlite3_errmsg(log->db));
		break;
		}
	}

	sqlite3_finalize(sq3_stmt);

	return retVal;
}
