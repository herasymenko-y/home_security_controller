#ifndef MFRC522_H
#define MFRC522_H

#include <stdint.h>

#define COMMANDREG 0x01
#define BITFRAMINGREG 0x0D
#define TMODEREG 0x2A
#define TPRESCALERREG 0x2B
#define TRELOADREG_H 0x2C
#define TRELOADREG_L 0x2D
#define TXCONTROLREG 0x14
#define TXASKREG 0x15
#define FIFOLEVELREG 0x0A
#define FIFODATAREG 0x09
#define VERSIONREG 0x37
#define ERRORREG   0x06
#define COMIRQREG  0x04

int mfrc522_init(int fd);

int request_card(int fd, uint8_t atqa[]);

int anticollision(int fd, uint8_t uid[]);

int mfrc522_read_card(int fd, uint8_t uid[]);

#endif // MFRC522_H
