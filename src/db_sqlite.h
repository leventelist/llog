/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2024  Levente Kovacs
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


/*functions*/
int db_sqlite_init(llog_t *llog);
int db_close(llog_t *llog);
int db_set_log_entry(llog_t *log, log_entry_t *entry);
int db_check_dup_qso(llog_t *log, log_entry_t *entry);
int db_get_max_nr(llog_t *log, log_entry_t *entry);
int db_get_station_entry(llog_t *log, station_entry_t *station);
int db_get_mode_entry(llog_t *log, mode_entry_t *mode, uint64_t *id);
int db_get_log_entries(llog_t *log, log_entry_t *entry);
int db_get_log_entry_with_station(llog_t *llog, log_entry_t *entry, station_entry_t *station);
int db_create_from_schema(llog_t *llog, const char *schema_file);
int db_get_summit_entry(llog_t *llog, summit_entry_t *summit, position_t *pos);

#endif
