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
#include <glib.h>
#include <stdbool.h>
#include "db_sqlite.h"
#include "llog.h"
#include "llog_config.h"
#include "band.h"

#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#define BUF_SIZ 8192
#define EMPTY_STRING ""
#define R_EARTH 6371e3
#define SQLITE_FLAGS SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX


/*Run the schema file on the open log database. The schema only creates what is
  missing, so this also repairs a log file that lacks some of its tables.*/
static int db_apply_schema(llog_t *llog, const char *schema_file) {
  gchar *schema = NULL;
  GError *error = NULL;
  char *err_msg = NULL;
  int ret_val = llog_stat_ok;

  if (!g_file_get_contents(schema_file, &schema, NULL, &error)) {
    printf("Error opening schema file '%s': %s\n", schema_file, error->message);
    g_error_free(error);
    return llog_stat_file_err;
  }

  if (sqlite3_exec(llog->log_db, schema, NULL, NULL, &err_msg) != SQLITE_OK) {
    printf("Error creating database schema: %s\n", err_msg);
    sqlite3_free(err_msg);
    /*Don't leave the schema's transaction open on the connection*/
    if (!sqlite3_get_autocommit(llog->log_db)) {
      sqlite3_exec(llog->log_db, "ROLLBACK;", NULL, NULL, NULL);
    }
    ret_val = llog_stat_err;
  }

  g_free(schema);
  return ret_val;
}


/*True if the log file has every table the schema creates and at least one station.*/
static bool db_schema_complete(llog_t *llog) {
  sqlite3_stmt *stmt = NULL;
  bool complete = false;

  if (sqlite3_prepare_v2(llog->log_db,
                         "SELECT (SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN ('log', 'station')) = 2 "
                         "AND EXISTS (SELECT 1 FROM station);",
                         -1, &stmt, NULL) == SQLITE_OK) {
    complete = sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 1;
  }
  sqlite3_finalize(stmt);
  return complete;
}


/*Bring log files created by older versions up to the current schema.
  Must run after the schema of a new log file has been created, otherwise
  the schema's own CREATE TABLE statements fail on the tables added here.*/
static void db_upgrade_schema(llog_t *llog) {
  int ret;

  // Log files created before the upload feature don't have the upload table.
  ret = sqlite3_exec(llog->log_db,
                     "CREATE TABLE IF NOT EXISTS upload ("
                     "log_id INTEGER NOT NULL, "
                     "service TEXT NOT NULL, "
                     "remote_id TEXT, "
                     "uploaded_at TEXT DEFAULT CURRENT_TIMESTAMP, "
                     "UNIQUE(log_id, service));",
                     NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "Failed to create upload table: %s\n", sqlite3_errmsg(llog->log_db));
  }
}


