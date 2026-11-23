#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include <stdint.h>
#include <linux/i2c.h>

#define I2C_PATH "/dev/i2c-1"

int i2c_init(void);

int i2c_transfer(int fd, struct i2c_msg *msgs, uint32_t num);

int i2c_write_reg(int fd, uint8_t addr, uint8_t reg, uint8_t data);

int i2c_read_regs(int fd, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

#endif // I2C_CONFIG_H
