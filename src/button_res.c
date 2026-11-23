#include <gpiod.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include "sys_state.h"
#include "hardware_config.h"
#include "button_res.h"
#include "event_queue.h"
#include "logger.h"

void *button_res_thread(void *arg) {
    struct gpio_param *hw = arg;

    if (hw == NULL) {
        if (log_event("ERROR", "Button RES thread: NULL argument") != 0) {
            fprintf(stderr, "Logger write error in button_res_thread\n");
        }
        return NULL;
    }

    struct gpiod_edge_event_buffer *event_buf = gpiod_edge_event_buffer_new(1);
    if (event_buf == NULL) {
        perror("Failed to create edge event buffer");
        if (log_event("ERROR", "Button RES: failed to create edge event buffer") != 0) {
            fprintf(stderr, "Logger write error in button_res_thread\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "Button RES: failed to push EVENT_HW_ERROR\n");
        }
        return NULL;
    }

    while (is_run) {
        int ret = gpiod_line_request_wait_edge_events(hw->button_res, 1000000000);

        if (!is_run) {
            break;
        }

        if (ret < 0) {
            perror("Button RES event wait error");
            if (log_event("ERROR", "Button RES: event wait error") != 0) {
                fprintf(stderr, "Logger write error on RES wait\n");
            }
            if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                fprintf(stderr, "Button RES: failed to push EVENT_HW_ERROR\n");
            }
            break;
        }

        if (ret > 0) {
            int events = gpiod_line_request_read_edge_events(hw->button_res, event_buf, 1);

            if (events < 0) {
                perror("Button RES event read error");
                if (log_event("ERROR", "Button RES: event read error") != 0) {
                    fprintf(stderr, "Logger write error on RES read\n");
                }
                if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                    fprintf(stderr, "Button RES: failed to push EVENT_HW_ERROR\n");
                }
                break;
            }

            if (events > 0) {
                usleep(50000);

                if (event_queue_push(EVENT_BTN_RESET, 0) != 0) {
                    fprintf(stderr, "Button RES: failed to push EVENT_BTN_RESET\n");
                }
            }
        }
    }

    gpiod_edge_event_buffer_free(event_buf);

    return NULL;
}