/*Open the log and aux databases without touching the log's schema.*/
static int db_open(llog_t *llog) {
  int ret;
  int ret_val = llog_stat_ok;

  if (llog->stat == db_opened) {
    db_close(llog);
    llog->stat = db_closed;
  }

  printf("Opening log file %s\n", llog->log_file_name);
  ret = sqlite3_open_v2(llog->log_file_name, &llog->log_db, SQLITE_FLAGS, NULL);

  if (ret != SQLITE_OK) {
    printf("Error opening the log database '%s'.\n", llog->log_file_name);
    ret_val = llog_stat_err;
    goto out;
  } else {
    sqlite3_busy_timeout(llog->log_db, DATABASE_TIMEOUT);
    llog->stat = db_opened;
  }

  // Set journal mode
  ret = sqlite3_exec(llog->log_db, "PRAGMA journal_mode=DELETE;", NULL, NULL, NULL);
  if (ret != SQLITE_OK) {
    fprintf(stderr, "Failed to set journal mode: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  printf("Opening aux database\n");
  // Open the summits database
  ret = sqlite3_open_v2(llog->aux_db_path, &llog->aux_db, SQLITE_FLAGS, NULL);
  if (ret != SQLITE_OK) {
    printf("Error opening the aux database '%s'.\n", llog->aux_db_path);
    ret_val = llog_stat_err;
  } else {
    sqlite3_busy_timeout(llog->aux_db, DATABASE_TIMEOUT);
  }

out:
  return ret_val;
}


int db_sqlite_init(llog_t *llog) {
  int ret_val = db_open(llog);

  /*The log may be usable even if the aux database failed to open*/
  if (llog->stat == db_opened) {
    /*A configured log file that didn't exist was just created empty by SQLite*/
    if (!db_schema_complete(llog)) {
      printf("Log file '%s' is missing tables or stations, applying %s\n", llog->log_file_name, LLOG_DB_PATH);
      if (db_apply_schema(llog, LLOG_DB_PATH) != llog_stat_ok) {
        ret_val = llog_stat_err;
      }
    }
    db_upgrade_schema(llog);
  }
  return ret_val;
}


int db_close(llog_t *llog) {
  int ret;

  printf("Closing log database\n");
  ret = sqlite3_close_v2(llog->log_db);
  if (ret != SQLITE_OK) {
    printf("Error closing log database: %s\n", sqlite3_errmsg(llog->log_db));
  }
  llog->stat = db_closed;
  llog->log_db = NULL;
  printf("Closing aux database\n");
  ret = sqlite3_close_v2(llog->aux_db);
  if (ret != SQLITE_OK) {
    printf("Error closing aux database: %s\n", sqlite3_errmsg(llog->log_db));
  }

  return llog_stat_ok;
}


int db_merge_wal_file(llog_t *llog) {
  int ret;
  char *err_msg = NULL;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  ret = sqlite3_exec(llog->log_db, "PRAGMA wal_checkpoint(FULL);", NULL, NULL, &err_msg);
  if (ret != SQLITE_OK) {
    printf("Error merging WAL file: %s\n", err_msg);
    sqlite3_free(err_msg);
    return llog_stat_err;
  }

  return llog_stat_ok;
}


int db_check_dup_qso(llog_t *llog, log_entry_t *entry) {
  int ret, ret_val = llog_stat_ok;
  char buff[BUF_SIZ];
  bool have_work = true;


  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  sprintf(buff, "SELECT date, UTC FROM log WHERE call='%s' COLLATE NOCASE;", entry->call);
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
    snprintf(buff, BUF_SIZ,
             "SELECT rowid, date, UTC, call, rxrst, txrst, QRG, mode, "
             "SOTA_REF, S2S_REF, POTA_REF, P2P_REF, WWFF_REF, W2W_REF "
             "FROM log ORDER BY rowid DESC;");
    sqlite3_prepare_v2(llog->log_db, buff, -1, &entry->sq3_stmt, NULL);
  }

  entry->data_stat = db_data_err;

  ret = sqlite3_step(entry->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    entry->id = sqlite3_column_int64(entry->sq3_stmt, 0);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 1);
    strncpy(entry->date, cell != NULL ? cell : "", NAME_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 2);
    strncpy(entry->utc, cell != NULL ? cell : "", NAME_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 3);
    strncpy(entry->call, cell != NULL ? cell : "", CALL_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 4);
    strncpy(entry->rxrst, cell != NULL ? cell : "", RST_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 5);
    strncpy(entry->txrst, cell != NULL ? cell : "", RST_LEN);

    entry->qrg = sqlite3_column_double(entry->sq3_stmt, 6);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 7);
    strncpy(entry->mode.name, cell != NULL ? cell : "", MODE_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 8);  // SOTA_REF
    strncpy(entry->sota_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 9);  // S2S_REF
    strncpy(entry->s2s_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 10); // POTA_REF
    strncpy(entry->pota_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 11); // P2P_REF
    strncpy(entry->p2p_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 12); // WWFF_REF
    strncpy(entry->wwff_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 13); // W2W_REF
    strncpy(entry->w2w_ref, cell != NULL ? cell : "", SPW_REF_LEN);

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
             "SELECT log.rowid, log.date, log.UTC, log.call, log.rxrst, log.txrst, log.QRA ,log.QRG, log.mode, "
             "log.SOTA_REF, log.S2S_REF, log.POTA_REF, log.P2P_REF, log.WWFF_REF, log.W2W_REF, "
             "station.rowid, station.name, station.CALL, station.QTH, station.QRA, station.ASL, station.rig, station.ant, "
             "log.name, log.QTH, log.pwr, log.comment, "
             "station.OPERATOR_CALL, station.OPERATOR_NAME, station.comment "
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
    strncpy(entry->date, cell != NULL ? cell : "", NAME_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 2);
    strncpy(entry->utc, cell != NULL ? cell : "", NAME_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 3);
    strncpy(entry->call, cell != NULL ? cell : "", CALL_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 4);
    strncpy(entry->rxrst, cell != NULL ? cell : "", RST_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 5);
    strncpy(entry->txrst, cell != NULL ? cell : "", RST_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 6);
    strncpy(entry->qra, cell != NULL ? cell : "", MODE_LEN);

    entry->qrg = sqlite3_column_double(entry->sq3_stmt, 7);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 8);
    strncpy(entry->mode.name, cell != NULL ? cell : "", MODE_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 9);  // SOTA_REF
    strncpy(entry->sota_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 10);  // S2S_REF
    strncpy(entry->s2s_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 11); // POTA_REF
    strncpy(entry->pota_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 12); // P2P_REF
    strncpy(entry->p2p_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 13); // WWFF_REF
    strncpy(entry->wwff_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 14); // W2W_REF
    strncpy(entry->w2w_ref, cell != NULL ? cell : "", SPW_REF_LEN);

    /* Station columns — start at 14 */
    station->id = sqlite3_column_int64(entry->sq3_stmt, 15);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 16);
    strncpy(station->name, cell != NULL ? cell : "", NAME_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 17);
    strncpy(station->call, cell != NULL ? cell : "", CALL_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 18);
    strncpy(station->QTH, cell != NULL ? cell : "", QTH_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 19);
    strncpy(station->QRA, cell != NULL ? cell : "", QRA_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 20);
    strncpy(station->ASL, cell != NULL ? cell : "", ASL_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 21);
    strncpy(station->rig, cell != NULL ? cell : "", RIG_LEN);

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 22);
    strncpy(station->ant, cell != NULL ? cell : "", ANT_LEN);

    /* Additional log columns */
    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 23);
    snprintf(entry->name, NAME_LEN, "%s", cell != NULL ? cell : "");

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 24);
    snprintf(entry->qth, QTH_LEN, "%s", cell != NULL ? cell : "");

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 25);
    snprintf(entry->power, PWR_LEN, "%s", cell != NULL ? cell : "");

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 26);
    snprintf(entry->comment, COMMENT_LEN, "%s", cell != NULL ? cell : "");

    /* Additional station columns */
    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 27);
    snprintf(station->operator_call, CALL_LEN, "%s", cell != NULL ? cell : "");

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 28);
    snprintf(station->operator_name, NAME_LEN, "%s", cell != NULL ? cell : "");

    cell = (char *)sqlite3_column_text(entry->sq3_stmt, 29);
    snprintf(station->comment, COMMENT_LEN, "%s", cell != NULL ? cell : "");

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


