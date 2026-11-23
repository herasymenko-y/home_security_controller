#ifndef ENS160_H
#define ENS160_H
#include <stdint.h>

#define ENS160_ADDR 0x53

int ens160_read_aqi(int fd, uint8_t *aqi);
int ens160_read_tvoc(int fd, uint16_t *tvoc);
int ens160_read_eco2(int fd, uint16_t *eco2);

int ens160_set_standard_mode(int fd);
int ens160_data_ready(int fd, uint8_t *ready);
int ens160_wait_data(int fd);

#endif // ENS160_H
