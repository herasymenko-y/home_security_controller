
#include <gpiod.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "sys_state.h"
#include "hardware_config.h"
#include "button_pow_off.h"

void* button_pow_off_thread(void* arg) {

    struct gpio_param *hw = arg;

    if (hw == NULL) {
        return NULL;
    }

    struct gpiod_edge_event_buffer *event_buf =
        gpiod_edge_event_buffer_new(1);

    if (event_buf == NULL) {
        perror("Failed to create edge event buffer");
        return NULL;
    }

    while (is_run) {

        int ret = gpiod_line_request_wait_edge_events(
            hw->button_pow_off, 1000000000);

        if (!is_run) {
            break;
        }

        if (ret < 0) {
            perror("Button POW_OFF event wait error");
            break;
        }

        if (ret > 0) {

            int events = gpiod_line_request_read_edge_events(
                hw->button_pow_off, event_buf, 1);

            if (events < 0) {
                perror("Button POW_OFF event read error");
                break;
            }

            if (events > 0) {

                usleep(50000);

                pthread_mutex_lock(&sys_mut);

                is_run = 0;

                printf("System power off\n");

                pthread_cond_broadcast(&sys_sig);

                sem_post(&sem_rfid);

                pthread_mutex_unlock(&sys_mut);

                gpiod_edge_event_buffer_free(event_buf);

                sync();

                system("poweroff");

                return NULL;
            }
        }
    }

    gpiod_edge_event_buffer_free(event_buf);

    return NULL;
}

