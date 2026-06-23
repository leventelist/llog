/*	This is llog, a minimalist HAM logging software.
 *	Copyright (C) 2013-2025  Levente Kovacs
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>
#include <gio/gio.h>

#include "llog.h"
#include "db_sqlite.h"
#include "main_window.h"
#include "conf.h"
#include "position.h"
#include "xml_client.h"
#include "llog_config.h"

#define BUF_SIZ 1024
#define CND_SIZ 2048

static llog_t llog;
static station_entry_t initial_station;

/*Define configuration items*/
static config_attribute_t llog_ca[] = {
  { "log_filename", CONFIG_String, llog.log_file_name },
  { "station", CONFIG_ULLInteger, &initial_station.id },
  { "gpsd_host", CONFIG_String, llog.gpsd_host },
  { "gpsd_port", CONFIG_ULLInteger, &llog.gpsd_port },
  { "export_filename", CONFIG_String, llog.export_file_name },
  { "tx_nr_per_band", CONFIG_Boolean, &llog.band_nr },
  { NULL, CONFIG_Unused, NULL }
};


llog_t *llog_set_default(void) {
  llog.log_db = NULL;
  llog.log_file_name[0] = '\0';
  llog.stat = db_closed;


  llog.ca = llog_ca;

  /*Some sensible defaults for GPS configuration*/
  sprintf(llog.gpsd_host, "localhost");
  llog.gpsd_port = 2947;

  llog.export_file_name[0] = '\0';

  sprintf(llog.xmlrpc_host, "localhost");
  llog.xmlrpc_port = 7362;

  llog.force_generate_aux_db = false;
  return &llog;
}


llog_t *llog_init() {
  int ret;

  // Build the aux database.

  llog_ensure_aux_db(llog.force_generate_aux_db);


	ret = llog_parse_config_file();

	if (ret != llog_stat_ok) {
		goto out;
	}

  llog_save_config_file();
	llog_open_db();

  initial_station.data_stat = db_data_init;
  while (initial_station.data_stat != db_data_last) {
    ret = db_get_station_entry(&llog, &initial_station);
    if (ret != llog_stat_ok) {
      break;
    }
  }

  /*Initialize GPS*/
  position_init(llog.gpsd_host, llog.gpsd_port, main_window_update_position_labels);

  xml_client_init(llog.xmlrpc_host, llog.xmlrpc_port);

out:
  return &llog;
}


int llog_ensure_aux_db(bool force) {

  char cmd[CND_SIZ];
  char *home;

  home = getenv("HOME");

  snprintf(llog.aux_db_path, FILE_LEN, "%s/.config/%s/aux_db.sqlite", home, PROGRAM_NAME);
  printf("Database to generate: %s\n", llog.aux_db_path);

  if (force == false && access(llog.aux_db_path, F_OK) == 0) {
    return 0;  /* already exists */
  }


  snprintf(cmd, CND_SIZ, "python3 %s --output %s", AUX_DB_GEN_PATH, llog.aux_db_path);

  printf("Generating auxiliary database...\n%s\n", cmd);
  int ret = system(cmd);

  return ret;
}


int llog_set_log_file(char *log_file_name, bool check) {
  char real_log_file_name[FILE_LEN];
  int ret_val;

  if (check) {
    if (realpath(log_file_name, real_log_file_name) == NULL) {
      ret_val = llog_stat_file_err;
      perror("realpath");
    } else {
      ret_val = llog_stat_ok;
      strncpy(llog.log_file_name, real_log_file_name, FILE_LEN);
    }
  } else {
    ret_val = llog_stat_ok;
    strncpy(llog.log_file_name, log_file_name, FILE_LEN);
  }

  llog.log_db = NULL;
  llog.stat = db_closed;

  return ret_val;
}


int llog_parse_config_file(void) {
  int ret_val;
  int ret = llog_stat_ok;

  ret_val = config_file_read(llog.ca, PROGRAM_NAME);
  if (ret_val == CONF_READ_ERR) {
    char *path;
    path = get_config_file_path();
    printf("Error reading config file `%s`\n", path);
    ret = llog_stat_err;
  }

  return ret;
}


int llog_open_db(void) {
  int ret;

  ret = db_sqlite_init(&llog);

  return ret;
}


int llog_get_initial_station(station_entry_t **station) {
  *station = &initial_station;
  return llog_stat_ok;
}


int llog_save_config_file(void) {
  int ret;

  ret = config_print_file(llog.ca);
  return ret;
}


int llog_get_log_file_path(char * *path) {
  *path = llog.log_file_name;
  return llog_stat_ok;
}

int llog_add_log_entries(void) {
  int ret_val = llog_stat_ok;
  log_entry_t entry;

  entry.data_stat = db_data_init;

  main_window_clear_log_list();

  while (entry.data_stat != db_data_last) {
    ret_val = db_get_log_entries(&llog, &entry);
    if (ret_val != llog_stat_ok) {
      break;
    }
    if (entry.data_stat == db_data_valid) {
      main_window_add_log_entry_to_list(&entry);
    }else {
      break;
    }
  }

  return ret_val;
}


