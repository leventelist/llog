#include "position.h"
#include <gps.h>
#include "llog.h"

int position_init(char *host, uint64_t port, gps_data_real_t *gpsdata) {
  char port_str[16];
  int ret;
  int ret_val;


  snprintf(port_str, sizeof(port_str), "%lu", port);

  ret = gps_open(host, port_str, gpsdata);

  switch (ret) {
  case 0:
    ret_val = OK;
    break;

  case -1:
    ret_val = ERR;
    break;

  default:
    ret_val = ERR;
    break;
  }

  gps_stream(gpsdata, WATCH_ENABLE | WATCH_JSON, NULL);

  return ret_val;
}


int position_step(gps_data_real_t *gpsdata, position_t *pos) {
  bool data_ready;
  int ret;
  int ret_val;
  bool progress = false;

  data_ready = gps_waiting(gpsdata, POSITION_TIMEOUT_USEC);

  if (data_ready) {
    ret = gps_read(gpsdata, NULL, 0);
    switch (ret) {
    case 0:
      if ((gpsdata->set & MODE_SET) != MODE_SET) {
        ret_val = NO_DATA;
        break;
      }
      switch (gpsdata->fix.mode) {
      case MODE_NOT_SEEN:
        progress = false;
        break;

      case MODE_NO_FIX:
        progress = false;
        break;

      case MODE_2D:
        progress = true;
        break;

      case MODE_3D:
        progress = true;
        break;

      default:
        progress = false;
        break;
      }
      if (!progress) {
        ret_val = NO_DATA;
        break;
      } else {
        ret_val = OK;
        pos->lat = gpsdata->fix.latitude;
        pos->lon = gpsdata->fix.longitude;
        pos->alt = gpsdata->fix.altitude;
        pos->speed = gpsdata->fix.speed;
      }

      ret_val = OK;
      break;

    case -1:
      ret_val = ERR;
      break;

    default:
      break;
    }
  } else {
    ret_val = NO_DATA;
  }


  return ret_val;
}

int position_close(gps_data_real_t *gpsdata) {
  int ret;
  int ret_val;

  gps_stream(gpsdata, WATCH_DISABLE, NULL);

  ret = gps_close(gpsdata);

  switch (ret) {
  case 0:
    ret_val = OK;
    break;

  case -1:
    ret_val = ERR;
    break;

  default:
    ret_val = ERR;
    break;
  }

  return ret_val;
}