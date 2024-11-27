#include <gps.h>
#include "llog.h"


#define POSITION_TIMEOUT_USEC 500000

typedef struct gps_data_t gps_data_real_t;

typedef struct {
    double lat;
    double lon;
    double alt;
    double speed;
    double climb;
    double track;
} position_t;

int position_init(char *host, uint64_t port, gps_data_real_t *gpsdata);
int position_step(gps_data_real_t *gpsdata, position_t *pos);
int position_close(gps_data_real_t *gpsdata);
