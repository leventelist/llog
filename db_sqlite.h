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
 *
 * http://levente.logonex.eu
 * leva@logonex.eu
 *
 */

#ifndef DB_SQLITE_H
#define DB_SQLITE_H

#include "llog.h"
#include <stdint.h>

#define BUF_SIZ 2048


/*return values*/
#define DB_OK 0
#define DB_ERROR 1

/*functions*/
int db_sqlite_init(llog_t *llog);
int db_sqlite_close(llog_t *llog);
int lookupStation(llog_t *llog, stationEntry_t *station);
int setLogEntry(llog_t *log, logEntry_t *entry);
int checkDupQSO(llog_t *log, logEntry_t *entry);
int getMaxNr(llog_t *log, logEntry_t *entry);

#endif
