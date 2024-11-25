#include "position.h"
#include "gps.h"
#include "llog.h"


int position_init(char *host, uint64_t port, gps_data_t *gpsdata)
{
  char port_str[16];
  int ret;
  int ret_val;


  snprintf(port_str, sizeof(port_str), "%lu", port);

  ret = gps_open(host, port_str, gpsdata);

  switch (ret)
  {
  case 0:
    ret_val = OK
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


int position_get_status(void)
{
    return gps_get_status();
}