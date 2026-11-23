#include "environment_check.h"
#include "ens160.h"
#include "aht21.h"
#include <stdint.h>


int environment_check_danger(int fd) {

    int danger = 0;

    uint8_t aqi;
    uint16_t eco2, tvoc;

    double temperature, humidity;

    if (ens160_wait_data(fd) == 0) {

        if (ens160_read_aqi(fd, &aqi) == 0 &&
            aqi >= DANGER_AQI_LEVEL) {
            danger = 1;
        }

        if (ens160_read_eco2(fd, &eco2) == 0 &&
            eco2 >= DANGER_ECO2_LEVEL) {
            danger = 1;
        }

        if (ens160_read_tvoc(fd, &tvoc) == 0 &&
            tvoc >= DANGER_TVOC_LEVEL) {
            danger = 1;
        }
    }

    if (aht21_measure(fd, &humidity, &temperature) == 0) {

        if (temperature >= DANGER_TEMP_HIGH ||
            temperature <= DANGER_TEMP_LOW) {
            danger = 1;
        }
    }

    return danger;
}
