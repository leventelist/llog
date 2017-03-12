/*	This is llog, logger for Amateur Radio operations.
 *
 *	Copyright (C) 2017 Levente Kovacs
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
#include "debug.h"
#include "db_sqlite.h"

#include <inttypes.h>
#include <string.h>


int db_sqlite_init(hm_t *hm_data) {

	int ret;

	ret=sqlite3_open(hm_data->sqlite_fn, &hm_data->sq3_db);

	if (ret!=SQLITE_OK) {
		debug(hm_data->dbg, DEBUG_ERR, "%s: Error opening main database '%s'.\n", __func__, hm_data->sqlite_fn);
		return ret;
	}

	if (hm_data->db_timeout_ms < 1000) {
		hm_data->db_timeout_ms=1000;
	}

	sqlite3_busy_timeout(hm_data->sq3_db, hm_data->db_timeout_ms);

	ret=sqlite3_open(hm_data->sqlite_log_fn, &hm_data->sq3_db_log);

	if (ret==SQLITE_OK) {
		sqlite3_busy_timeout(hm_data->sq3_db_log, hm_data->db_timeout_ms);
	} else {
		debug(hm_data->dbg, DEBUG_ERR, "%s: Error opening log database. '%s'\n", __func__, hm_data->sqlite_log_fn);
	}

	return ret;
}

uint8_t db_sqlite_close(hm_t *data) {

	sqlite3_close(data->sq3_db);
	sqlite3_close(data->sq3_db_log);

	return OK;
}


uint8_t db_select_zones_to_run(hm_t *hm_data, uint32_t timing) {

	sqlite3_stmt *sq3_stmt;

	char buff[BUF_SIZ], table_name[128];
	uint8_t ret_val;
	struct timeval now;
	uint64_t enabled_zones[MAIN_ZONES];
	uint64_t time_to_run[MAIN_ZONES];
	uint64_t stat[MAIN_ZONES];
	int ret, have_work, table_not_existed;
	uint32_t enabled_zones_i, found_zones_n, out_zone_i;
	uint64_t last_access_time, log_id;


	/*get current system time*/
	gettimeofday(&now, NULL);

	/*Get zone info       0   1            2*/
	sprintf(buff, "SELECT id, time_to_run, stat FROM zone WHERE stat=%lu OR stat=%lu OR stat=%lu OR stat=%lu;", HM_ZONE_S_ENABLED, HM_ZONE_S_RUNNING, HM_ZONE_S_STOPPED, HM_ZONE_S_DISABLE);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	enabled_zones_i=0;
	hm_data->zone_n=0;

	have_work=1;
	while (have_work==1) {
		ret=sqlite3_step(sq3_stmt);
		switch (ret) {
			case SQLITE_ROW:
			if (enabled_zones_i<MAIN_ZONES) {
				enabled_zones[enabled_zones_i]=sqlite3_column_int64(sq3_stmt, 0);
				time_to_run[enabled_zones_i]=sqlite3_column_int64(sq3_stmt, 1);
				stat[enabled_zones_i]=sqlite3_column_int64(sq3_stmt, 2);
				enabled_zones_i++;
				have_work=1;
			}
			break;
			case SQLITE_DONE:
			have_work=0;
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			have_work=0;
			ret_val=ERR;
			break;
			default:
			have_work=0;
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (%d): %s\n", __func__, ret, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
	}

	sqlite3_finalize(sq3_stmt);
	found_zones_n=enabled_zones_i;
	out_zone_i=0;


/*select the log database for devices that needs to be updated.*/
	for (enabled_zones_i=0; enabled_zones_i<found_zones_n; enabled_zones_i++) {
		if (timing == DB_NO_TIMING) { /*simply copy all found zones to the output set. Don't care about the timing*/
			hm_data->zone_id[out_zone_i]=enabled_zones[enabled_zones_i];
			out_zone_i++;
			continue;
		}
		if (stat[enabled_zones_i]==HM_ZONE_S_DISABLE || stat[enabled_zones_i]==HM_ZONE_S_STOPPED) {
			hm_data->zone_id[out_zone_i]=enabled_zones[enabled_zones_i];
			out_zone_i++;
			continue;
		}
		sprintf(table_name, "zone_%" PRIu64, enabled_zones[enabled_zones_i]);
		/*see if there is a table in the log database for the zone. If not, create, and add to the output.*/
		sprintf(buff, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table_name);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);

		table_not_existed=1;
		have_work=0;

		switch (ret) {
			case SQLITE_ROW: /*there is the table in the database*/
			table_not_existed=0;
			ret_val=OK;
			break;
			case SQLITE_DONE: /*no table in the database*/
			table_not_existed=1;
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
			default:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: (%d) %s\n", __func__, ret, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
		sqlite3_finalize(sq3_stmt);


		if (table_not_existed) {
		/*CREATE the table*/
			debug(hm_data->dbg, DEBUG_ERR, "%s(): creating log table '%s'\n", __func__, table_name);
			sprintf(buff, "CREATE TABLE '%s' ("ZONE_LOG_FIELDS");", table_name);
			sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
			sqlite3_step(sq3_stmt);
			/*no error checking. If the table can't be created, the read will fail.*/
			hm_data->zone_id[out_zone_i]=enabled_zones[enabled_zones_i];
			out_zone_i++;
			sqlite3_finalize(sq3_stmt);
			/*INSERT some fake data. This is needed to triger a zone run, when there is no real data in the database*/
			sprintf(buff, "INSERT INTO '%s' (access_time) VALUES (0);", table_name);
			sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
			sqlite3_step(sq3_stmt);
			sqlite3_finalize(sq3_stmt);
		} else { /*table existed*/
			sprintf(buff, "SELECT id, access_time FROM '%s' ORDER BY id DESC LIMIT 1;", table_name);
			sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
			ret=sqlite3_step(sq3_stmt);
			switch (ret) {
				case SQLITE_ROW:
				log_id=sqlite3_column_int64(sq3_stmt, 0);
				last_access_time=sqlite3_column_int64(sq3_stmt, 1);
				if (time_to_run[enabled_zones_i] + last_access_time <= (uint64_t)now.tv_sec) { /*yes, this zone must be updated*/
					hm_data->zone_id[out_zone_i]=enabled_zones[enabled_zones_i];
					out_zone_i++;
					have_work=1;
				}
				break;
				case SQLITE_DONE: /*no*/
				break;
				case SQLITE_BUSY:
				debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
				ret_val=ERR;
				break;
				default:
				debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: (%d) %s\n", __func__, ret, sqlite3_errmsg(hm_data->sq3_db));
				ret_val=ERR;
				break;
			}
			sqlite3_finalize(sq3_stmt);
			if (have_work==1) {
				sprintf(buff, "UPDATE '%s' SET access_time=%" PRIu64 " WHERE id=%" PRIu64 ";", table_name, (uint64_t)now.tv_sec, log_id);
				sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
				sqlite3_step(sq3_stmt);
				sqlite3_finalize(sq3_stmt);
			}
		}
	}
	hm_data->zone_n=out_zone_i;

	return ret_val;
}

