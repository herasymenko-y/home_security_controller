#ifndef AHT21_H
#define AHT21_H

#define AHT21_ADDR 0x38

int aht21_init(int fd);

int aht21_measure(int fd, double *humidity, double *temperature);

#endif // AHT21_H
