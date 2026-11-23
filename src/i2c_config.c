#include "i2c_config.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


int i2c_init(void) {

    int fd = open(I2C_PATH, O_RDWR | O_CLOEXEC);

    if (fd < 0) {
        return -1;
    }

    return fd;
}


int i2c_transfer(int fd, struct i2c_msg *msgs, uint32_t num) {

    if (fd < 0 || msgs == NULL || num == 0) {
        return -1;
    }

    struct i2c_rdwr_ioctl_data transfer = {
        .msgs = msgs,
        .nmsgs = num
    };

    if (ioctl(fd, I2C_RDWR, &transfer) != (int)num) {
        return -1;
    }

    return 0;
}


int i2c_write_reg(int fd, uint8_t addr, uint8_t reg, uint8_t data) {

    uint8_t tx[2] = {reg, data};

    struct i2c_msg msg = {
        .addr = addr,
        .flags = 0,
        .len = 2,
        .buf = tx
    };

    return i2c_transfer(fd, &msg, 1);
}


int i2c_read_regs(int fd, uint8_t addr, uint8_t reg,
                  uint8_t *buf, uint16_t len) {

    if (buf == NULL || len == 0) {
        return -1;
    }

    struct i2c_msg msgs[2] = {
        {
            .addr = addr,
            .flags = 0,
            .len = 1,
            .buf = &reg
        },
        {
            .addr = addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = buf
        }
    };

    return i2c_transfer(fd, msgs, 2);
}