int db_get_max_nr(llog_t *llog, log_entry_t *entry, double qrg_mhz) {
  int ret, ret_val = llog_stat_ok;
  char buff[BUF_SIZ];
  bool have_work = true;
  double band_low, band_high;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (llog->band_nr) {
    if (band_get_edges(qrg_mhz, &band_low, &band_high) != 0) {
    // QRG not in any known band — return 0
    entry->txnr = 0;
    return ret_val;
    }

    snprintf(buff, BUF_SIZ,
           "SELECT txnr FROM log WHERE qrg >= %f AND qrg < %f ORDER BY txnr DESC LIMIT 1;",
           band_low, band_high);
  } else {
    sprintf(buff, "SELECT txnr FROM log ORDER BY txnr DESC LIMIT 1;");
  }

  entry->txnr = 1;

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
      printf("Error looking up serial number by band: %s\n", sqlite3_errmsg(llog->log_db));
      break;
    }
  }

  sqlite3_finalize(entry->sq3_stmt);
  return ret_val;
}


int db_set_log_entry(llog_t *llog, log_entry_t *entry) {
  sqlite3_stmt *stmt = NULL;
  int ret_val = llog_stat_err;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->log_db,
                         "INSERT INTO log (date, UTC, call, rxrst, txrst, rxnr, txnr, rxextra, txextra, QTH, name, "
                         "QRA, QRG, mode, pwr, rxQSL, txQSL, comment, station, "
                         "SOTA_REF, S2S_REF, POTA_REF, P2P_REF, WWFF_REF, W2W_REF) VALUES "
                         "(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, 0, 0, ?16, ?17, "
                         "?18, ?19, ?20, ?21, ?22, ?23);", -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing log entry insert: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_text(stmt, 1, entry->date, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, entry->utc, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, entry->call, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, entry->rxrst, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, entry->txrst, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 6, (sqlite3_int64)entry->rxnr);
  sqlite3_bind_int64(stmt, 7, (sqlite3_int64)entry->txnr);
  sqlite3_bind_text(stmt, 8, entry->rxextra, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 9, entry->txextra, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 10, entry->qth, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 11, entry->name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 12, entry->qra, -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 13, entry->qrg);
  sqlite3_bind_text(stmt, 14, entry->mode.name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 15, entry->power, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 16, entry->comment, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 17, (sqlite3_int64)entry->station_id);
  sqlite3_bind_text(stmt, 18, entry->sota_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 19, entry->s2s_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 20, entry->pota_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 21, entry->p2p_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 22, entry->wwff_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 23, entry->w2w_ref, -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt) == SQLITE_DONE) {
    ret_val = llog_stat_ok;
  } else {
    printf("Error inserting log entry: %s\n", sqlite3_errmsg(llog->log_db));
  }

  sqlite3_finalize(stmt);
  return ret_val;
}


