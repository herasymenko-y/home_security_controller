
#include <gpiod.h>
#include <pthread.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

#include "sys_state.h"
#include "hardware_config.h"
#include "button_res.h"

void* button_res_thread(void* arg) {

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
            hw->button_res, 1000000000);

        if (!is_run) {
            break;
        }

        if (ret < 0) {
            perror("Button RES event wait error");
            break;
        }

        if (ret > 0) {

            int events = gpiod_line_request_read_edge_events(
                hw->button_res, event_buf, 1);

            if (events < 0) {
                perror("Button RES event read error");
                break;
            }

            if (events > 0) {

                usleep(50000);

                pthread_mutex_lock(&sys_mut);

                curr_state = STATE_IDLE;

                printf("System sleep\n");

                pthread_cond_broadcast(&sys_sig);

                pthread_mutex_unlock(&sys_mut);
            }
        }
    }

    gpiod_edge_event_buffer_free(event_buf);

    return NULL;
}

