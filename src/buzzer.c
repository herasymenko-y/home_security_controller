#include <pthread.h>
#include <gpiod.h>
#include <stddef.h>
#include <stdio.h>
#include "sys_state.h" 
#include "buzzer.h"
#include "hardware_config.h"

void* buzzer_thread (void* arg) {
struct gpio_param *hw = arg;

if (hw == NULL) {
    return NULL;
}
    int state = -1;
    
    while (is_run) {
        pthread_mutex_lock(&sys_mut);
        while ((state == curr_state) && (is_run)) {
            pthread_cond_wait(&sys_sig, &sys_mut);
        }
        state = curr_state;
        pthread_mutex_unlock(&sys_mut);
        
        printf("BUZZER: state = %d\n", state);
        if (!is_run) break;

        if (state == 2) {
            if (gpiod_line_request_set_value(
    hw->buzzer, GPIO_BUZZER, GPIOD_LINE_VALUE_ACTIVE) < 0) {
                perror("Buzzer GPIO LOW error");
            } else {
                printf("Buzzer: LOW (ON)\n");
            }
        } else {
            if (gpiod_line_request_set_value(
    hw->buzzer, GPIO_BUZZER, GPIOD_LINE_VALUE_INACTIVE) < 0) {
                perror("Buzzer GPIO HIGH error")
;
            } else {
                printf("Buzzer: HIGH (OFF)\n");
            }
        }
    }
    gpiod_line_request_set_value(
    hw->buzzer, GPIO_BUZZER, GPIOD_LINE_VALUE_INACTIVE);
    return NULL;
}