uint8_t db_update_zone(hm_t *hm_data, zone_t *z_data) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret;
	uint8_t ret_val;


	sprintf(buff, "UPDATE zone SET stat='%" PRIu64 "' WHERE id = '%" PRIu64 "';", z_data->stat, z_data->id);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);


	switch (ret) {
			case SQLITE_ROW: /*Update should not return a row, but we don't care*/
			ret_val=OK;
			break;
			case SQLITE_DONE: /*This is good*/
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
			default:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
	sqlite3_finalize(sq3_stmt);

	return ret_val;
}

uint8_t db_query_zone(hm_t *hm_data, zone_t *z_data) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret;
	uint8_t ret_val;


						//   0    1      2        3   4      5
	sprintf(buff, "SELECT name,stat,target_val,param,logic,log_entries FROM zone WHERE id = '%" PRIu64 "';", z_data->id);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	switch (ret) {
		case SQLITE_ROW:
		/*name*/
		strncpy(z_data->name, (char *) sqlite3_column_text(sq3_stmt, 0), MAIN_ZONE_NAME_LEN);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found zone name '%s' for zone '%" PRIu64 "'.\n", __func__, z_data->name, z_data->id);
		/*stat*/
		z_data->stat=sqlite3_column_int64(sq3_stmt, 1);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found zone stat '%" PRIu64 "' for zone '%" PRIu64 "'.\n", __func__, z_data->stat, z_data->id);
		/*target_val*/
		z_data->target_val=sqlite3_column_double(sq3_stmt, 2);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found zone target_val '%f' for zone '%" PRIu64 "'.\n", __func__, z_data->target_val, z_data->id);
		/*param*/
		z_data->param=sqlite3_column_int64(sq3_stmt, 3);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found zone param '%" PRIu64 "' for zone '%" PRIu64 "'.\n", __func__, z_data->param, z_data->id);
		/*logic*/
		strncpy(z_data->logic, (char *) sqlite3_column_text(sq3_stmt, 4), MAIN_ZONE_LOGIC_LEN);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found zone logic for zone '%" PRIu64 "': \n%s\n", __func__, z_data->id, z_data->logic);
		/*log entries*/
		z_data->log_entries=sqlite3_column_int64(sq3_stmt, 5);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found zone log_entries '%" PRIu64 "' for zone '%" PRIu64 "'.\n", __func__, z_data->log_entries, z_data->id);
		ret_val=OK;
		break;
		case SQLITE_DONE: /*Since we want only one row, this indicates an error (zero row returned)*/
		debug(hm_data->dbg, DEBUG_ERR, "%s(): No such zone '%" PRIu64"'\n", __func__, z_data->id);
		ret_val=ERR;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}
	sqlite3_finalize(sq3_stmt);

	return ret_val;
}