/*Read every editable column of one QSO, selected by entry->id.*/
int db_get_log_entry_by_id(llog_t *llog, log_entry_t *entry) {
  sqlite3_stmt *stmt = NULL;
  int ret, ret_val = llog_stat_err;
  const char *cell;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->log_db,
                         "SELECT date, UTC, call, rxrst, txrst, rxnr, txnr, rxextra, txextra, QTH, name, "
                         "QRA, QRG, mode, pwr, comment, station, "
                         "SOTA_REF, S2S_REF, POTA_REF, P2P_REF, WWFF_REF, W2W_REF "
                         "FROM log WHERE rowid=?1;", -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing log entry query: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_int64(stmt, 1, (sqlite3_int64)entry->id);

#define LOG_TEXT_COL(col, dst) \
  cell = (const char *)sqlite3_column_text(stmt, col); \
  snprintf(dst, sizeof(dst), "%s", cell != NULL ? cell : "")

  ret = sqlite3_step(stmt);
  if (ret == SQLITE_ROW) {
    LOG_TEXT_COL(0, entry->date);
    LOG_TEXT_COL(1, entry->utc);
    LOG_TEXT_COL(2, entry->call);
    LOG_TEXT_COL(3, entry->rxrst);
    LOG_TEXT_COL(4, entry->txrst);
    entry->rxnr = (uint64_t)sqlite3_column_int64(stmt, 5);
    entry->txnr = (uint64_t)sqlite3_column_int64(stmt, 6);
    LOG_TEXT_COL(7, entry->rxextra);
    LOG_TEXT_COL(8, entry->txextra);
    LOG_TEXT_COL(9, entry->qth);
    LOG_TEXT_COL(10, entry->name);
    LOG_TEXT_COL(11, entry->qra);
    entry->qrg = sqlite3_column_double(stmt, 12);
    LOG_TEXT_COL(13, entry->mode.name);
    LOG_TEXT_COL(14, entry->power);
    LOG_TEXT_COL(15, entry->comment);
    entry->station_id = (uint64_t)sqlite3_column_int64(stmt, 16);
    LOG_TEXT_COL(17, entry->sota_ref);
    LOG_TEXT_COL(18, entry->s2s_ref);
    LOG_TEXT_COL(19, entry->pota_ref);
    LOG_TEXT_COL(20, entry->p2p_ref);
    LOG_TEXT_COL(21, entry->wwff_ref);
    LOG_TEXT_COL(22, entry->w2w_ref);
    ret_val = llog_stat_ok;
  } else if (ret == SQLITE_DONE) {
    ret_val = llog_no_data;
  } else {
    printf("Error reading log entry: %s\n", sqlite3_errmsg(llog->log_db));
  }

#undef LOG_TEXT_COL

  sqlite3_finalize(stmt);
  return ret_val;
}


