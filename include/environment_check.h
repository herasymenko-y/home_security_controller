#ifndef ENVIRONMENT_CHECK_H
#define ENVIRONMENT_CHECK_H

#include <stdint.h>


#define DANGER_AQI_LEVEL       4
#define DANGER_ECO2_LEVEL      1500
#define DANGER_TVOC_LEVEL      1000
#define DANGER_TEMP_HIGH       70.0
#define DANGER_TEMP_LOW       -10.0


int environment_check_danger(int fd);

#endif // ENVIRONMENT_CHECK_H
