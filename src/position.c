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


#include <gps.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>
#include <float.h>
#include <string.h>

#include "position.h"
#include "llog.h"


typedef enum {
  position_stat_uninit,
  position_stat_init,
  position_stat_running
} position_status_t;

/*Module Constants*/

#define R_EARTH 6371e3F

/*Module variables*/
static pthread_t thread_id;
static position_status_t position_status = position_stat_uninit;
static gps_data_real_t gpsdata;

static position_t pos;
static pthread_mutex_t pos_mutex = PTHREAD_MUTEX_INITIALIZER;

static position_callback_t position_callback = NULL;

/*Module functions*/
static void position_register_callback(position_callback_t callback);
static void position_start_thread(void);
static void position_set(position_t *new_pos);

/*Interface functions*/
int position_init(char *host, uint64_t port, position_callback_t callback) {
  char port_str[16];
  int ret;
  int ret_val;

  printf("Initializing GPSd interface %s, %ld\n", host, port);

  if (position_status != position_stat_uninit) {
    position_stop();
  }

  position_status = position_stat_init;

  snprintf(port_str, sizeof(port_str), "%lu", port);

  ret = gps_open(host, port_str, &gpsdata);

  switch (ret) {
  case 0:
    ret_val = llog_stat_ok;
    position_status = position_stat_init;
    position_register_callback(callback);
    position_start_thread();
    break;

  case -1:
    ret_val = llog_stat_err;
    break;

  default:
    ret_val = llog_stat_err;
    break;
  }

  gps_stream(&gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);

  return ret_val;
}

static void position_register_callback(position_callback_t callback) {
  position_callback = callback;
}

void position_get(position_t *out_pos) {
  pthread_mutex_lock(&pos_mutex);
  *out_pos = pos;
  pthread_mutex_unlock(&pos_mutex);
}


int position_step(void) {
  bool data_ready;
  int ret;
  int ret_val;
  bool progress = false;
  position_t new_pos;


  new_pos.alt = NAN;
  new_pos.lat = NAN;
  new_pos.lon = NAN;
  new_pos.speed = NAN;
  new_pos.fix = MODE_NOT_SEEN;

  data_ready = gps_waiting(&gpsdata, POSITION_TIMEOUT_USEC);

  if (data_ready) {
    ret = gps_read(&gpsdata, NULL, 0);
    if (ret == -1) {
      ret_val = llog_stat_err;
    } else {
      if (gpsdata.set & LATLON_SET) {
        new_pos.lat = gpsdata.fix.latitude;
        new_pos.lon = gpsdata.fix.longitude;
        progress = true;
      }

      if (gpsdata.set & ALTITUDE_SET) {
        new_pos.alt = gpsdata.fix.altMSL;
        progress = true;
      }

      if (gpsdata.set & SPEED_SET) {
        new_pos.speed = gpsdata.fix.speed;
        progress = true;
      }
      if (progress) {
        new_pos.fix = gpsdata.fix.mode;
        position_set(&new_pos);
        ret_val = llog_stat_ok;
      } else {
        ret_val = llog_no_data;
      }
    }
  } else {
    ret_val = llog_no_data;
  }


  return ret_val;
}


static void *position_thread(void *user_data) {
  (void)user_data;
  position_t local_pos;

  int ret_val;

  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  timespec_t ts;
  ts.tv_sec = 1;
  ts.tv_nsec = 0;

  while (1) {
    pthread_testcancel();
    position_status = position_stat_running;
    ret_val = position_step();
    if (ret_val == llog_stat_ok) {
      position_get(&local_pos);
      if (position_callback) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        position_callback(&local_pos);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
      }
    }
    nanosleep(&ts, NULL);
  }

  position_stop();
  return NULL;
}


void position_distance_and_heading(position_t *pos1, position_t *pos2, double *distance, double *heading) {
  double lat1 = pos1->lat * M_PI / 180.0;
  double lon1 = pos1->lon * M_PI / 180.0;
  double lat2 = pos2->lat * M_PI / 180.0;
  double lon2 = pos2->lon * M_PI / 180.0;

  double dlat = lat2 - lat1;
  double dlon = lon2 - lon1;

  double a = sin(dlat / 2) * sin(dlat / 2) +
             cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  if (distance != NULL) {
    *distance = R_EARTH * c; // Earth's radius in meters
  }
  // Calculate heading
  double y = sin(dlon) * cos(lat2);
  double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dlon);

  if (heading != NULL) {
    *heading = atan2(y, x) * 180.0 / M_PI; // Convert to degrees

    // Normalize heading to [0, 360] degrees
    if (*heading < 0) {
      *heading += 360.0;
    }
  }
}

void position_to_qra(position_t *pos, char *qra_locator) {
  if (pos->lat < -90.0 || pos->lat > 90.0 || pos->lon < -180.0 || pos->lon > 180.0) {
    strcpy(qra_locator, "Invalid");
    return;
  }

  double lat = pos->lat + 90.0;
  double lon = pos->lon + 180.0;

  // First pair: Field (A-R)
  int lon_field = (int)(lon / 20.0);
  int lat_field = (int)(lat / 10.0);

  // Second pair: Square (0-9)
  int lon_square = (int)((lon - (lon_field * 20.0)) / 2.0);
  int lat_square = (int)((lat - (lat_field * 10.0)) / 1.0);

  // Third pair: Subsquare (a-x)
  int lon_subsquare = (int)((lon - (lon_field * 20.0) - (lon_square * 2.0)) * 12.0);
  int lat_subsquare = (int)((lat - (lat_field * 10.0) - (lat_square * 1.0)) * 24.0);


  qra_locator[0] = 'A' + lon_field;
  qra_locator[1] = 'A' + lat_field;
  qra_locator[2] = '0' + lon_square;
  qra_locator[3] = '0' + lat_square;
  qra_locator[4] = 'A' + lon_subsquare;
  qra_locator[5] = 'A' + lat_subsquare;
  qra_locator[6] = '\0';
}

void position_stop(void) {
  printf("Stopping GPS\n");
  position_register_callback(NULL);
  if (position_status == position_stat_running) {
    pthread_cancel(thread_id);
    pthread_join(thread_id, NULL);
  }
  gps_stream(&gpsdata, WATCH_DISABLE, NULL);
  gps_close(&gpsdata);
  position_status = position_stat_uninit;
}

static void position_start_thread(void) {
  pthread_create(&thread_id, NULL, position_thread, NULL);
  position_status = position_stat_running;
}

static void position_set(position_t *new_pos) {
  pthread_mutex_lock(&pos_mutex);
  pos = *new_pos;
  pthread_mutex_unlock(&pos_mutex);
}