uint8_t db_select_device_to_update(hm_t *hm_data, device_t *d_data) {
	/*Select devices that shall be read or written*/

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ], table_name[128];
	int ret, have_work, table_not_existed;
	uint32_t enabled_device_i, found_devices_n, out_device_i;
	uint8_t ret_val;
	uint64_t enabled_devices[MAIN_IO_DEVICES];
	uint64_t poll_timeout[MAIN_IO_DEVICES];
	uint64_t last_access_time;
	uint64_t log_id;

	struct timeval now;

	gettimeofday(&now, NULL);


/*select the main db for enabled devices*/
	sprintf(buff, "SELECT id,poll_timeout FROM device WHERE stat=%lu AND type=%" PRIu64 ";", HM_DEVICE_S_ENABLED, d_data->type);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	enabled_device_i=0;
	have_work=1;
	while (have_work==1) {
		ret=sqlite3_step(sq3_stmt);
		switch (ret) {
			case SQLITE_ROW:
			enabled_devices[enabled_device_i]=sqlite3_column_int64(sq3_stmt, 0);
			poll_timeout[enabled_device_i]=sqlite3_column_int64(sq3_stmt, 1);
			enabled_device_i++;
			have_work=1;
			break;
			case SQLITE_DONE:
			have_work=0;
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			have_work=0;
			ret_val=ERR;
			break;
			default:
			have_work=0;
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
	}
	sqlite3_finalize(sq3_stmt);

	found_devices_n=enabled_device_i;
	out_device_i=0;


/*select the log database for devices that needs to be updated.*/
	for (enabled_device_i=0; enabled_device_i<found_devices_n; enabled_device_i++) {
		sprintf(table_name, "device_%" PRIu64, enabled_devices[enabled_device_i]);
		/*see if there is a table in the log database for the device. If not, create, and add to the output. This is essential for input devices*/
		sprintf(buff, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table_name);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);

		table_not_existed=1;

		switch (ret) {
			case SQLITE_ROW: /*there is the table in the database*/
			table_not_existed=0;
			ret_val=OK;
			break;
			case SQLITE_DONE: /*no table in the database*/
			table_not_existed=1;
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
			default:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
		sqlite3_finalize(sq3_stmt);

		if (table_not_existed) {
		/*CREATE the table*/
			debug(hm_data->dbg, DEBUG_ERR, "%s(): creating log table '%s'\n", __func__, table_name);
			sprintf(buff, "CREATE TABLE '%s' ("DEV_LOG_FIELDS");", table_name);
			sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
			sqlite3_step(sq3_stmt);
			/*no error checking. If the table can't be created, the read will fail.*/
			hm_data->device_id[out_device_i]=enabled_devices[enabled_device_i];
			out_device_i++;
			sqlite3_finalize(sq3_stmt);
			sprintf(buff, "INSERT INTO '%s' (sample_time) VALUES (0);", table_name);
			sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
			sqlite3_step(sq3_stmt);
			sqlite3_finalize(sq3_stmt);
		} else { /*table existed*/
			/*get the access_time of the last log entry*/
			have_work=0;
			sprintf(buff, "SELECT id, access_time FROM '%s' ORDER BY id DESC LIMIT 1;", table_name);
			sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
			ret=sqlite3_step(sq3_stmt);
			switch (ret) {
				case SQLITE_ROW:
				log_id=sqlite3_column_int64(sq3_stmt, 0);
				last_access_time=sqlite3_column_int64(sq3_stmt, 1);
				if (poll_timeout[enabled_device_i] + last_access_time <= (uint64_t)now.tv_sec) { /*yes, this device must be updated*/
					hm_data->device_id[out_device_i]=enabled_devices[enabled_device_i];
					out_device_i++;
					have_work=1;
				}
				break;
				case SQLITE_DONE: /*no*/
				break;
				case SQLITE_BUSY:
				debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
				ret_val=ERR;
				break;
				default:
				debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
				ret_val=ERR;
				break;
			}
			sqlite3_finalize(sq3_stmt);
			if (have_work==1 && d_data->type==HM_DEVICE_DT_OUTPUT) {
				sprintf(buff, "UPDATE '%s' SET access_time=%" PRIu64 " WHERE id=%" PRIu64 ";", table_name, (uint64_t)now.tv_sec, log_id);
				sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
				sqlite3_step(sq3_stmt);
				sqlite3_finalize(sq3_stmt);
			}
		}
	}
	hm_data->device_n=out_device_i;

	return ret_val;
}

