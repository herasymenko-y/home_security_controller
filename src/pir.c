#include <pthread.h>
#include <semaphore.h>
#include <gpiod.h>
#include <stdio.h>
#include <stddef.h>

#include "sys_state.h"
#include "hardware_config.h"
#include "pir.h"

void *pir_thread(void *arg) {
    struct gpio_param *hw = arg;

    if (hw == NULL || hw->pir == NULL) {
        return NULL;
    }

    int pir_active = 0;
    int motion_handled = 0;

    struct gpiod_edge_event_buffer *event_buf =
        gpiod_edge_event_buffer_new(1);

    if (event_buf == NULL) {
        perror("Failed to create PIR event buffer");
        return NULL;
    }

    while (is_run) {
        int ret = gpiod_line_request_wait_edge_events(
            hw->pir, 50000000);

        if (!is_run) {
            break;
        }

        if (ret < 0) {
            perror("PIR: failed to wait for edge events");
            break;
        }

        if (ret > 0) {
            int events = gpiod_line_request_read_edge_events(
                hw->pir, event_buf, 1);

            if (events < 0) {
                perror("PIR: failed to read edge events");
                break;
            }

            if (events > 0) {
                struct gpiod_edge_event *event =
                    gpiod_edge_event_buffer_get_event(event_buf, 0);

                int event_type =
                    gpiod_edge_event_get_event_type(event);

                if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE) {
                    pir_active = 1;
                    motion_handled = 0;
                }
                else if (event_type == GPIOD_EDGE_EVENT_FALLING_EDGE) {
                    pir_active = 0;
                }
            }
        }

        pthread_mutex_lock(&sys_mut);

        if (pir_active &&
            curr_state == STATE_IDLE &&
            !motion_handled) {

            printf("[PIR] MOTION DETECTED! Start scanning...\n");

            sem_post(&sem_rfid);
            motion_handled = 1;
        }

        pthread_mutex_unlock(&sys_mut);
    }

    gpiod_edge_event_buffer_free(event_buf);

    return NULL;
}