int llog_log_entry(log_entry_t *entry) {
  int ret_val;

  ret_val = db_set_log_entry(&llog, entry);

  return ret_val;
}


void llog_reset_entry(log_entry_t *entry) {
  entry->date[0] = '\0';
  entry->utc[0] = '\0';
  entry->rxrst[0] = '\0';
  entry->call[0] = '\0';
  entry->name[0] = '\0';

  if (entry->mode.id != 0) {
    strcpy(entry->txrst, entry->mode.default_rst);
  }

  entry->rxrst[0] = '\0';
  entry->qth[0] = '\0';
  entry->qra[0] = '\0';
  entry->rxnr = 0;
  entry->rxextra[0] = '\0';
  entry->txextra[0] = '\0';
  entry->comment[0] = '\0';
}


int llog_add_station_entries(void) {
  int ret_val = llog_stat_ok;
  station_entry_t station;

  station.data_stat = db_data_init;

  main_window_clear_station_list();

  while (station.data_stat != db_data_last) {
    station.id = 0;
    db_get_station_entry(&llog, &station);
    if (station.data_stat == db_data_valid) {
      main_window_add_station_entry_to_list(&station);
    }else {
      break;
    }
  }

  return ret_val;
}


int llog_add_modes_entries(void) {
  int ret_val = llog_stat_ok;
  mode_entry_t mode;

  mode.data_stat = db_data_init;

  main_window_clear_modes_list();

  while (mode.data_stat != db_data_last) {
    db_get_mode_entry(&llog, &mode, NULL);
    if (mode.data_stat == db_data_valid) {
      main_window_add_mode_entry_to_list(&mode);
    } else {
      break;
    }
  }

  return ret_val;
}


int llog_get_default_rst(char *default_rst, uint64_t mode_id) {
  mode_entry_t mode;
  int ret;

  mode.data_stat = db_data_init;

  ret = db_get_mode_entry(&llog, &mode, &mode_id);
  if (ret == llog_stat_ok) {
    strncpy(default_rst, mode.default_rst, MODE_LEN);
  }

  return ret;
}


void llog_get_time(log_entry_t *entry) {
  struct timeval qso_time;
  struct tm qso_bdt;

  gettimeofday(&qso_time, NULL);
  gmtime_r(&qso_time.tv_sec, &qso_bdt);
  sprintf(entry->date, "%d-%02d-%02d", 1900 + qso_bdt.tm_year, 1 + qso_bdt.tm_mon, qso_bdt.tm_mday);
  sprintf(entry->utc, "%02d%02d", qso_bdt.tm_hour, qso_bdt.tm_min);
}


int llog_check_dup_qso(log_entry_t *entry) {
  int ret_val;

  entry->data_stat = db_data_init;

  ret_val = db_check_dup_qso(&llog, entry);

  return ret_val;
}


int llog_load_static_data(log_entry_t *entry) {
  int ret;


  ret = llog_add_log_entries();
  if (ret != llog_stat_ok) {
    goto out;
  }
  ret = llog_add_station_entries();
  if (ret != llog_stat_ok) {
    goto out;
  }
  ret = llog_add_modes_entries();
  if (ret != llog_stat_ok) {
    goto out;
  }
  ret = db_get_max_nr(&llog, entry, entry->qrg);
  if (ret != llog_stat_ok) {
    goto out;
  }

out:
  return ret;
}


void llog_print_log_data(log_entry_t *entry) {
  printf("\nCall [%s]\nOperator's name: [%s]\n", entry->call, entry->name);
  printf("RXRST [%s]\nTXRST [%s]\n", entry->rxrst, entry->txrst);
  printf("QTH [%s]\nQRA [%s]\n", entry->qth, entry->qra);
  printf("QRG [%f]\nMode [%s]\nPower: [%s]\n", entry->qrg, entry->mode.name, entry->power);
  printf("RXNR [%04" PRIu64 "]\nTXNR [%04" PRIu64 "]\n", entry->rxnr, entry->txnr);
  printf("RX_EXTRA [%s]\nTX_EXTRA [%s]\n", entry->rxextra, entry->txextra);
  printf("Comment [%s]\n\n", entry->comment);
  return;
}


void llog_open_qrz_url(const char *call) {
  char url[BUF_SIZ];
  GError *error = NULL;

  if (call == NULL || call[0] == '\0') {
    return;
  }

  snprintf(url, BUF_SIZ, "https://www.qrz.com/db/%s", call);

  if (!g_app_info_launch_default_for_uri(url, NULL, &error)) {
    g_printerr("Error launching browser: %s\n", error->message);
    g_error_free(error);
  }
}


void llog_shutdown(void) {
  printf("Shutting down llog\n");
  position_stop();
  db_close(&llog);
  xml_client_shutdown();
}
