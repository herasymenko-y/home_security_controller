#ifndef LIGHT_H
#define LIGHT_H

#include <pthread.h>

void sysfs_write(const char *filename, const char *value);

void* light_thread(void* arg);

struct light_param {
    struct gpio_param *hw;
    struct pwm_param *pwm;
};

#endif // LIGHT_H 


