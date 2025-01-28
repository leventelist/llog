/*	This is llog, logger for Amateur Radio operations.
 *
 *	Copyright (C) 2017-2024 Levente Kovacs
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

/* To aid portability the only SQL statements is used is INSERT, SELECT,
 * DELETE, UPDATE.
 */

#include <stdint.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <stdbool.h>
#include "db_sqlite.h"
#include "llog.h"
#include "llog_config.h"

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#define BUF_SIZ 8192
#define EMPTY_STRING ""
#define R_EARTH 6371e3
#define SQLITE_FLAGS SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX


int db_sqlite_init(llog_t *llog) {
  int ret;
  int ret_val = llog_stat_ok;

  if (llog->stat == db_opened) {
    db_close(llog);
    llog->stat = db_closed;
  }

  printf("Opening log file %s\n", llog->log_file_name);
  ret = sqlite3_open_v2(llog->log_file_name, &llog->log_db, SQLITE_FLAGS, NULL);

  do{
    if (ret != SQLITE_OK) {
      printf("Error opening the log database '%s'.\n", llog->log_file_name);
      ret_val = llog_stat_err;
      break;
    }
    sqlite3_busy_timeout(llog->log_db, DATABASE_TIMEOUT);
    llog->stat = db_opened;
  } while (0);


  printf("Opening summits database %s\n", SUMMITS_DB_PATH);
  // Enable WAL mode
  ret = sqlite3_exec(llog->log_db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "Failed to enable WAL mode: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  // Open the summits database
  ret = sqlite3_open_v2(SUMMITS_DB_PATH, &llog->summits_db, SQLITE_FLAGS, NULL);
  if (ret != SQLITE_OK) {
    printf("Error opening the summits database '%s'.\n", SUMMITS_DB_PATH);
    ret_val = llog_stat_err;
  } else {
    sqlite3_busy_timeout(llog->summits_db, DATABASE_TIMEOUT);
  }


  return ret_val;
}


int db_close(llog_t *llog) {
  printf("Closing log database\n");
  sqlite3_close_v2(llog->log_db);
  llog->stat = db_closed;
  llog->log_db = NULL;
  printf("Closing summit database\n");
  sqlite3_close_v2(llog->summits_db);

  return llog_stat_ok;
}


int db_check_dup_qso(llog_t *llog, log_entry_t *entry) {
  int ret, ret_val = llog_stat_ok;
  char buff[BUF_SIZ];
  bool have_work = true;


  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  sprintf(buff, "SELECT date, UTC FROM log WHERE call='%s';", entry->call);
  sqlite3_prepare_v2(llog->log_db, buff, -1, &entry->sq3_stmt, NULL);

  while (have_work) {
    ret = sqlite3_step(entry->sq3_stmt);
    switch (ret) {
    case SQLITE_ROW:
      strncpy(entry->date, (char *)sqlite3_column_text(entry->sq3_stmt, 0), NAME_LEN);
      strncpy(entry->utc, (char *)sqlite3_column_text(entry->sq3_stmt, 1), NAME_LEN);
      printf("\nDUP QSO on %s at %sUTC.\n", entry->date, entry->utc);
      ret_val = llog_stat_dup;
      have_work = false;
      break;

    case SQLITE_DONE:
      have_work = false;
      ret_val = llog_stat_ok;
      break;

    case SQLITE_BUSY:
      ret_val = llog_stat_err;
      have_work = false;
      break;

    default:
      ret_val = llog_stat_err;
      have_work = false;
      printf("Error looking up DUP QSOs: %s\n", sqlite3_errmsg(llog->log_db));
      break;
    }
  }

  sqlite3_finalize(entry->sq3_stmt);

  return ret_val;
}


/* Get a single log entry.
 *
 * Call this function repetatively until you get an error,
 * or a db_data_last in the status of the entry pointer, even if you don't want more.
 * */
int db_get_log_entries(llog_t *llog, log_entry_t *entry) {
  int ret, ret_val = llog_stat_ok;
  bool finalize = true;
  char buff[BUF_SIZ];
  char *cell;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (entry->data_stat == db_data_init) {
    snprintf(buff, BUF_SIZ, "SELECT rowid, date, UTC, call, rxrst, txrst, QRG, mode FROM log ORDER BY rowid DESC;");
    sqlite3_prepare_v2(llog->log_db, buff, -1, &entry->sq3_stmt, NULL);
  }

  entry->data_stat = db_data_err;

  ret = sqlite3_step(entry->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    entry->id = sqlite3_column_int64(entry->sq3_stmt, 0);
    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 1);
    if (cell != NULL) {
      strncpy(entry->date, cell, NAME_LEN);
    } else {
      entry->date[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 2);
    if (cell != NULL) {
      strncpy(entry->utc, cell, NAME_LEN);
    } else {
      entry->utc[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 3);
    if (cell != NULL) {
      strncpy(entry->call, cell, CALL_LEN);
    } else {
      entry->call[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 4);
    if (cell != NULL) {
      strncpy(entry->rxrst, cell, RST_LEN);
    } else {
      entry->rxrst[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 5);
    if (cell != NULL) {
      strncpy(entry->txrst, cell, RST_LEN);
    } else {
      entry->txrst[0] = '\0';
    }

    entry->qrg = sqlite3_column_double(entry->sq3_stmt, 6);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 7);
    if (cell != NULL) {
      strncpy(entry->mode.name, cell, MODE_LEN);
    } else {
      entry->mode.name[0] = '\0';
    }

    finalize = false;
    ret_val = llog_stat_ok;
    entry->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    entry->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up log entry: %s\n", sqlite3_errmsg(llog->log_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(entry->sq3_stmt);
  }

  return ret_val;
}


int db_get_log_entry_with_station(llog_t *llog, log_entry_t *entry, station_entry_t *station) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (entry->data_stat == db_data_init) {
    snprintf(buff, BUF_SIZ,
             "SELECT log.rowid, log.date, log.UTC, log.call, log.rxrst, log.txrst, log.QRG, log.mode, log.SOTA_REF, log.S2S_REF, "
             "station.rowid, station.name, station.CALL, station.QTH, station.QRA, station.ASL, station.rig, station.ant "
             "FROM log "
             "JOIN station ON log.station = station.rowid "
             "ORDER BY log.rowid DESC;");

    sqlite3_prepare_v2(llog->log_db, buff, -1, &entry->sq3_stmt, NULL);
  }
  entry->data_stat = db_data_err;

  ret = sqlite3_step(entry->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    entry->id = sqlite3_column_int64(entry->sq3_stmt, 0);
    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 1);
    if (cell != NULL) {
      strncpy(entry->date, cell, NAME_LEN);
    } else {
      entry->date[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 2);
    if (cell != NULL) {
      strncpy(entry->utc, cell, NAME_LEN);
    } else {
      entry->utc[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 3);
    if (cell != NULL) {
      strncpy(entry->call, cell, CALL_LEN);
    } else {
      entry->call[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 4);
    if (cell != NULL) {
      strncpy(entry->rxrst, cell, RST_LEN);
    } else {
      entry->rxrst[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 5);
    if (cell != NULL) {
      strncpy(entry->txrst, cell, RST_LEN);
    } else {
      entry->txrst[0] = '\0';
    }

    entry->qrg = sqlite3_column_double(entry->sq3_stmt, 6);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 7);
    if (cell != NULL) {
      strncpy(entry->mode.name, cell, MODE_LEN);
    } else {
      entry->mode.name[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 8);
    if (cell != NULL) {
      strncpy(entry->summit_ref, cell, MAX_SUMMIT_CODE_LENGTH);
    } else {
      entry->summit_ref[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 9);
    if (cell != NULL) {
      strncpy(entry->s2s_ref, cell, MAX_SUMMIT_CODE_LENGTH);
    } else {
      entry->s2s_ref[0] = '\0';
    }

    station->id = sqlite3_column_int64(entry->sq3_stmt, 10);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 11);
    if (cell != NULL) {
      strncpy(station->name, cell, NAME_LEN);
    } else {
      station->name[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 12);
    if (cell != NULL) {
      strncpy(station->call, cell, CALL_LEN);
    } else {
      station->call[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 13);
    if (cell != NULL) {
      strncpy(station->QTH, cell, QTH_LEN);
    } else {
      station->QTH[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 14);
    if (cell != NULL) {
      strncpy(station->QRA, cell, QRA_LEN);
    } else {
      station->QRA[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 15);
    if (cell != NULL) {
      strncpy(station->ASL, cell, ASL_LEN);
    } else {
      station->ASL[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 16);
    if (cell != NULL) {
      strncpy(station->rig, cell, RIG_LEN);
    } else {
      station->rig[0] = '\0';
    }

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 17);
    if (cell != NULL) {
      strncpy(station->ant, cell, ANT_LEN);
    } else {
      station->ant[0] = '\0';
    }


    finalize = false;
    ret_val = llog_stat_ok;
    entry->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    entry->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up log entry with station: %s\n", sqlite3_errmsg(llog->log_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(entry->sq3_stmt);
  }

  return ret_val;
}



int db_get_max_nr(llog_t *llog, log_entry_t *entry) {
  int ret, ret_val = llog_stat_ok;
  char buff[BUF_SIZ];
  bool have_work = true;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  entry->txnr = 1;

  sprintf(buff, "SELECT txnr FROM log ORDER BY txnr DESC LIMIT 1;");
  sqlite3_prepare_v2(llog->log_db, buff, -1, &entry->sq3_stmt, NULL);

  while (have_work) {
    ret = sqlite3_step(entry->sq3_stmt);
    switch (ret) {
    case SQLITE_ROW:
      entry->txnr = sqlite3_column_int64(entry->sq3_stmt, 0) + 1;
      ret_val = llog_stat_ok;
      break;

    case SQLITE_DONE:
      have_work = false;
      ret_val = llog_stat_ok;
      break;

    case SQLITE_BUSY:
      ret_val = llog_stat_err;
      have_work = false;
      break;

    default:
      ret_val = llog_stat_err;
      have_work = false;
      printf("Error looking up serial number: %s\n", sqlite3_errmsg(llog->log_db));
      break;
    }
  }

  sqlite3_finalize(entry->sq3_stmt);

  return ret_val;
}


int db_set_log_entry(llog_t *llog, log_entry_t *entry) {
  int ret, ret_val = llog_stat_ok;
  char buff[BUF_SIZ];

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  snprintf(buff, BUF_SIZ, "INSERT INTO log (date, UTC, call, rxrst, txrst, rxnr, txnr, rxextra, txextra, QTH, name, "
                          "QRA, QRG, mode, pwr, rxQSL, txQSL, comment, station, S2S_REF, SOTA_REF) VALUES "
                          "('%s', '%s', '%s', '%s', '%s', %" PRIu64 ", %" PRIu64 ", '%s', '%s', '%s', '%s', '%s', "
                          "%f, '%s', '%s', %" PRIu64 ", %" PRIu64 ", '%s', %" PRIu64 ", '%s', '%s');",
                          entry->date, entry->utc, entry->call, entry->rxrst, entry->txrst, entry->rxnr,
                          entry->txnr, entry->rxextra, entry->txextra, entry->qth, entry->name, entry->qra,
                          entry->qrg, entry->mode.name, entry->power, (uint64_t)0U, (uint64_t)0U, entry->comment,
                          entry->station_id, entry->s2s_ref, entry->summit_ref);
  sqlite3_prepare_v2(llog->log_db, buff, -1, &entry->sq3_stmt, NULL);


  ret = sqlite3_step(entry->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    /*This should not happen.*/
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error inserting log entry: %s\n", sqlite3_errmsg(llog->log_db));
    break;
  }


  sqlite3_finalize(entry->sq3_stmt);

  return ret_val;
}


int db_get_station_entry(llog_t *llog, station_entry_t *station) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (station->data_stat == db_data_init) {
    if (station->id == 0) {
      sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant FROM station ORDER BY rowid DESC;");
    } else {
      sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant FROM station WHERE rowid=%" PRIu64 " ORDER BY rowid DESC;", station->id);
    }
    sqlite3_prepare_v2(llog->log_db, buff, -1, &station->sq3_stmt, NULL);
  }

  station->data_stat = db_data_err;

  ret = sqlite3_step(station->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    station->id = sqlite3_column_int64(station->sq3_stmt, 0);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 1);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->name, cell, NAME_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 2);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->call, cell, CALL_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 3);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->QTH, cell, QTH_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 4);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->QRA, cell, QRA_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 5);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->ASL, cell, ASL_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 6);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->rig, cell, RIG_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 7);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->ant, cell, ANT_LEN);
    ret_val = llog_stat_ok;
    finalize = false;
    station->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    station->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up stations: %s\n", sqlite3_errmsg(llog->log_db));
    break;
  }


  if (finalize) {
    sqlite3_finalize(station->sq3_stmt);
  }

  return ret_val;
}


