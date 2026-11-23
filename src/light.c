
#include <pthread.h>
#include "sys_state.h"
#include "light.h"
#include "pwm_config.h"
#include <gpiod.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include "hardware_config.h"

void* light_thread(void* arg)
{
    struct light_param *param = arg;

    if (param == NULL || param->hw == NULL || param->pwm == NULL) {
        return NULL;
    }

    struct gpio_param *hw = param->hw;
    struct pwm_param *pwm = param->pwm;

    if (hw->yellow_led == NULL || hw->green_led == NULL ||
        pwm->enable_fd < 0 || pwm->period_fd < 0 ||
        pwm->duty_cycle_fd < 0) {
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
                perror("PWM enable");

            if (gpiod_line_request_set_value(
                hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                perror("Yellow LED");

            if (gpiod_line_request_set_value(
                hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                perror("Green LED");

        } else if (local_state == STATE_SCANNING) {

            if (pwm_write(pwm, "enable", "0") < 0)
                perror("PWM enable");

            if (gpiod_line_request_set_value(
                hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                perror("Yellow LED");

            if (gpiod_line_request_set_value(
                hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                perror("Green LED");

        } else if (local_state == STATE_ALARM) {

            if (gpiod_line_request_set_value(
                hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                perror("Yellow LED");

            if (gpiod_line_request_set_value(
                hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                perror("Green LED");

            if (pwm_write(pwm, "enable", "0") < 0)
                perror("PWM enable");

            if (pwm_write(pwm, "duty_cycle", "0") < 0)
                perror("PWM duty_cycle");

            if (pwm_write(pwm, "period", "1000000000") < 0)
                perror("PWM period");

            if (pwm_write(pwm, "duty_cycle", "500000000") < 0)
                perror("PWM duty_cycle");

            if (pwm_write(pwm, "enable", "1") < 0)
                perror("PWM enable");

            while (is_run) {

                pthread_mutex_lock(&sys_mut);

                int still_alarm = (curr_state == STATE_ALARM);

                pthread_mutex_unlock(&sys_mut);

                if (!still_alarm) {
                    break;
                }

                usleep(20000);
            }

            if (pwm_write(pwm, "enable", "0") < 0)
                perror("PWM enable");

        } else if (local_state == STATE_DISARMED) {

            if (pwm_write(pwm, "enable", "0") < 0)
                perror("PWM enable");

            if (gpiod_line_request_set_value(
                hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
                perror("Yellow LED");

            if (gpiod_line_request_set_value(
                hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                perror("Green LED");

        } else if (local_state == STATE_ERROR) {

            if (pwm_write(pwm, "enable", "0") < 0)
                perror("PWM enable");

            if (gpiod_line_request_set_value(
                hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                perror("Yellow LED");

            if (gpiod_line_request_set_value(
                hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_ACTIVE) < 0)
                perror("Green LED");
        }
    }

    if (pwm_write(pwm, "enable", "0") < 0)
        perror("PWM enable");

    if (gpiod_line_request_set_value(
        hw->yellow_led, GPIO_YELLOW_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
        perror("Yellow LED");

    if (gpiod_line_request_set_value(
        hw->green_led, GPIO_GREEN_LED, GPIOD_LINE_VALUE_INACTIVE) < 0)
        perror("Green LED");

    return NULL;
}




