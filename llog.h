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

#ifndef LLOG_H
#define LLOG_H

#define VERSION "v1.3.1"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <sqlite3.h>

#define DATABASE_TIMEOUT 1000

#define LOG_ENTRY_LEN 2048

#define LOGFILE_LEN 100
#define QTH_LEN 100
#define QRA_LEN 20
#define QRG_LEN 20
#define MODE_LEN 20
#define RIG_LEN 50
#define ANT_LEN 20
#define RST_LEN 20
#define CALL_LEN 40
#define NAME_LEN 100
#define COMMENT_LEN 2048
#define QSL_LEN 10
#define ASL_LEN 20
#define PWR_LEN 20
#define LIST_LEN 50
#define LINE_LEN 1024
#define SUBSTR_LEN 512
#define X_LEN 40
#define STATION_LEN 256

#define MONDE_N 256

#define OK 0
#define FILE_ERR 1
#define CMD_LINE_ERR 2

#define LLOG_EOF 1
#define LLOG_CANCEL 2
#define LLOG_ERR 3

#define LLOG_MODE_L 1
#define LLOG_MODE_N 2



#define PROMPT "\n: "


typedef struct {
	char logfileFn[LOGFILE_LEN]; /*SQLite database file name*/
	char station[STATION_LEN];
	sqlite3 *db;
	uint32_t stat;
} llog_t;


typedef struct {
	char *name;
	char *default_rst;
} op_mode_t;


typedef struct {
	uint64_t id;
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
	op_mode_t mode;
	char pwr[PWR_LEN];
	char comment[COMMENT_LEN];
	uint64_t stationId;
	char date[NAME_LEN];
	char utc[NAME_LEN];
	uint32_t data_stat;
} log_entry_t;


typedef struct {
	uint64_t id;
	char name[NAME_LEN];
	char call[CALL_LEN];
	char QTH[QTH_LEN];
	char QRA[QRA_LEN];
	char ASL[ASL_LEN];
	char RIG[RIG_LEN];
	char ANT[ANT_LEN];
} station_entry_t;

/*Function definitions*/
int llog_init(char *logfile_name);
int llog_open_db(void);
void llog_shutdown(void);
int llog_add_log_entry(void);

#endif