/*Overwrite the QSO selected by entry->id with the contents of entry.*/
int db_update_log_entry(llog_t *llog, log_entry_t *entry) {
  sqlite3_stmt *stmt = NULL;
  int ret_val = llog_stat_err;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->log_db,
                         "UPDATE log SET date=?1, UTC=?2, call=?3, rxrst=?4, txrst=?5, rxnr=?6, txnr=?7, "
                         "rxextra=?8, txextra=?9, QTH=?10, name=?11, QRA=?12, QRG=?13, mode=?14, pwr=?15, "
                         "comment=?16, station=?17, SOTA_REF=?18, S2S_REF=?19, POTA_REF=?20, P2P_REF=?21, "
                         "WWFF_REF=?22, W2W_REF=?23 WHERE rowid=?24;", -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing log entry update: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_text(stmt, 1, entry->date, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, entry->utc, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, entry->call, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, entry->rxrst, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, entry->txrst, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 6, (sqlite3_int64)entry->rxnr);
  sqlite3_bind_int64(stmt, 7, (sqlite3_int64)entry->txnr);
  sqlite3_bind_text(stmt, 8, entry->rxextra, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 9, entry->txextra, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 10, entry->qth, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 11, entry->name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 12, entry->qra, -1, SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 13, entry->qrg);
  sqlite3_bind_text(stmt, 14, entry->mode.name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 15, entry->power, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 16, entry->comment, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 17, (sqlite3_int64)entry->station_id);
  sqlite3_bind_text(stmt, 18, entry->sota_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 19, entry->s2s_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 20, entry->pota_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 21, entry->p2p_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 22, entry->wwff_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 23, entry->w2w_ref, -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 24, (sqlite3_int64)entry->id);

  if (sqlite3_step(stmt) == SQLITE_DONE) {
    ret_val = llog_stat_ok;
  } else {
    printf("Error updating log entry: %s\n", sqlite3_errmsg(llog->log_db));
  }

  sqlite3_finalize(stmt);
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
      sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant, OPERATOR_CALL, OPERATOR_NAME, comment FROM station ORDER BY rowid DESC;");
    } else {
      sprintf(buff, "SELECT rowid, name, CALL, QTH, QRA, ASL, rig, ant, OPERATOR_CALL, OPERATOR_NAME, comment FROM station WHERE rowid=%" PRIu64 " ORDER BY rowid DESC;", station->id);
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
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 8);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->operator_call, cell, CALL_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 9);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->operator_name, cell, NAME_LEN);
    cell = (char *)sqlite3_column_text(station->sq3_stmt, 10);
    if (cell == NULL) {
      cell = EMPTY_STRING;
    }
    strncpy(station->comment, cell, COMMENT_LEN);
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


/*Insert the station if its id is 0, update the row otherwise.
  On insert, the new rowid is stored in station->id.*/