int db_get_mode_entry(llog_t *llog, mode_entry_t *mode, uint64_t *id) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (mode->data_stat == db_data_init) {
    if (id == NULL) {
      sprintf(buff, "SELECT rowid, name, default_rst, comment FROM mode ORDER BY name ASC;");
    } else {
      sprintf(buff, "SELECT rowid, name, default_rst, comment FROM mode WHERE rowid=%" PRIu64 ";", *id);
    }
    sqlite3_prepare_v2(llog->log_db, buff, -1, &mode->sq3_stmt, NULL);
  }

  mode->data_stat = db_data_err;

  ret = sqlite3_step(mode->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    mode->id = sqlite3_column_int64(mode->sq3_stmt, 0);
    cell = (char *)sqlite3_column_text(mode->sq3_stmt, 1);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(mode->name, cell, MODE_LEN);
    cell = (char *)sqlite3_column_text(mode->sq3_stmt, 2);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(mode->default_rst, cell, MODE_LEN);
    cell = (char *)sqlite3_column_text(mode->sq3_stmt, 2);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(mode->comment, cell, NAME_LEN);
    ret_val = llog_stat_ok;
    finalize = false;
    mode->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    mode->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up mode: %s\n", sqlite3_errmsg(llog->log_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(mode->sq3_stmt);
  }

  return ret_val;
}

