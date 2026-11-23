#include <pthread.h>
#include <gpiod.h>
#include <stddef.h>
#include <stdio.h>
#include "sys_state.h"
#include "buzzer.h"
#include "hardware_config.h"
#include "event_queue.h"
#include "logger.h"

void *buzzer_thread(void *arg) {
    struct gpio_param *hw = arg;

    if (hw == NULL) {
        if (log_event("ERROR", "Buzzer thread: NULL argument") != 0) {
            fprintf(stderr, "Logger write error in buzzer_thread\n");
        }
        return NULL;
    }

    int state = -1;

    while (is_run) {
        pthread_mutex_lock(&sys_mut);
        while ((state == curr_state) && is_run) {
            pthread_cond_wait(&sys_sig, &sys_mut);
        }
        state = curr_state;
        pthread_mutex_unlock(&sys_mut);

        if (!is_run) {
            break;
        }

        if (state == STATE_ALARM) {
            if (gpiod_line_request_set_value(
                    hw->buzzer, GPIO_BUZZER, GPIOD_LINE_VALUE_ACTIVE) < 0) {
                perror("Buzzer GPIO ON error");
                if (log_event("ERROR", "Buzzer: failed to turn ON") != 0) {
                    fprintf(stderr, "Logger write error on buzzer ON\n");
                }
                if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                    fprintf(stderr, "Buzzer: failed to push EVENT_HW_ERROR\n");
                }
            }
        } else {
            if (gpiod_line_request_set_value(
                    hw->buzzer, GPIO_BUZZER, GPIOD_LINE_VALUE_INACTIVE) < 0) {
                perror("Buzzer GPIO OFF error");
                if (log_event("ERROR", "Buzzer: failed to turn OFF") != 0) {
                    fprintf(stderr, "Logger write error on buzzer OFF\n");
                }
                if (state != STATE_ERROR) {
                    if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
                        fprintf(stderr, "Buzzer: failed to push EVENT_HW_ERROR\n");
                    }
                }
            }
        }
    }

    gpiod_line_request_set_value(hw->buzzer, GPIO_BUZZER, GPIOD_LINE_VALUE_INACTIVE);

    return NULL;
}