int db_set_station_entry(llog_t *llog, station_entry_t *station) {
  sqlite3_stmt *stmt = NULL;
  int ret, ret_val = llog_stat_err;
  const char *sql;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (station->id == 0) {
    sql = "INSERT INTO station (name, CALL, QTH, QRA, ASL, rig, ant, OPERATOR_CALL, OPERATOR_NAME, comment) "
          "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10);";
  } else {
    sql = "UPDATE station SET name=?1, CALL=?2, QTH=?3, QRA=?4, ASL=?5, rig=?6, ant=?7, "
          "OPERATOR_CALL=?8, OPERATOR_NAME=?9, comment=?10 WHERE rowid=?11;";
  }

  if (sqlite3_prepare_v2(llog->log_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing station statement: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_text(stmt, 1, station->name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, station->call, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, station->QTH, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, station->QRA, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, station->ASL, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 6, station->rig, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 7, station->ant, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 8, station->operator_call, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 9, station->operator_name, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 10, station->comment, -1, SQLITE_TRANSIENT);
  if (station->id != 0) {
    sqlite3_bind_int64(stmt, 11, (sqlite3_int64)station->id);
  }

  ret = sqlite3_step(stmt);
  if (ret == SQLITE_DONE) {
    ret_val = llog_stat_ok;
    if (station->id == 0) {
      station->id = (uint64_t)sqlite3_last_insert_rowid(llog->log_db);
    }
  } else {
    printf("Error writing station entry: %s\n", sqlite3_errmsg(llog->log_db));
  }

  sqlite3_finalize(stmt);
  return ret_val;
}


/*Number of QSOs logged with the given station.*/
int db_get_station_use_count(llog_t *llog, uint64_t station_id, uint64_t *count) {
  sqlite3_stmt *stmt = NULL;
  int ret_val = llog_stat_err;

  *count = 0;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->log_db, "SELECT COUNT(*) FROM log WHERE station=?1;", -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing station count: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_int64(stmt, 1, (sqlite3_int64)station_id);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    *count = (uint64_t)sqlite3_column_int64(stmt, 0);
    ret_val = llog_stat_ok;
  } else {
    printf("Error counting station use: %s\n", sqlite3_errmsg(llog->log_db));
  }

  sqlite3_finalize(stmt);
  return ret_val;
}


int db_delete_station_entry(llog_t *llog, uint64_t station_id) {
  sqlite3_stmt *stmt = NULL;
  int ret_val = llog_stat_err;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->log_db, "DELETE FROM station WHERE rowid=?1;", -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing station delete: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_int64(stmt, 1, (sqlite3_int64)station_id);

  if (sqlite3_step(stmt) == SQLITE_DONE) {
    ret_val = llog_stat_ok;
  } else {
    printf("Error deleting station entry: %s\n", sqlite3_errmsg(llog->log_db));
  }

  sqlite3_finalize(stmt);
  return ret_val;
}


int db_get_mode_entry(llog_t *llog, mode_entry_t *mode, uint64_t *id) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->aux_db == NULL) {
    return llog_stat_err;
  }

  if (mode->data_stat == db_data_init) {
    if (id == NULL) {
      sprintf(buff, "SELECT rowid, name, default_rst, comment FROM mode ORDER BY name ASC;");
    } else {
      sprintf(buff, "SELECT rowid, name, default_rst, comment FROM mode WHERE rowid=%" PRIu64 ";", *id);
    }
    sqlite3_prepare_v2(llog->aux_db, buff, -1, &mode->sq3_stmt, NULL);
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

  if (finalize || id != NULL) {
    sqlite3_finalize(mode->sq3_stmt);
  }

  return ret_val;
}


int db_create_from_schema(llog_t *llog, const char *schema_file) {
  int ret_val = db_open(llog);

  if (ret_val != llog_stat_ok) {
    printf("Error opening database.\n");
    return ret_val;
  }

  ret_val = db_apply_schema(llog, schema_file);
  db_upgrade_schema(llog);

  return ret_val;
}

int db_get_sota_entry(llog_t *llog, spw_entry_t *summit, position_t *pos) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->aux_db == NULL) {
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
    sqlite3_prepare_v2(llog->aux_db, buff, -1, &summit->sq3_stmt, NULL);
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
    strncpy(summit->ref, cell, SPW_REF_LEN);

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
    printf("Error looking up summit: %s\n", sqlite3_errmsg(llog->aux_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(summit->sq3_stmt);
  }

  return ret_val;
}


int db_get_pota_entry(llog_t *llog, spw_entry_t *park, position_t *pos) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->aux_db == NULL) {
    park->data_stat = db_data_err;
    return llog_stat_err;
  }

