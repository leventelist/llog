/*	This is llog, logger for Amateur Radio operations.
 *
 *	Copyright (C) 2017-2021 Levente Kovacs
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

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#define BUF_SIZ 8192
#define EMPTY_STRING ""


int db_sqlite_init(llog_t *llog) {
  int ret;
  int ret_val = OK;

  if (llog->stat == db_opened) {
      db_close(llog);
      llog->stat = db_closed;
    }

  ret = sqlite3_open(llog->log_file_name, &llog->db);

  do{
      if (ret != SQLITE_OK) {
          printf("Error opening the log database '%s'.\n", llog->log_file_name);
          ret_val = FILE_ERR;
          break;
        }
      sqlite3_busy_timeout(llog->db, DATABASE_TIMEOUT);
      llog->stat = db_opened;
    } while (0);

  return ret_val;
}


int db_close(llog_t *llog) {
  sqlite3_close(llog->db);

  return OK;
}


int db_check_dup_qso(llog_t *log, log_entry_t *entry) {
  int ret, ret_val = OK;
  sqlite3_stmt *sq3_stmt;
  char buff[BUF_SIZ];
  bool have_work = true;


  sprintf(buff, "SELECT date, UTC FROM log WHERE call='%s';", entry->call);
  sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

  while (have_work) {
      ret = sqlite3_step(sq3_stmt);
      switch (ret) {
        case SQLITE_ROW:
          strncpy(entry->date, (char *)sqlite3_column_text(sq3_stmt, 0), NAME_LEN);
          strncpy(entry->utc, (char *)sqlite3_column_text(sq3_stmt, 1), NAME_LEN);
          printf("\nDUP QSO on %s at %sUTC.\n", entry->date, entry->utc);
          ret_val = LLOG_DUP;
          have_work = false;
          break;

        case SQLITE_DONE:
          have_work = false;
          ret_val = OK;
          break;

        case SQLITE_BUSY:
          ret_val = LLOG_ERR;
          have_work = false;
          break;

        default:
          ret_val = LLOG_ERR;
          have_work = false;
          printf("Error looking up DUP QSOs: %s\n", sqlite3_errmsg(log->db));
          break;
        }
    }

  sqlite3_finalize(sq3_stmt);

  return ret_val;
}


/* Get a single log entry.
 *
 * Call this function repetatively until you get an error,
 * or a db_data_last in the status of the entry pointer, even if you don't want more.
 * */
int db_get_log_entries(llog_t *log, log_entry_t *entry) {
  int ret, ret_val = OK;
  static sqlite3_stmt *sq3_stmt;
  bool finalize = true;
  char buff[BUF_SIZ];

  if (entry->data_stat == db_data_init) {
      snprintf(buff, BUF_SIZ, "SELECT rowid, date, UTC, call, rxrst, txrst, QRG, mode FROM log;");
      sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);
    }

  entry->data_stat = db_data_err;

  ret = sqlite3_step(sq3_stmt);
  switch (ret) {
    case SQLITE_ROW:
      entry->id = sqlite3_column_int64(sq3_stmt, 0);
      strncpy(entry->date, (char *)sqlite3_column_text(sq3_stmt, 1), NAME_LEN);
      strncpy(entry->utc, (char *)sqlite3_column_text(sq3_stmt, 2), NAME_LEN);
      strncpy(entry->call, (char *)sqlite3_column_text(sq3_stmt, 3), CALL_LEN);
      strncpy(entry->rxrst, (char *)sqlite3_column_text(sq3_stmt, 4), RST_LEN);
      strncpy(entry->txrst, (char *)sqlite3_column_text(sq3_stmt, 5), RST_LEN);
      entry->qrg = sqlite3_column_double(sq3_stmt, 6);
      strncpy(entry->mode.name, (char *)sqlite3_column_text(sq3_stmt, 7), MODE_LEN);
      finalize = false;
      ret_val = OK;
      entry->data_stat = db_data_valid;
      break;

    case SQLITE_DONE:
      ret_val = OK;
      entry->data_stat = db_data_last;
      break;

    case SQLITE_BUSY:
      ret_val = LLOG_ERR;
      break;

    default:
      ret_val = LLOG_ERR;
      printf("Error looking up log entry: %s\n", sqlite3_errmsg(log->db));
      break;
    }

  if (finalize) {
      sqlite3_finalize(sq3_stmt);
    }


  return ret_val;
}


int db_get_max_nr(llog_t *log, log_entry_t *entry) {
  int ret, ret_val = OK;
  sqlite3_stmt *sq3_stmt;
  char buff[BUF_SIZ];
  bool have_work = true;

  entry->txnr = 1;

  sprintf(buff, "SELECT txnr FROM log ORDER BY txnr DESC LIMIT 1;");
  sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);

  while (have_work) {
      ret = sqlite3_step(sq3_stmt);
      switch (ret) {
        case SQLITE_ROW:
          entry->txnr = sqlite3_column_int64(sq3_stmt, 0) + 1;
          ret_val = OK;
          break;

        case SQLITE_DONE:
          have_work = false;
          ret_val = OK;
          break;

        case SQLITE_BUSY:
          ret_val = LLOG_ERR;
          have_work = false;
          break;

        default:
          ret_val = LLOG_ERR;
          have_work = false;
          printf("Error looking up serial number: %s\n", sqlite3_errmsg(log->db));
          break;
        }
    }

  sqlite3_finalize(sq3_stmt);

  return ret_val;
}


