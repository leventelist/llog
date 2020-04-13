/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2019  Levente Kovacs
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

#ifndef LLOG_H
#define LLOG_H

#define VERSION "v1.3.0"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <sqlite3.h>

#define DATABASE_TIMEOUT 1000

#define LOGFILE_LEN 100
#define QTH_LEN 100
#define QRA_LEN 20
#define QRG_LEN 20
#define MODE_LEN 20
#define RIG_LEN 50
#define ANT_LEN 20
#define RST_LEN 10
#define CALL_LEN 40
#define NAME_LEN 100
#define COMMENT_LEN 200
#define QSL_LEN 10
#define ASL_LEN 20
#define PWR_LEN 20
#define LIST_LEN 50
#define LINE_LEN 1024
#define SUBSTR_LEN 512
#define X_LEN 40
#define STATION_LEN 256

#define OK 0
#define FILE_ERR 1
#define CMD_LINE_ERR 2

#define LLOG_EOF 1
#define LLOG_CANCEL 2
#define LLOG_ERR 3

#define PROMPT "\n: "

/*CSV stuff*/
#define CSV_LIST_LEN 23
/*CSV field positions*/
#define CSV_DATE_POS 0
#define CSV_TIME_POS 1
#define CSV_CALL_POS 2
#define CSV_RXRST_POS 3
#define CSV_TXRST_POS 4
#define CSV_QTH_POS 5
#define CSV_QRA_POS 6
#define CSV_NAME_POS 7
#define CSV_QRG_POS 8
#define CSV_MODE_POS 9
#define CSV_RXNR_POS 10
#define CSV_TXNR_POS 11
#define CSV_RXX_POS 12
#define CSV_TXX_POS 13
#define CSV_COMMENT_POS 14
#define CSV_QSL_POS 15
#define CSV_LOCAL_CALL_POS 16
#define CSV_LOCAL_QTH_POS 17
#define CSV_LOCAL_QRA_POS 18
#define CSV_LOCAL_ALT_POS 19
#define CSV_LOCAL_RIG_POS 20
#define CSV_LOCAL_PWR_POS 21
#define CSV_LOCAL_ANT_POS 22

typedef struct {
	char defaultRst[RST_LEN];
	char logfileFn[LOGFILE_LEN]; /*SQLite database file name*/
	char station[STATION_LEN];
	sqlite3 *db;
} llog_t;


typedef struct {
	char call[CALL_LEN];
	char rxrst[RST_LEN];
	char txrst[RST_LEN];
	uint64_t tx_nr;
	uint64_t rx_nr;
	char rx_x[X_LEN];
	char tx_x[X_LEN];
	char QTH[QTH_LEN];
	char name[NAME_LEN];
	char QRA[QRA_LEN];
	double QRG;
	char mode[MODE_LEN];
	char pwr[PWR_LEN];
	char comment[COMMENT_LEN];
	uint64_t stationId;
	struct timeval tv;
} logEntry_t;


typedef struct {
	uint64_t id;
	char name[NAME_LEN];
	char call[CALL_LEN];
	char QTH[QTH_LEN];
	char QRA[QRA_LEN];
	char ASL[ASL_LEN];
	char RIG[RIG_LEN];
	char ANT[ANT_LEN];
} stationEntry_t;





#endif

