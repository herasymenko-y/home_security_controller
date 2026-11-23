#include "aht21.h"
#include "i2c_config.h"
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>


int aht21_init(int fd) {

    if (fd < 0) {
        return -1;
    }

    usleep(100000);

    uint8_t cmd = 0x71;
    uint8_t status;

    struct i2c_msg msgs[2] = {
        {
            .addr = AHT21_ADDR,
            .flags = 0,
            .len = 1,
            .buf = &cmd
        },
        {
            .addr = AHT21_ADDR,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = &status
        }
    };

    if (i2c_transfer(fd, msgs, 2) < 0) {
        return -1;
    }

    if (!(status & 0x08)) {

        uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};

        struct i2c_msg msg = {
            .addr = AHT21_ADDR,
            .flags = 0,
            .len = 3,
            .buf = init_cmd
        };

        if (i2c_transfer(fd, &msg, 1) < 0) {
            return -1;
        }

        usleep(10000);
    }

    return 0;
}


int aht21_measure(int fd, double *humidity, double *temperature) {

    if (fd < 0 || humidity == NULL || temperature == NULL) {
        return -1;
    }

    uint8_t cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];

    struct i2c_msg msg = {
        .addr = AHT21_ADDR,
        .flags = 0,
        .len = 3,
        .buf = cmd
    };

    if (i2c_transfer(fd, &msg, 1) < 0) {
        return -1;
    }

    usleep(80000);


    uint8_t status_cmd = 0x71;
    uint8_t status;

    struct i2c_msg status_msgs[2] = {
        {
            .addr = AHT21_ADDR,
            .flags = 0,
            .len = 1,
            .buf = &status_cmd
        },
        {
            .addr = AHT21_ADDR,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = &status
        }
    };

    int ready = 0;

    for (int i = 0; i < 10; i++) {

        if (i2c_transfer(fd, status_msgs, 2) < 0) {
            return -1;
        }

        if (!(status & 0x80)) {
            ready = 1;
            break;
        }

        usleep(10000);
    }

    if (!ready) {
        return -1;
    }


    struct i2c_msg read_msg = {
        .addr = AHT21_ADDR,
        .flags = I2C_M_RD,
        .len = 6,
        .buf = data
    };

    if (i2c_transfer(fd, &read_msg, 1) < 0) {
        return -1;
    }

    if (data[0] & 0x80) {
        return -1;
    }


    uint32_t raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        (data[3] >> 4);

    uint32_t raw_temp =
        (((uint32_t)data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];


    *humidity = raw_humidity * 100.0 / 1048576.0;

    *temperature = raw_temp * 200.0 / 1048576.0 - 50.0;

    return 0;
}
