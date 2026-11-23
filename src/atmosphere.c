#include <stdio.h>
#include <unistd.h>
#include "sys_state.h"
#include "atmosphere.h"
#include "i2c_config.h"
#include "ens160.h"
#include "aht21.h"
#include "environment_check.h"
#include "event_queue.h"
#include "logger.h"

void *atmosphere_thread(void *arg) {
    if (arg == NULL) {
        if (log_event("ERROR", "Atmosphere thread: NULL argument") != 0) {
            fprintf(stderr, "Logger write error in atmosphere_thread\n");
        }
        return NULL;
    }

    int fd = *(int *)arg;
    if (fd < 0) {
        if (log_event("ERROR", "Atmosphere thread: invalid I2C fd") != 0) {
            fprintf(stderr, "Logger write error in atmosphere_thread\n");
        }
        return NULL;
    }

    if (ens160_set_standard_mode(fd) < 0) {
        fprintf(stderr, "Error initializing ENS160\n");
        if (log_event("ERROR", "Failed to initialize ENS160") != 0) {
            fprintf(stderr, "Logger write error on ENS160 init\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "Atmosphere: failed to push EVENT_HW_ERROR\n");
        }
    }

    if (aht21_init(fd) < 0) {
        fprintf(stderr, "Error initializing AHT21\n");
        if (log_event("ERROR", "Failed to initialize AHT21") != 0) {
            fprintf(stderr, "Logger write error on AHT21 init\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "Atmosphere: failed to push EVENT_HW_ERROR\n");
        }
    }

    ens160_wait_data(fd);

    while (is_run) {
        int danger = environment_check_danger(fd);

        if (!is_run) {
            break;
        }

        if (danger) {
            if (event_queue_push(EVENT_ENV_ALERT, danger) != 0) {
                fprintf(stderr, "Atmosphere: failed to push EVENT_ENV_ALERT\n");
            }
        }

        sleep(2);
    }

    return NULL;
}
