#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "sys_state.h"
#include "atmosphere.h"
#include "i2c_config.h"
#include "ens160.h"
#include "aht21.h"
#include "environment_check.h"


void *atmosphere_thread(void *arg) {

    if (arg == NULL) {
        return NULL;
    }

    int fd = *(int *)arg;

    if (fd < 0) {
        return NULL;
    }


    if (ens160_set_standard_mode(fd) < 0) {
        printf("Error initializing ENS160\n");
    }

    if (aht21_init(fd) < 0) {
        printf("Error initializing AHT21\n");
    }

    ens160_wait_data(fd);


    while (is_run) {

        int danger = environment_check_danger(fd);

        if (!is_run) {
            break;
        }

        if (danger) {

            pthread_mutex_lock(&sys_mut);

            curr_state = STATE_ALARM;

            pthread_cond_broadcast(&sys_sig);

            pthread_mutex_unlock(&sys_mut);
        }

        sleep(2);
    }

    return NULL;
}
