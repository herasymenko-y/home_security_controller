#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include "spi_config.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int spi_init(void) {
int fd = open(SPI_PATH, O_RDWR | O_CLOEXEC);
 if (fd < 0) {
 return -1;
}
            uint8_t mode = SPI_MODE;
            uint8_t bits = SPI_BITS;
            uint32_t speed = SPI_SPEED;

            if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0 ||
                ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
                ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
                close(fd);
return -1;
}
uint8_t check_mode;
uint8_t check_bits;
uint32_t check_speed;
 if (ioctl(fd, SPI_IOC_RD_MODE, &check_mode) < 0 ||
                ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &check_bits) < 0 ||
                ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &check_speed) < 0) {
                close(fd);
return -1;
}
if (check_mode != SPI_MODE ||
    check_bits != SPI_BITS ||
    check_speed != SPI_SPEED) {
    close(fd);
    return -1;
}
return fd;
}

int spi_write_reg(int fd, uint8_t addr, uint8_t data)
{
if (fd < 0 || addr > SPI_MAX_REG_ADDR){
return -1;
}
    uint8_t tx[2];
    uint8_t rx[2];
    struct spi_ioc_transfer tr;

    tx[0] = (addr << 1) & 0x7E;
    tx[1] = data;

    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = SPI_LEN;
    tr.speed_hz = SPI_SPEED;
    tr.bits_per_word = SPI_BITS;

    if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) != SPI_LEN){
return -1;
}
    return 0;
}

int spi_read_reg(int fd, uint8_t addr, uint8_t *val)
{
    if (fd < 0 || val == NULL || addr > SPI_MAX_REG_ADDR) {
    return -1;
}
    uint8_t tx[2];
    uint8_t rx[2];
    struct spi_ioc_transfer tr;

    tx[0] = ((addr << 1) & 0x7E) | 0x80;
    tx[1] = 0x00;

    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
   tr.len = SPI_LEN;
    tr.speed_hz = SPI_SPEED;
    tr.bits_per_word = SPI_BITS;

    if(ioctl(fd, SPI_IOC_MESSAGE(1), &tr) != SPI_LEN){
return -1;
}

*val = rx[1];
return 0;
}

int set_reg_bit(int fd, uint8_t addr, uint8_t bit)
{
if (fd < 0 || bit > 7 || addr > SPI_MAX_REG_ADDR){
return -1;
}
uint8_t value; 
if (spi_read_reg(fd, addr, &value) < 0){
return -1;
}

    value |= (1U << bit);
    if (spi_write_reg(fd, addr, value) < 0){
return -1;
}
    return 0;
}

int read_reg_bit(int fd, uint8_t addr, uint8_t bit)
{
if (fd < 0 || bit > 7 || addr > SPI_MAX_REG_ADDR){
return -1;
}
    uint8_t value;
    if (spi_read_reg(fd, addr, &value) < 0){
return -1;
}

    if (value & (1U << bit)) {
        return 1;
    }
    else {
        return 0;
    }
}