uint8_t db_query_device(hm_t *hm_data, device_t *d_data) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret;
	uint8_t ret_val;

//                        0    1    2    3    4   5   6    7     8            9            10
	sprintf(buff, "SELECT name,stat,type,port,vci,fid,chid,param,poll_timeout,install_time,log_entries FROM device WHERE id = '%" PRIu64 "';", d_data->id);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	switch (ret) {
		case SQLITE_ROW:
		/*name*/
		strncpy(d_data->name, (char *) sqlite3_column_text(sq3_stmt, 0), MAIN_DEV_NAME_LEN);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device name '%s' for device '%" PRIu64 "'.\n", __func__, d_data->name, d_data->id);
		/*stat*/
		d_data->stat=sqlite3_column_int64(sq3_stmt, 1);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device stat '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->stat, d_data->id);
		/*type*/
		d_data->type=sqlite3_column_int64(sq3_stmt, 2);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device type '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->type, d_data->id);
		/*port*/
		d_data->port=sqlite3_column_int64(sq3_stmt, 3);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device port '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->port, d_data->id);
		/*vci*/
		d_data->vci=sqlite3_column_int64(sq3_stmt, 4);
		if (d_data->vci==0) { /*it means, that the tranformation of the number was unsuccessful, and the field is represented as text*/
			d_data->vci=strtoull((char *) sqlite3_column_text(sq3_stmt, 4), NULL, 0);
		}
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device vci '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->vci, d_data->id);
		/*fid*/
		d_data->fid=sqlite3_column_int64(sq3_stmt, 5);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device fid '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->fid, d_data->id);
		/*chid*/
		d_data->chid=sqlite3_column_int64(sq3_stmt, 6);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device chid '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->chid, d_data->id);
		/*param*/
		d_data->param=sqlite3_column_int64(sq3_stmt, 7);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device param '%" PRIu64 "' for device '%" PRIu64 "'.\n", __func__, d_data->param, d_data->id);
		/*poll_timeout*/
		d_data->poll_timeout=sqlite3_column_int64(sq3_stmt, 8);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device poll_timeout '%" PRIu64 "' for dev_id '%" PRIu64 "'.\n", __func__, d_data->poll_timeout, d_data->id);
		/*install_time*/
		d_data->install_time=sqlite3_column_int64(sq3_stmt, 9);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device install_time '%" PRIu64 "' for dev_id '%" PRIu64 "'.\n", __func__, d_data->install_time, d_data->id);
		/*log entries*/
		d_data->log_entries=sqlite3_column_int64(sq3_stmt, 10);
		debug(hm_data->dbg, DEBUG_ALL, "%s(): found device log_entries '%" PRIu64 "' for dev_id '%" PRIu64 "'.\n", __func__, d_data->log_entries, d_data->id);
		ret_val=OK;
		break;
		case SQLITE_DONE: /*Since we want only one row, this indicates an error (zero row returned)*/
		debug(hm_data->dbg, DEBUG_ERR, "%s(): No such device '%" PRIu64"'\n", __func__, d_data->id);
		ret_val=ERR;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}
	sqlite3_finalize(sq3_stmt);

	sprintf(buff, "SELECT type FROM port WHERE id = '%" PRIu64 "';", d_data->port);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	switch (ret) {
		case SQLITE_ROW:
		d_data->port_type=sqlite3_column_int64(sq3_stmt, 0);
		ret_val=OK;
		break;
		case SQLITE_DONE: /*Since we want only one row, this indicates an error (zero row returned)*/
		debug(hm_data->dbg, DEBUG_ERR, "%s(): No port for the device '%" PRIu64"'\n", __func__, d_data->id);
		ret_val=ERR;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}
	sqlite3_finalize(sq3_stmt);

	return ret_val;
}