#define POTA_QUERY "SELECT \
     rowid, reference, name, active, entity_id, location, latitude, longitude, grid, ( \
        6371 * acos( \
            cos(radians(%f)) * cos(radians(latitude)) * \
            cos(radians(longitude) - radians(%f)) + \
            sin(radians(%f)) * sin(radians(latitude)) \
        ) \
    ) AS distance \
  FROM pota_park_data \
  ORDER BY distance ASC \
  LIMIT 1;"

  if (park->data_stat == db_data_init) {
    if (pos == NULL) {
      snprintf(buff, BUF_SIZ, "SELECT rowid, reference, name, active, entity_id, location, latitude, longitude, grid FROM pota_park_data;");
    } else {
      snprintf(buff, BUF_SIZ, POTA_QUERY, pos->lat, pos->lon, pos->lat);
    }
    sqlite3_prepare_v2(llog->aux_db, buff, -1, &park->sq3_stmt, NULL);
  }

  park->data_stat = db_data_err;

  ret = sqlite3_step(park->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    park->id = sqlite3_column_int64(park->sq3_stmt, 0);

    cell = (char *)sqlite3_column_text(park->sq3_stmt, 1); // reference
    strncpy(park->ref, cell != NULL ? cell : EMPTY_STRING, SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(park->sq3_stmt, 2); // name
    strncpy(park->name, cell != NULL ? cell : EMPTY_STRING, SPW_REF_LEN);

    park->is_active = (uint8_t)sqlite3_column_int(park->sq3_stmt, 3);
    park->entity_id = (uint32_t)sqlite3_column_int(park->sq3_stmt, 4);

    cell = (char *)sqlite3_column_text(park->sq3_stmt, 5); // location
    strncpy(park->location, cell != NULL ? cell : EMPTY_STRING, MAX_ENTITY_LEN);

    park->position.lat = sqlite3_column_double(park->sq3_stmt, 6);
    park->position.lon = sqlite3_column_double(park->sq3_stmt, 7);

    cell = (char *)sqlite3_column_text(park->sq3_stmt, 8); // grid
    strncpy(park->grid, cell != NULL ? cell : EMPTY_STRING, QRA_LEN);

    ret_val = llog_stat_ok;
    finalize = false;
    park->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    park->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up POTA park: %s\n", sqlite3_errmsg(llog->aux_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(park->sq3_stmt);
  }

  return ret_val;
}


int db_get_wwff_entry(llog_t *llog, spw_entry_t *area, position_t *pos) {
  char buff[BUF_SIZ];
  int ret, ret_val = llog_stat_err;
  bool finalize = true;
  char *cell;

  if (llog->aux_db == NULL) {
    area->data_stat = db_data_err;
    return llog_stat_err;
  }

#define WWFF_QUERY "SELECT \
     rowid, reference, status, name, latitude, longitude, iaru_locator, \
     valid_from, valid_to, program, dxcc, continent, country, ( \
        6371 * acos( \
            cos(radians(%f)) * cos(radians(latitude)) * \
            cos(radians(longitude) - radians(%f)) + \
            sin(radians(%f)) * sin(radians(latitude)) \
        ) \
    ) AS distance \
  FROM wwff_area_data \
  ORDER BY distance ASC \
  LIMIT 1;"

  if (area->data_stat == db_data_init) {
    if (pos == NULL) {
      snprintf(buff, BUF_SIZ, "SELECT rowid, reference, status, name, latitude, longitude, iaru_locator, valid_from, valid_to, program, dxcc, continent, country FROM wwff_area_data;");
    } else {
      snprintf(buff, BUF_SIZ, WWFF_QUERY, pos->lat, pos->lon, pos->lat);
    }
    sqlite3_prepare_v2(llog->aux_db, buff, -1, &area->sq3_stmt, NULL);
  }

  area->data_stat = db_data_err;

  ret = sqlite3_step(area->sq3_stmt);
  switch (ret) {
  case SQLITE_ROW:
    area->id = sqlite3_column_int64(area->sq3_stmt, 0);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 1); // reference
    strncpy(area->ref, cell != NULL ? cell : EMPTY_STRING, SPW_REF_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 2); // status
    strncpy(area->status, cell != NULL ? cell : EMPTY_STRING, STATUS_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 3); // name
    strncpy(area->name, cell != NULL ? cell : EMPTY_STRING, SPW_REF_LEN);

    area->position.lat = sqlite3_column_double(area->sq3_stmt, 4);
    area->position.lon = sqlite3_column_double(area->sq3_stmt, 5);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 6); // iaru_locator -> grid
    strncpy(area->grid, cell != NULL ? cell : EMPTY_STRING, QRA_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 7); // valid_from
    strncpy(area->valid_from, cell != NULL ? cell : EMPTY_STRING, DATE_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 8); // valid_to
    strncpy(area->valid_to, cell != NULL ? cell : EMPTY_STRING, DATE_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 9); // program
    strncpy(area->wwff_program, cell != NULL ? cell : EMPTY_STRING, WWFF_PROGRAM_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 10); // dxcc
    strncpy(area->dxcc, cell != NULL ? cell : EMPTY_STRING, DXCC_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 11); // continent
    strncpy(area->continent, cell != NULL ? cell : EMPTY_STRING, CONTINENT_LEN);

    cell = (char *)sqlite3_column_text(area->sq3_stmt, 12); // country
    strncpy(area->country, cell != NULL ? cell : EMPTY_STRING, COUNTRY_LEN);

    ret_val = llog_stat_ok;
    finalize = false;
    area->data_stat = db_data_valid;
    break;

  case SQLITE_DONE:
    ret_val = llog_stat_ok;
    area->data_stat = db_data_last;
    break;

  case SQLITE_BUSY:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    printf("Error looking up WWFF area: %s\n", sqlite3_errmsg(llog->aux_db));
    break;
  }

  if (finalize) {
    sqlite3_finalize(area->sq3_stmt);
  }

  return ret_val;
}


