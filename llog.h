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

#ifndef LLOG_H
#define LLOG_H

#define VERSION "v1.5.1"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <sqlite3.h>
#include "conf.h"

#define DATABASE_TIMEOUT 1000

#define FILE_LEN 1024
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
#define ASL_LEN 20
#define PWR_LEN 20
#define X_LEN 40
#define STATION_LEN 256

#define OK 0
#define FILE_ERR 1
#define CMD_LINE_ERR 2
#define NO_DATA 3
#define ERR 0xffff

#define LLOG_ERR 3
#define LLOG_DUP 4

enum llog_entry_pos {
  llog_entry_call = 0,
  llog_entry_date,
  llog_entry_utc,
  llog_entry_rxrst,
  llog_entry_txrst,
  llog_entry_qth,
  llog_entry_name,
  llog_entry_qra,
  llog_entry_qrg,
  llog_entry_mode,
  llog_entry_power,
  llog_entry_rxnr,
  llog_entry_txnr,
  llog_entry_rxextra,
  llog_entry_txextra,
  llog_entry_comment,
  llog_entry_summit_ref,
  llog_entry_station_id,
};

/*Main data storage*/
typedef struct {
  char log_file_name[FILE_LEN];       /*SQLite database file name*/
  char config_file_name[FILE_LEN];
//	char station[STATION_LEN];
  uint64_t station_id;
  sqlite3 *db;
  uint32_t stat;
  config_attribute_t *ca;
  char gpsd_host[FILE_LEN];
  uint64_t gpsd_port;
} llog_t;


/*Mode data storage.*/
typedef struct {
  uint64_t id;
  char name[MODE_LEN];
  char default_rst[MODE_LEN];
  char comment[COMMENT_LEN];
  uint32_t data_stat;
  sqlite3_stmt *sq3_stmt;
} mode_entry_t;


typedef struct {
  uint64_t id;
  char call[CALL_LEN];
  char rxrst[RST_LEN];
  char txrst[RST_LEN];
  uint64_t txnr;
  uint64_t rxnr;
  char rxextra[X_LEN];
  char txextra[X_LEN];
  char qth[QTH_LEN];
  char name[NAME_LEN];
  char qra[QRA_LEN];
  double qrg;
  mode_entry_t mode;
  char power[PWR_LEN];
  char comment[COMMENT_LEN];
  uint64_t station_id;
  char date[NAME_LEN];
  char utc[NAME_LEN];
  uint32_t data_stat;
  sqlite3_stmt *sq3_stmt;
} log_entry_t;


/*station data*/
typedef struct {
  uint64_t id;
  char name[STATION_LEN];
  char call[CALL_LEN];
  char QTH[QTH_LEN];
  char QRA[QRA_LEN];
  char ASL[ASL_LEN];
  char rig[RIG_LEN];
  char ant[ANT_LEN];
  uint32_t data_stat;
  sqlite3_stmt *sq3_stmt;
} station_entry_t;


/*Function definitions*/
llog_t *llog_init(void);
int llog_set_log_file(char *log_file_name);
int llog_set_config_file(char *config_file_name);
int llog_get_log_file_path(char **path);
int llog_open_db(void);
void llog_shutdown(void);
int llog_add_log_entries(void);
int llog_add_station_entries(void);
int llog_get_initial_station(station_entry_t **station);
int llog_save_config_file(void);
int llog_parse_config_file(void);
int llog_add_modes_entries(void);
int llog_log_entry(log_entry_t *entry);
void llog_reset_entry(log_entry_t *entry);
void llog_get_time(log_entry_t *entry);
void llog_print_log_data(log_entry_t *entry);
void llog_strupper(char *s);
int llog_check_dup_qso(log_entry_t *entry);
int llog_get_default_rst(char *default_rst, uint64_t mode_id);
int llog_get_max_nr(log_entry_t *entry);
int llog_load_static_data(log_entry_t *entry);

#endif
