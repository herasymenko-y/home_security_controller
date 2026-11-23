#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gpiod.h>
#include "sys_state.h"
#include "atmosphere.h"
#include "buzzer.h"
#include "pir.h"
#include "light.h"
#include "rfid.h"
#include "button_pow_off.h"
#include "button_res.h"
#include "hardware_config.h"
#include "spi_config.h"
#include "i2c_config.h"
#include "pwm_config.h"
#include "event_queue.h"
#include "logger.h"

int curr_state = STATE_IDLE;
atomic_bool is_run = ATOMIC_VAR_INIT(1);

pthread_mutex_t sys_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sys_sig = PTHREAD_COND_INITIALIZER;
sem_t sem_rfid;

static int start_thread(pthread_t *tid, void *(*func)(void *), void *arg,
                        const char *name, int *started) {
    int rc = pthread_create(tid, NULL, func, arg);
    if (rc != 0) {
        fprintf(stderr, "ERROR: Failed to start %s thread: %s\n", name, strerror(rc));
        log_event("ERROR", name);
        return -1;
    }
    *started = 1;
    return 0;
}

static void cleanup_resources(struct gpio_param *hw, struct pwm_param *pwm,
                              int pwm_initialized, int i2c_fd, int spi_fd,
                              int sem_initialized, int logger_initialized) {
    if (pwm_initialized && pwm_close(pwm) < 0) {
        fprintf(stderr, "ERROR: PWM closing failed\n");
    }

    gpio_resources_close(hw);

    if (hw->chip != NULL) {
        gpiod_chip_close(hw->chip);
        hw->chip = NULL;
    }

    if (i2c_fd >= 0 && close(i2c_fd) < 0) {
        perror("I2C close");
    }

    if (spi_fd >= 0 && close(spi_fd) < 0) {
        perror("SPI close");
    }

    if (sem_initialized && sem_destroy(&sem_rfid) < 0) {
        perror("Semaphore destruction");
    }

    if (logger_initialized) {
        log_event("EXIT", "Home security daemon stopped");
        if (logger_close() != 0) {
            fprintf(stderr, "ERROR: Logger close failed\n");
        }
    }
}

int main(void) {
    int ret = 0;
    int spi_fd = -1;
    int i2c_fd = -1;
    int sem_initialized = 0;
    int pwm_initialized = 0;
    int logger_initialized = 0;

    struct gpio_param hw = {0};
    struct pwm_param pwm = {
        .enable_fd = -1,
        .period_fd = -1,
        .duty_cycle_fd = -1
    };

    struct light_param light_args = {
        .hw = &hw,
        .pwm = &pwm
    };

    pthread_t event_loop, rfid, light, pir, buzzer;
    pthread_t atmosphere, button_pow_off, button_res;

    int event_started = 0, rfid_started = 0, light_started = 0, pir_started = 0;
    int buzzer_started = 0, atmosphere_started = 0;
    int button_pow_off_started = 0, button_res_started = 0;

    if (logger_init("security.log") != 0) {
        fprintf(stderr, "ERROR: Logger initialization failed\n");
        return -1;
    }
    logger_initialized = 1;
    log_event("INIT", "Home security daemon starting");

    if (sem_init(&sem_rfid, 0, 0) < 0) {
        perror("Semaphore initialization");
        cleanup_resources(&hw, &pwm, pwm_initialized, i2c_fd, spi_fd, sem_initialized, logger_initialized);
        return -1;
    }
    sem_initialized = 1;

    spi_fd = spi_init();
    if (spi_fd < 0) {
        fprintf(stderr, "ERROR: SPI initialization failed\n");
        cleanup_resources(&hw, &pwm, pwm_initialized, i2c_fd, spi_fd, sem_initialized, logger_initialized);
        return -1;
    }

    i2c_fd = i2c_init();
    if (i2c_fd < 0) {
        fprintf(stderr, "ERROR: I2C initialization failed\n");
        cleanup_resources(&hw, &pwm, pwm_initialized, i2c_fd, spi_fd, sem_initialized, logger_initialized);
        return -1;
    }

    hw.chip = gpio_init();
    if (hw.chip == NULL) {
        fprintf(stderr, "ERROR: GPIO chip initialization failed\n");
        cleanup_resources(&hw, &pwm, pwm_initialized, i2c_fd, spi_fd, sem_initialized, logger_initialized);
        return -1;
    }

    hw.pir = pir_init(hw.chip);
    hw.yellow_led = yellow_led_init(hw.chip);
    hw.green_led = green_led_init(hw.chip);
    hw.buzzer = buzzer_init(hw.chip);
    hw.button_res = button_res_init(hw.chip);
    hw.button_pow_off = button_pow_off_init(hw.chip);

    if (pwm_init(&pwm) < 0) {
        fprintf(stderr, "ERROR: PWM initialization failed\n");
        cleanup_resources(&hw, &pwm, pwm_initialized, i2c_fd, spi_fd, sem_initialized, logger_initialized);
        return -1;
    }
    pwm_initialized = 1;

    /* 1. Запускаємо чергу подій та конс'юмери */
    if (start_thread(&event_loop, event_loop_thread, NULL, "event_loop", &event_started) != 0 ||
        start_thread(&light, light_thread, &light_args, "light", &light_started) != 0 ||
        start_thread(&buzzer, buzzer_thread, &hw, "buzzer", &buzzer_started) != 0 ||
        /* 2. Запускаємо продюсери */
        start_thread(&pir, pir_thread, &hw, "pir", &pir_started) != 0 ||
        start_thread(&rfid, rfid_thread, &spi_fd, "rfid", &rfid_started) != 0 ||
        start_thread(&atmosphere, atmosphere_thread, &i2c_fd, "atmosphere", &atmosphere_started) != 0 ||
        start_thread(&button_pow_off, button_pow_off_thread, &hw, "button_pow_off", &button_pow_off_started) != 0 ||
        start_thread(&button_res, button_res_thread, &hw, "button_res", &button_res_started) != 0) {

        is_run = 0;
        pthread_mutex_lock(&sys_mut);
        pthread_cond_broadcast(&sys_sig);
        pthread_mutex_unlock(&sys_mut);
        sem_post(&sem_rfid);
        event_queue_stop();
        ret = -1;
    }

    /* 1. Спочатку завершуємо всі ПРОДЮСЕРИ */
    if (pir_started)            pthread_join(pir, NULL);
    if (rfid_started)           pthread_join(rfid, NULL);
    if (atmosphere_started)     pthread_join(atmosphere, NULL);
    if (button_pow_off_started) pthread_join(button_pow_off, NULL);
    if (button_res_started)     pthread_join(button_res, NULL);

    /* 2. Зупиняємо та завершуємо ЧЕРГУ ПОДІЙ */
    if (event_started) {
        if (event_queue_stop() != 0) {
            fprintf(stderr, "ERROR: Failed to stop event queue\n");
        }
        pthread_join(event_loop, NULL);
    }

    /* 3. Будимо і завершуємо всі КОНС'ЮМЕРИ */
    pthread_mutex_lock(&sys_mut);
    pthread_cond_broadcast(&sys_sig);
    pthread_mutex_unlock(&sys_mut);

    if (light_started)  pthread_join(light, NULL);
    if (buzzer_started) pthread_join(buzzer, NULL);

    cleanup_resources(&hw, &pwm, pwm_initialized, i2c_fd, spi_fd, sem_initialized, logger_initialized);

    return ret;
}

