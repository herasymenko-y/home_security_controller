#include <pthread.h>
#include <gpiod.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "sys_state.h"
#include "light.h"
#include "pwm_config.h"
#include "hardware_config.h"
#include "event_queue.h"
#include "logger.h"

static void report_light_error(int state, const char *msg) {
    perror(msg);
    if (log_event("ERROR", msg) != 0) {
        fprintf(stderr, "Logger write error in light_thread\n");
    }
    if (state != STATE_ERROR) {
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "Light: failed to push EVENT_HW_ERROR\n");
        }
    }
}

void *light_thread(void *arg) {
    struct light_param *param = arg;

    if (param == NULL || param->hw == NULL || param->pwm == NULL) {
        if (log_event("ERROR", "Light thread: NULL argument") != 0) {
            fprintf(stderr, "Logger write error in light_thread\n");
        }
        return NULL;
    }

    struct gpio_param *hw = param->hw;
    struct pwm_param *pwm = param->pwm;

    if (hw->yellow_led == NULL || hw->green_led == NULL ||
        pwm->enable_fd < 0 || pwm->period_fd < 0 ||
        pwm->duty_cycle_fd < 0) {
        if (log_event("ERROR", "Light thread: invalid GPIO or PWM descriptors") != 0) {
            fprintf(stderr, "Logger write error in light_thread\n");
        }
        if (event_queue_push(EVENT_HW_ERROR, 0) != 0) {
            fprintf(stderr, "Light: failed to push EVENT_HW_ERROR\n");
        }
        return NULL;
    }

    int local_state = -1;

    while (is_run) {
        pthread_mutex_lock(&sys_mut);
        while ((local_state == curr_state) && is_run) {
            pthread_cond_wait(&sys_sig, &sys_mut);
        }
        local_state = curr_state;
        pthread_mutex_unlock(&sys_mut);

        if (!is_run) {
            break;
        }

        if (local_state == STATE_IDLE) {
            if (pwm_write(pwm, "enable", "0") < 0)
                report_light_error(local_state, "Light: PWM disable error in IDLE");

            if (gpiod_line_request_set_value(
                    hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Yellow LED OFF error in IDLE");

            if (gpiod_line_request_set_value(
                    hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Green LED OFF error in IDLE");

        } else if (local_state == STATE_SCANNING) {
            if (pwm_write(pwm, "enable", "0") < 0)
                report_light_error(local_state, "Light: PWM disable error in SCANNING");

            if (gpiod_line_request_set_value(
                    hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                report_light_error(local_state, "Light: Yellow LED ON error in SCANNING");

            if (gpiod_line_request_set_value(
                    hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Green LED OFF error in SCANNING");

        } else if (local_state == STATE_ALARM) {
            if (gpiod_line_request_set_value(
                    hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Yellow LED OFF error in ALARM");

            if (gpiod_line_request_set_value(
                    hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Green LED OFF error in ALARM");

            if (pwm_write(pwm, "enable", "0") < 0 ||
                pwm_write(pwm, "duty_cycle", "0") < 0 ||
                pwm_write(pwm, "period", "1000000000") < 0 ||
                pwm_write(pwm, "duty_cycle", "500000000") < 0 ||
                pwm_write(pwm, "enable", "1") < 0) {
                report_light_error(local_state, "Light: PWM start error in ALARM");
            }

            pthread_mutex_lock(&sys_mut);
            while (curr_state == STATE_ALARM && is_run) {
                pthread_cond_wait(&sys_sig, &sys_mut);
            }
            pthread_mutex_unlock(&sys_mut);

            if (pwm_write(pwm, "enable", "0") < 0)
                report_light_error(local_state, "Light: PWM stop error after ALARM");

        } else if (local_state == STATE_DISARMED) {
            if (pwm_write(pwm, "enable", "0") < 0)
                report_light_error(local_state, "Light: PWM disable error in DISARMED");

            if (gpiod_line_request_set_value(
                    hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Yellow LED OFF error in DISARMED");

            if (gpiod_line_request_set_value(
                    hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                report_light_error(local_state, "Light: Green LED ON error in DISARMED");

            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;

            pthread_mutex_lock(&sys_mut);
            while (curr_state == STATE_DISARMED && is_run) {
                int rc = pthread_cond_timedwait(&sys_sig, &sys_mut, &ts);
                if (rc == ETIMEDOUT) {
                    break;
                }
            }
            pthread_mutex_unlock(&sys_mut);

            if (gpiod_line_request_set_value(
                    hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                report_light_error(local_state, "Light: Green LED OFF error after DISARMED timeout");

        } else if (local_state == STATE_ERROR) {
            if (pwm_write(pwm, "enable", "0") < 0)
                report_light_error(local_state, "Light: PWM disable error in ERROR");

            if (gpiod_line_request_set_value(
                    hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                report_light_error(local_state, "Light: Yellow LED ON error in ERROR");

            if (gpiod_line_request_set_value(
                    hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                report_light_error(local_state, "Light: Green LED ON error in ERROR");
        }
    }

    pwm_write(pwm, "enable", "0");
    gpiod_line_request_set_value(hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_request_set_value(hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE);

    return NULL;
}