uint8_t db_select_device(hm_t *hm_data, device_t *d_data) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret;
	uint8_t ret_val;

	/*we do a select to get id, which is further used.*/
	sprintf(buff, "SELECT id FROM device WHERE vci='%" PRIu64 "' AND fid='%" PRIu64 "' AND chid='%" PRIu64 "' AND param='%" PRIu64 "';", d_data->vci, d_data->fid, d_data->chid, d_data->param);
	sqlite3_prepare_v2(hm_data->sq3_db, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	switch (ret) {
		case SQLITE_ROW:
		d_data->id=sqlite3_column_int64(sq3_stmt, 0);
		ret_val=OK;
		break;
		case SQLITE_DONE: /*Since we want only one row, this indicates an error (zero row returned)*/
		debug(hm_data->dbg, DEBUG_ERR, "%s(): Device not found.\n", __func__);
		ret_val=ERR;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}
	sqlite3_finalize(sq3_stmt);
	
	return ret_val;
}

uint8_t db_device_log(hm_t *hm_data, device_t *d_data) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ], table_name[128]; /*string buffers*/
	int ret;
	uint8_t ret_val, table_empty, no_table, new_data;
	uint64_t log_id;
	uint64_t last_access_time, last_sample_time;
	double last_value;

	log_id=0;

	sprintf(table_name, "device_%" PRIu64, d_data->id);

	/*Check if the log table exists*/
	/*How to make this portable?*/
	sprintf(buff, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table_name);
	sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	no_table=1;

	switch (ret) {
		case SQLITE_ROW: /*there is the table in the database*/
		no_table=0;
		ret_val=OK;
		break;
		case SQLITE_DONE: /*no table in the database*/
		no_table=1;
		ret_val=OK;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}

	sqlite3_finalize(sq3_stmt);

	if (ret_val!=OK) {
		return ret_val;
	}

	if (no_table) {
	/*CREATE the table*/
		debug(hm_data->dbg, DEBUG_ERR, "%s(): creating log table '%s'\n", __func__, table_name);
		sprintf(buff, "CREATE TABLE '%s' ("DEV_LOG_FIELDS");", table_name);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);
		/*no error checking. If the table can't be created, the read will fail.*/
	}

	sprintf(buff, "SELECT id, access_time, sample_time, value FROM %s ORDER BY id DESC LIMIT 1;", table_name);
	sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	table_empty=1;
	new_data=1;

	switch (ret) {
		case SQLITE_ROW: /*there is some logged data in the database*/
		log_id=sqlite3_column_int64(sq3_stmt, 0);
		last_access_time=sqlite3_column_int64(sq3_stmt, 1);
		last_sample_time=sqlite3_column_int64(sq3_stmt, 2);
		last_value=sqlite3_column_double(sq3_stmt, 3);
		if (last_value==d_data->value) {
			new_data=0;
		}
		table_empty=0;
		ret_val=OK;
		break;
		case SQLITE_DONE: /*the log table is empty*/
		table_empty=1;
		ret_val=OK;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}
	sqlite3_finalize(sq3_stmt);

	if (ret_val!=OK) {
		return ret_val;
	}

	if (table_empty==0 && new_data==1 && last_access_time!=last_sample_time) {
	/*This indicates that the data we are going to insert is changed. We have to INSERT a row with the old value, but with the recent time,
	 * so plots is going to look nice*/
		/*INSERT*/
		sprintf(buff, "INSERT INTO %s (value, param, sample_time, access_time) VALUES (%f, %" PRIu64 ", %" PRIu64 ", %" PRIu64 ");", table_name, last_value, d_data->param, last_access_time, last_access_time);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);

		switch (ret) {
			case SQLITE_ROW: /*This is an insert. It should not return data*/
			ret_val=ERR;
			break;
			case SQLITE_DONE: /*Insert successful*/
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
			default:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
		sqlite3_finalize(sq3_stmt);
	}

	if (table_empty==0 && new_data==0) {
		/*The data doesn't changed. We just update the access_time of the existing row*/
		sprintf(buff, "UPDATE %s SET access_time=%" PRIi64 " WHERE id=%" PRIi64 ";", table_name, d_data->sample_time, log_id);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);

		switch (ret) {
			case SQLITE_ROW: /*This is an insert. It should not return data*/
			ret_val=ERR;
			break;
			case SQLITE_DONE: /*Insert successful*/
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
			default:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
		sqlite3_finalize(sq3_stmt);
	}

	if (table_empty==1 || new_data==1) { /*when to INSERT new value?*/
		/*INSERT*/
		sprintf(buff, "INSERT INTO %s (value, param, sample_time, access_time) VALUES (%f, %" PRIu64 ", %" PRIu64 ", %" PRIu64 ");", table_name, d_data->value, d_data->param, d_data->sample_time, d_data->sample_time);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);

		switch (ret) {
			case SQLITE_ROW: /*This is an insert. It should not return data*/
			ret_val=ERR;
			break;
			case SQLITE_DONE: /*Insert successful*/
			ret_val=OK;
			break;
			case SQLITE_BUSY:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
			default:
			debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
			ret_val=ERR;
			break;
		}
		sqlite3_finalize(sq3_stmt);
	}

	/*DELETE if we reach log_entries*/
	if(table_empty==0 && log_id > d_data->log_entries) {
		sprintf(buff, "DELETE FROM '%s' WHERE id<%" PRIu64 ";", table_name, log_id-d_data->log_entries);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		sqlite3_step(sq3_stmt);
		sqlite3_finalize(sq3_stmt);
		/*no error checking.*/
	}

	return ret_val;
}