/* Returns true if the QSO with the given log rowid has already been uploaded to the service. */
bool db_is_uploaded(llog_t *llog, uint64_t log_id, const char *service) {
  sqlite3_stmt *stmt;
  bool uploaded = false;

  if (llog->log_db == NULL) {
    return false;
  }

  if (sqlite3_prepare_v2(llog->log_db,
                         "SELECT 1 FROM upload WHERE log_id = ? AND service = ?;",
                         -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error looking up upload state: %s\n", sqlite3_errmsg(llog->log_db));
    return false;
  }

  sqlite3_bind_int64(stmt, 1, (sqlite3_int64)log_id);
  sqlite3_bind_text(stmt, 2, service, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    uploaded = true;
  }

  sqlite3_finalize(stmt);
  return uploaded;
}


/* Looks up the ADIF mode a mode name belongs to, e.g. OLIVIA for "OLIVIA 8/250".
 * super_mode is left empty if the mode is not in the mode table or has no super mode.
 */
int db_get_super_mode(llog_t *llog, const char *mode, char *super_mode, size_t len) {
  sqlite3_stmt *stmt;
  const char *cell;

  super_mode[0] = '\0';

  if (llog->aux_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->aux_db, "SELECT super_mode FROM mode WHERE name = ? LIMIT 1;",
                         -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error looking up super mode: %s\n", sqlite3_errmsg(llog->aux_db));
    return llog_stat_err;
  }

  sqlite3_bind_text(stmt, 1, mode, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    cell = (const char *)sqlite3_column_text(stmt, 0);
    snprintf(super_mode, len, "%s", cell != NULL ? cell : "");
  }

  sqlite3_finalize(stmt);
  return llog_stat_ok;
}


int db_set_uploaded(llog_t *llog, uint64_t log_id, const char *service, const char *remote_id) {
  sqlite3_stmt *stmt;
  int ret_val = llog_stat_ok;

  if (llog->log_db == NULL) {
    return llog_stat_err;
  }

  if (sqlite3_prepare_v2(llog->log_db,
                         "INSERT OR REPLACE INTO upload (log_id, service, remote_id) VALUES (?, ?, ?);",
                         -1, &stmt, NULL) != SQLITE_OK) {
    printf("Error preparing upload insert: %s\n", sqlite3_errmsg(llog->log_db));
    return llog_stat_err;
  }

  sqlite3_bind_int64(stmt, 1, (sqlite3_int64)log_id);
  sqlite3_bind_text(stmt, 2, service, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, remote_id, -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    printf("Error recording upload: %s\n", sqlite3_errmsg(llog->log_db));
    ret_val = llog_stat_err;
  }

  sqlite3_finalize(stmt);
  return ret_val;
}