int db_create_from_schema(llog_t *llog, const char *schema_file) {
  FILE *file;
  char *schema;
  long length;
  int ret_val = llog_stat_ok;
  char *err_msg = NULL;

  file = fopen(schema_file, "rb");
  if (!file) {
    printf("Error opening schema file '%s'.\n", schema_file);
    return llog_stat_file_err;
  }

  fseek(file, 0, SEEK_END);
  length = ftell(file);
  fseek(file, 0, SEEK_SET);

  schema = malloc(length + 1);
  if (!schema) {
    fclose(file);
    printf("Memory allocation error.\n");
    return llog_stat_mem_err;
  }

  fread(schema, 1, length, file);
  fclose(file);
  schema[length] = '\0';

  ret_val = db_sqlite_init(llog);

  if (ret_val != llog_stat_ok) {
    printf("Error opening database.\n");
    return ret_val;
  }

  ret_val = sqlite3_exec(llog->log_db, schema, 0, 0, &err_msg);
  if (ret_val != SQLITE_OK) {
    printf("Error creating database schema: %s\n", err_msg);
    sqlite3_free(err_msg);
    ret_val = llog_stat_err;
  }

  free(schema);
  return ret_val;
}

int db_get_summit_entry(llog_t *llog, summit_entry_t *summit, position_t *pos) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->summits_db == NULL) {
    summit->data_stat = db_data_err;
    return llog_stat_err;
  }