int db_set_log_entry(llog_t *log, log_entry_t *entry) {
  int ret, ret_val = OK;
  sqlite3_stmt *sq3_stmt;
  char buff[BUF_SIZ];

  snprintf(buff, BUF_SIZ, "INSERT INTO log (date, UTC, call, rxrst, txrst, rxnr, txnr, rxextra, txextra, QTH, name, QRA, QRG, mode, pwr, rxQSL, txQSL, comment, station) VALUES ('%s', '%s', '%s', '%s', '%s', %" PRIu64 ", %" PRIu64 ", '%s', '%s', '%s', '%s', '%s', %f, '%s', '%s', %" PRIu64 ", %" PRIu64 ", '%s', %" PRIu64 ");", entry->date, entry->utc, entry->call, entry->rxrst, entry->txrst, entry->rxnr, entry->txnr, entry->rxextra, entry->txextra, entry->qth, entry->name, entry->qra, entry->qrg, entry->mode.name, entry->power, (uint64_t)0U, (uint64_t)0U, entry->comment, entry->station_id);
  sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);


  ret = sqlite3_step(sq3_stmt);
  switch (ret) {
    case SQLITE_ROW:
      /*This should not happen.*/
      break;

    case SQLITE_DONE:
      ret_val = OK;
      break;

    case SQLITE_BUSY:
      ret_val = LLOG_ERR;
      break;

    default:
      ret_val = LLOG_ERR;
      printf("Error inserting log entry: %s\n", sqlite3_errmsg(log->db));
      break;
    }


  sqlite3_finalize(sq3_stmt);

  return ret_val;
}


int db_get_station_entry(llog_t *log, station_entry_t *station) {
  static sqlite3_stmt *sq3_stmt;
  char buff[BUF_SIZ];
  int ret, ret_val = LLOG_ERR;
  bool finalize = true;
  char *cell;


  if (station->data_stat == db_data_init) {
      if (station->id == 0) {
          sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant FROM station ORDER BY rowid DESC;");
        }
      else{
          sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant FROM station WHERE rowid=%" PRIu64 " ORDER BY rowid DESC;", station->id);
        }
      sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);
    }

  station->data_stat = db_data_err;

  ret = sqlite3_step(sq3_stmt);
  switch (ret) {
    case SQLITE_ROW:
      station->id = sqlite3_column_int64(sq3_stmt, 0);
      cell = (char *)sqlite3_column_text(sq3_stmt, 1);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->name, cell, NAME_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 2);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->call, cell, CALL_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 3);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->QTH, cell, QTH_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 4);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->QRA, cell, QRA_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 5);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->ASL, cell, ASL_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 6);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->rig, cell, RIG_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 7);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(station->ant, cell, ANT_LEN);
      ret_val = OK;
      finalize = false;
      station->data_stat = db_data_valid;
      break;

    case SQLITE_DONE:
      station->data_stat = db_data_last;
      break;

    case SQLITE_BUSY:
      ret_val = LLOG_ERR;
      break;

    default:
      ret_val = LLOG_ERR;
      printf("Error looking up stations: %s\n", sqlite3_errmsg(log->db));
      break;
    }


  if (finalize) {
      sqlite3_finalize(sq3_stmt);
    }

  return ret_val;
}


int db_get_mode_entry(llog_t *log, mode_entry_t *mode, uint64_t *id) {
  static sqlite3_stmt *sq3_stmt;
  char buff[BUF_SIZ];
  int ret, ret_val = LLOG_ERR;
  bool finalize = true;
  char *cell;


  if (mode->data_stat == db_data_init) {
      if (id == NULL) {
          sprintf(buff, "SELECT rowid, name, default_rst, comment FROM mode ORDER BY name DESC;");
        }
      else{
          sprintf(buff, "SELECT rowid, name, default_rst, comment FROM mode WHERE rowid=%" PRIu64 ";", *id);
        }
      sqlite3_prepare_v2(log->db, buff, -1, &sq3_stmt, NULL);
    }

  mode->data_stat = db_data_err;

  ret = sqlite3_step(sq3_stmt);
  switch (ret) {
    case SQLITE_ROW:
      mode->id = sqlite3_column_int64(sq3_stmt, 0);
      cell = (char *)sqlite3_column_text(sq3_stmt, 1);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(mode->name, cell, MODE_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 2);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(mode->default_rst, cell, MODE_LEN);
      cell = (char *)sqlite3_column_text(sq3_stmt, 2);
      if (cell == NULL) {
          cell = EMPTY_STRING;
        }
      strncpy(mode->comment, cell, NAME_LEN);
      ret_val = OK;
      finalize = false;
      mode->data_stat = db_data_valid;
      break;

    case SQLITE_DONE:
      mode->data_stat = db_data_last;
      break;

    case SQLITE_BUSY:
      ret_val = LLOG_ERR;
      break;

    default:
      ret_val = LLOG_ERR;
      printf("Error looking up mode: %s\n", sqlite3_errmsg(log->db));
      break;
    }

  if (finalize) {
      sqlite3_finalize(sq3_stmt);
    }

  return ret_val;
}
