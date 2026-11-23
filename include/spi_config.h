#ifndef SPI_CONFIG_H
#define SPI_CONFIG_H

#include <linux/spi/spidev.h>
#include <stdint.h>

#define SPI_PATH "/dev/spidev0.0"
#define SPI_MODE SPI_MODE_0
#define SPI_BITS 8
#define SPI_SPEED 1000000
#define SPI_LEN 2
#define SPI_MAX_REG_ADDR 0x3F

int spi_init(void);

int spi_write_reg(int fd, uint8_t addr, uint8_t data);
int spi_read_reg(int fd, uint8_t addr, uint8_t *val);

int set_reg_bit(int fd, uint8_t addr, uint8_t bit);
int read_reg_bit(int fd, uint8_t addr, uint8_t bit);

#endif // SPI_CONFIG_H

