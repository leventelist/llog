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
 *
 */

#ifndef DB_SQLITE_H
#define DB_SQLITE_H

#include "llog.h"
#include <stdint.h>


/*data status*/

enum db_data_state {
    db_data_init,
    db_data_valid,
    db_data_last,
    db_data_err
};

enum db_state {
	db_opened,
	db_closed
};


/*functions*/
int db_sqlite_init(llog_t *llog);
int db_close(llog_t *llog);
int db_set_log_entry(llog_t *log, log_entry_t *entry);
int db_check_dup_qso(llog_t *log, log_entry_t *entry);
int db_get_max_nr(llog_t *log, log_entry_t *entry);
int db_get_station_entry(llog_t *log, station_entry_t *station);
int db_get_mode_entry(llog_t *log, mode_entry_t *mode, uint64_t *id);
int db_get_log_entries(llog_t *log, log_entry_t *entry);

#endif
