#include <stddef.h>
#include "spi_config.h"
#include "mfrc522.h"
#include <stdint.h>
#include <unistd.h>


int mfrc522_init(int fd)
{
if (fd < 0){
return -1;
}
    if (spi_write_reg(fd, COMMANDREG, 0x0F) < 0){
return -1;
}
uint8_t command;
for (int i = 0; i < 3; i++) {
    usleep(50000);

    if (spi_read_reg(fd, COMMANDREG, &command) < 0) {
        return -1;
    }

    if (!(command & (1U << 4))) {
        break;
    }

    if (i == 2) {
        return -1;
    }
}

uint8_t version;

if (spi_read_reg(fd, VERSIONREG, &version) < 0) {
    return -1;
}

if (version == 0x00 || version == 0xFF) {
    return -1;
}
    if (spi_write_reg(fd, TMODEREG, 0x8D) < 0){
return -1;
}
    if (spi_write_reg(fd, TPRESCALERREG, 0x3E) < 0){
    return -1;
    }
    
    if (spi_write_reg(fd, TRELOADREG_H, 0x00) < 0){
return -1;
}
    if (spi_write_reg(fd, TRELOADREG_L, 0x1E) < 0) {
    return -1;
    }

    if (spi_write_reg(fd, TXASKREG, 0x40) < 0) {
    return -1;
   }

    if (set_reg_bit(fd, TXCONTROLREG, 0) < 0){
    return -1;
    }
    if (set_reg_bit(fd, TXCONTROLREG, 1) < 0){
return -1;
}
    return 0;
}


static int fifo_flush(int fd)
{
    if (fd < 0) {
        return -1;
    }

    if (spi_write_reg(fd, FIFOLEVELREG, 0x80) < 0) {
        return -1;
    }

    return 0;
}

static int fifo_write(int fd, const uint8_t data[], uint8_t len)
{
if ( fd < 0 || data == NULL || len > 64){
return -1;
}
   if (fifo_flush(fd) < 0) {
    return -1;
}

    for (int i = 0; i < len; i++) {
        if (spi_write_reg(fd, FIFODATAREG, data[i]) < 0){
        fifo_flush(fd);
return -1;
}
    }
    
    uint8_t count;

if (spi_read_reg(fd, FIFOLEVELREG, &count) < 0) {
fifo_flush(fd);
    return -1;
}

count &= 0x7F;

if (count != len) {
fifo_flush(fd);
    return -1;
}
    return 0;
}

static int fifo_read(int fd, uint8_t data[], uint8_t capacity)
{
if (fd < 0 || data == NULL){
return -1;
}

  uint8_t val;

if (spi_read_reg(fd, FIFOLEVELREG, &val) < 0) {
    return -1;
}
    val = val & 0x7F;
if (val > capacity){
fifo_flush(fd);
return -1;
}
    for (int i = 0; i < val; i++) {
        if (spi_read_reg(fd, FIFODATAREG, &data[i]) < 0){
        fifo_flush(fd);
        return -1;
        }
    }
return val;
}

int request_card(int fd, uint8_t atqa[])
{
if (fd < 0 || atqa == NULL){
return -1;
}
    uint8_t irq;
    uint8_t error;
    uint8_t count;
    int timeout = 1000;

    if (spi_write_reg(fd, COMMANDREG, 0x00) < 0){
return -1;
}
    if (spi_write_reg(fd, FIFOLEVELREG, 0x80) < 0){
    fifo_flush(fd);
return -1;
}
    if (spi_write_reg(fd, COMIRQREG, 0x7F) < 0){
    return -1;
    }

    if (spi_write_reg(fd, FIFODATAREG, 0x26) < 0){
    fifo_flush(fd);
return -1;
}
    if (spi_write_reg(fd, BITFRAMINGREG, 0x07) < 0){
return -1; 
}
    if (spi_write_reg(fd, COMMANDREG, 0x0C) < 0){
    fifo_flush(fd);
    return -1;
}
    if (set_reg_bit(fd, BITFRAMINGREG, 7) < 0){
    return -1;
    }
    while (timeout > 0) {
        
      if (spi_read_reg(fd, COMIRQREG, &irq) < 0){
      return -1;
      }

        if (irq & 0x21) {
            break;
        }

        usleep(100);
        timeout--;
    }

    if (spi_write_reg(fd, COMMANDREG, 0x00) < 0){
return -1;
}

    if (timeout == 0 || (irq & 0x20) == 0) {
        return 0;
    }

    if (spi_read_reg(fd, ERRORREG, &error) < 0){
return -1;
}

    if (error & 0x1B) {
    fifo_flush(fd);
        return 0;
    }

  if (spi_read_reg(fd, FIFOLEVELREG, &count) < 0){
  fifo_flush(fd);
return -1;
}
  count &= 0x7F;

    if (count != 2) {
    fifo_flush(fd);
        return 0;
    }

    if (fifo_read(fd, atqa, 2) != 2) {
        return -1;
    }

    return 1;
}


int anticollision(int fd, uint8_t uid[]){

if (fd < 0 || uid == NULL){
return -1;
} 
    int timeout = 1000;
    uint8_t val;
uint8_t command[2] = {0x93, 0x20};

if (spi_write_reg(fd, COMMANDREG, 0x00) < 0){
return -1;
}
if (spi_write_reg(fd, COMIRQREG, 0x7F) < 0){
return -1;
}

if (fifo_write (fd, command, 2) < 0){
return -1;
}

if (spi_write_reg(fd, BITFRAMINGREG, 0x00) < 0){
return -1;
}

if (spi_write_reg(fd, COMMANDREG, 0x0C) < 0){
return -1;
}

if (set_reg_bit(fd, BITFRAMINGREG, 7) < 0){
return -1;
}

uint8_t irq = 0;

while (timeout > 0) {
    if (spi_read_reg(fd, COMIRQREG, &irq) < 0) {
        return -1;
    }

    if (irq & 0x21) {
        break;
    }

    usleep(100);
    timeout--;
}
if (spi_write_reg(fd, COMMANDREG, 0x00) < 0) {
    return -1;
}

if (timeout == 0 || !(irq & 0x20)) {
if (fifo_flush(fd) < 0){
return -1;
}
    return 0;
}

if (spi_read_reg (fd, ERRORREG, &val) < 0){
return -1;
}

if (val & 0x1B){
if (fifo_flush(fd) < 0){
return -1;
}
return 0;
}

if (spi_read_reg(fd, FIFOLEVELREG, &val) < 0){
return -1; 
}

val &= 0x7F;

if (val != 5){
    if (fifo_flush(fd) < 0) {
        return -1;
    }
return 0;
}

int result;
result = fifo_read(fd, uid, 5);

if (result < 0){
return -1;
} else if (result != 5){
return 0;
}

uint8_t bcc = uid[0] ^ uid[1] ^ uid[2] ^ uid[3];

if (bcc != uid[4]){
if (fifo_flush(fd) < 0){
return -1;
}
return 0;
}
return 1;
}

int mfrc522_read_card(int fd, uint8_t uid[])
{
    if (fd < 0 || uid == NULL) {
        return -1;
    }

    uint8_t atqa[2];

    int result = request_card(fd, atqa);

    if (result <= 0) {
        return result;
    }

    return anticollision(fd, uid);
}