#define SUMMIT_QUERY "SELECT \
     rowid, summit_code, summit_name, points, bonus_points, valid_from, valid_to, latitude, longitude, alt_m,( \
        6371 * acos( \
            cos(radians(%f)) * cos(radians(latitude)) * \
            cos(radians(longitude) - radians(%f)) + \
            sin(radians(%f)) * sin(radians(latitude)) \
        ) \
    ) AS distance \
  FROM summit_data \
  ORDER BY distance ASC \
  LIMIT 1;"

  if (summit->data_stat == db_data_init) {
    if (pos == NULL) {
      snprintf(buff, BUF_SIZ, "SELECT rowid, summit_code, summit_name, points, bonus_points, valid_from, valid_to, latitude, longitude, alt_m FROM summit_data;");
    } else {
      snprintf(buff, BUF_SIZ, SUMMIT_QUERY, pos->lat, pos->lon, pos->lat);
    }
    sqlite3_prepare_v2(llog->summits_db, buff, -1, &summit->sq3_stmt, NULL);
  }

  summit->data_stat = db_data_err;

  ret = sqlite3_step(summit->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    summit->id = sqlite3_column_int64(summit->sq3_stmt, 0); // rowid

    cell = (char *)sqlite3_column_text(summit->sq3_stmt, 1); // summit_code
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(summit->summit_code, cell, MAX_SUMMIT_CODE_LENGTH);

    cell = (char *)sqlite3_column_text(summit->sq3_stmt, 2); // name
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(summit->name, cell, NAME_LEN);

    summit->points = sqlite3_column_int(summit->sq3_stmt, 3); // points

    summit->bonus_points = sqlite3_column_int(summit->sq3_stmt, 4); // bonus_points

    cell = (char *)sqlite3_column_text(summit->sq3_stmt, 5); // valid_from
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(summit->valid_from, cell, DATE_LEN);

    cell = (char *)sqlite3_column_text(summit->sq3_stmt, 6); // valid_to
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(summit->valid_to, cell, DATE_LEN);
    summit->position.lat = sqlite3_column_double(summit->sq3_stmt, 7);
    summit->position.lon = sqlite3_column_double(summit->sq3_stmt, 8);
    summit->position.alt = sqlite3_column_double(summit->sq3_stmt, 9);
    ret_val = llog_stat_ok;
    finalize = false;
    summit->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    summit->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up summit: %s\n", sqlite3_errmsg(llog->summits_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(summit->sq3_stmt);
  }

  return ret_val;
}
