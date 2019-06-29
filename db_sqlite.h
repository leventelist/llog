/*	This is home_manager, which is a home controller daemon.
 *
 *	Copyright (C) 2014-2015 Levente Kovacs
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

#define DEV_LOG_FIELDS "id INTEGER PRIMARY KEY AUTOINCREMENT, value REAL, param INTEGER, access_time INTEGER, sample_time INTEGER"
#define ZONE_LOG_FIELDS "id INTEGER PRIMARY KEY AUTOINCREMENT, value REAL, param INTEGER, access_time INTEGER, sample_time INTEGER"


/*zone select mode*/
#define DB_NO_TIMING 0
#define DB_USE_TIMING 1


/*return values*/

#define DB_OK 0
#define DB_ERROR 1

/*functions*/
int db_sqlite_init(llog_t *llog);
int db_sqlite_close(llog_t *llog);
//uint8_t db_select_zones_to_run(hm_t *data, uint32_t timing);
//uint8_t db_query_zone(hm_t *data, zone_t *z_data);
//uint8_t db_update_zone(hm_t *hm_data, zone_t *z_data);
//uint8_t db_select_device_to_update(hm_t *hm_data, device_t *d_data);
//uint8_t db_query_device(hm_t *data, device_t *d_data);
//uint8_t db_select_device(hm_t *data, device_t *d_data);
//uint8_t db_device_log(hm_t *data, device_t *d_data);
//uint8_t db_zone_log(hm_t *data, zone_t *z_data);

#endif
