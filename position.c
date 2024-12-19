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

#include "position.h"
#include "llog.h"


typedef enum {
  position_stat_uninit,
  position_stat_init,
  position_stat_running
} position_status_t;

/*Module variables*/
static pthread_t thread_id;
static position_status_t position_status = position_stat_uninit;
static gps_data_real_t gpsdata;

static position_t pos;
static pthread_mutex_t pos_mutex = PTHREAD_MUTEX_INITIALIZER;


/*Module functions*/
static void position_start_thread(void);
static void position_set(position_t *new_pos);

/*Interface functions*/
int position_init(char *host, uint64_t port) {
  char port_str[16];
  int ret;
  int ret_val;


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
  //int ret;
  int ret_val;

  while (1) {
    position_status = position_stat_running;
    ret_val = position_step();
    if (ret_val == llog_stat_ok) {
      position_get(&local_pos);
      printf("Lat: %f, Lon: %f, Alt: %f, Speed: %f\n", local_pos.lat, local_pos.lon, local_pos.alt, local_pos.speed);
    }
  }

  position_stop();
  return NULL;
}


double position_distance(position_t *pos1, position_t *pos2) {
  double lat1 = pos1->lat * M_PI / 180.0;
  double lon1 = pos1->lon * M_PI / 180.0;
  double lat2 = pos2->lat * M_PI / 180.0;
  double lon2 = pos2->lon * M_PI / 180.0;

  double dlat = lat2 - lat1;
  double dlon = lon2 - lon1;

  double a = sin(dlat / 2) * sin(dlat / 2) +
             cos(lat1) * cos(lat2) * sin(dlon / 2) * sin(dlon / 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  double distance = 6371e3 * c; // Earth's radius in meters

  return distance;
}


void position_stop(void) {
  pthread_cancel(thread_id);
  pthread_join(thread_id, NULL);
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
