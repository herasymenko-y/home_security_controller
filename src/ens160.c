#include <stdint.h>
#include <unistd.h>
#include "ens160.h"
#include "i2c_config.h"

int ens160_read_aqi(int fd, uint8_t *aqi) {
    if (aqi == NULL) return -1;

    uint8_t status;

    if (i2c_read_regs(fd, ENS160_ADDR, 0x21, &status, 1) != 0) {
        return -1;
    }

    *aqi = status & 0x07;

    return 0;
}

int ens160_read_tvoc(int fd, uint16_t *tvoc) {
    if (tvoc == NULL) return -1;

    uint8_t tvoc_data[2];

    if (i2c_read_regs(fd, ENS160_ADDR, 0x22, tvoc_data, 2) != 0) {
        return -1;
    }

    *tvoc = ((uint16_t)tvoc_data[1] << 8) | tvoc_data[0];

    return 0;
}

int ens160_read_eco2(int fd, uint16_t *eco2) {
    if (eco2 == NULL) return -1;

    uint8_t eco2_data[2];

    if (i2c_read_regs(fd, ENS160_ADDR, 0x24, eco2_data, 2) != 0) {
        return -1;
    }

    *eco2 = ((uint16_t)eco2_data[1] << 8) | eco2_data[0];

    return 0;
}

int ens160_set_standard_mode(int fd) {
    return i2c_write_reg(fd, ENS160_ADDR, 0x10, 0x02);
}

int ens160_data_ready(int fd, uint8_t *ready) {
    if (ready == NULL) return -1;

    uint8_t status;

    if (i2c_read_regs(fd, ENS160_ADDR, 0x20, &status, 1) != 0) {
        return -1;
    }

    *ready = (status & 0x02) ? 1 : 0;

    return 0;
}

int ens160_wait_data(int fd) {
    uint8_t ready = 0;

    for (int i = 0; i < 50; i++) {
        if (ens160_data_ready(fd, &ready) != 0) {
            return -1;
        }

        if (ready == 1) {
            return 0;
        }

        usleep(100000);
    }

    return -1;
}
