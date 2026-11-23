#include <gpiod.h>
#include <stdio.h>
#include <stddef.h>
#include "sys_state.h"
#include "hardware_config.h"
#include "pir.h"
#include "event_queue.h"
#include "logger.h"

void *pir_thread(void *arg) {
    struct gpio_param *hw = arg;

    if (hw == NULL || hw->pir == NULL) {
        if (log_event("ERROR", "PIR thread: NULL argument") != 0) {
            fprintf(stderr, "Logger write error in pir_thread\n");
        }
        return NULL;
    }

    struct gpiod_edge_event_buffer *event_buf = gpiod_edge_event_buffer_new(1);
    if (event_buf == NULL) {
        perror("Failed to create PIR event buffer");
        if (log_event("ERROR", "PIR: failed to create edge event buffer") != 0) {
            fprintf(stderr, "Logger write error in pir_thread\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "PIR: failed to push EVENT_HW_ERROR\n");
        }
        return NULL;
    }

    while (is_run) {
        int ret = gpiod_line_request_wait_edge_events(hw->pir, 1000000000);

        if (!is_run) {
            break;
        }

        if (ret < 0) {
            perror("PIR: failed to wait for edge events");
            if (log_event("ERROR", "PIR: event wait error") != 0) {
                fprintf(stderr, "Logger write error on PIR wait\n");
            }
            if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                fprintf(stderr, "PIR: failed to push EVENT_HW_ERROR\n");
            }
            break;
        }

        if (ret > 0) {
            int events = gpiod_line_request_read_edge_events(hw->pir, event_buf, 1);

            if (events < 0) {
                perror("PIR: failed to read edge events");
                if (log_event("ERROR", "PIR: event read error") != 0) {
                    fprintf(stderr, "Logger write error on PIR read\n");
                }
                if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                    fprintf(stderr, "PIR: failed to push EVENT_HW_ERROR\n");
                }
                break;
            }

            if (events > 0) {
                struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(event_buf, 0);
                int event_type = gpiod_edge_event_get_event_type(event);

                if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE) {
                    if (event_queue_push(EVENT_MOTION, 0) != 0) {
                        fprintf(stderr, "PIR: failed to push EVENT_MOTION\n");
                    }
                }
            }
        }
    }

    gpiod_edge_event_buffer_free(event_buf);

    return NULL;
}
