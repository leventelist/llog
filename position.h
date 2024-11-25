#include "gps.h"
#include "llog.h"

struct
{
    double latitude;
    double longitude;
    double altitude;
} position;

int position_init(char *device);