uint8_t db_zone_log(hm_t *hm_data, zone_t *z_data) {

	sqlite3_stmt *sq3_stmt;
	char buff[BUF_SIZ];
	int ret;
	uint8_t ret_val, isEmpty;
	uint64_t log_id;

	/*Don't do anything if no logging is required.*/
	if (z_data->log_entries==0) {
		debug(hm_data->dbg, DEBUG_INFO, "%s(): no logging enabled for zone '%" PRIu64 "'\n", __func__, z_data->id);
		return OK;
	}

	/*Check if the log table exists*/
	/*How to make this portable?*/

	sprintf(buff, "SELECT name FROM sqlite_master WHERE type='table' AND name='zone_%" PRIu64 "';", z_data->id);
	sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	isEmpty=1;

	switch (ret) {
		case SQLITE_ROW: /*there is the table in the database*/
		isEmpty=0;
		ret_val=OK;
		break;
		case SQLITE_DONE: /*no table in the database*/
		isEmpty=1;
		ret_val=OK;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}

	sqlite3_finalize(sq3_stmt);

	if (ret_val!=OK) {
		return ret_val;
	}

	if (isEmpty) {
	/*CREATE the table*/
		debug(hm_data->dbg, DEBUG_ERR, "%s(): creating log table 'zone_%" PRIu64 "'\n", __func__, z_data->id);
		sprintf(buff, "CREATE TABLE 'zone_%" PRIu64 "' ("ZONE_LOG_FIELDS");", z_data->id);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		ret=sqlite3_step(sq3_stmt);
		/*no error checking. If the table can't be created, the read will fail.*/
	}

	/*get the latest log_id and the last logged time in the table.*/
	sprintf(buff, "SELECT id FROM zone_%" PRIu64 " ORDER BY id DESC LIMIT 1;", z_data->id);
	sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	isEmpty=1;

	switch (ret) {
		case SQLITE_ROW: /*there is some logged data in the database*/
		log_id=sqlite3_column_int64(sq3_stmt, 0);
		isEmpty=0;
		ret_val=OK;
		break;
		case SQLITE_DONE: /*the log table is empty*/
		isEmpty=1;
		ret_val=OK;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}

	sqlite3_finalize(sq3_stmt);

	if (ret_val!=OK) {
		return ret_val;
	}

	/*INSERT*/
	sprintf(buff, "INSERT INTO zone_%" PRIu64 " (value, param, sample_time, access_time) VALUES (%f, %" PRIu64 ", %" PRIu64 ", %" PRIu64 ");", z_data->id, z_data->value, z_data->param, z_data->sample_time, z_data->sample_time);
	sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
	ret=sqlite3_step(sq3_stmt);

	switch (ret) {
		case SQLITE_ROW: /*This is an insert. It should not return data*/
		ret_val=ERR;
		break;
		case SQLITE_DONE: /*Insert successful*/
		ret_val=OK;
		break;
		case SQLITE_BUSY:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite (BUSY): %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
		default:
		debug(hm_data->dbg, DEBUG_ERR, "%s(): sqlite: %s\n", __func__, sqlite3_errmsg(hm_data->sq3_db));
		ret_val=ERR;
		break;
	}
	sqlite3_finalize(sq3_stmt);

	/*DELETE*/
	if (isEmpty==0 && log_id > z_data->log_entries) {
		sprintf(buff, "DELETE FROM 'zone_%" PRIu64 "' WHERE id<%" PRIu64 ";", z_data->id, log_id-z_data->log_entries);
		sqlite3_prepare_v2(hm_data->sq3_db_log, buff, -1, &sq3_stmt, NULL);
		sqlite3_step(sq3_stmt);
		sqlite3_finalize(sq3_stmt);
		/*no error checking.*/
	}

	return ret_val;
}
