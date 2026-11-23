
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
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

int curr_state = STATE_IDLE;
atomic_bool is_run = ATOMIC_VAR_INIT(1);

pthread_mutex_t sys_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sys_sig = PTHREAD_COND_INITIALIZER;
sem_t sem_rfid;


int main()
{
    int spi_fd = -1;
    int i2c_fd = -1;
    int sem_initialized = 0;

    struct gpio_param hw = {0};

    struct pwm_param pwm = {
        .enable_fd = -1,
        .period_fd = -1,
        .duty_cycle_fd = -1
    };

    pthread_t rfid, light, pir, buzzer;
    pthread_t atmosphere, button_pow_off, button_res;

    struct light_param light_args = {
        .hw = &hw,
        .pwm = &pwm
    };


    if (sem_init(&sem_rfid, 0, 0) < 0) {
        perror("Semaphore initialization");
        return -1;
    }

    sem_initialized = 1;


    spi_fd = spi_init();

    if (spi_fd < 0) {
        printf("ERROR: SPI initialization failed\n");

        sem_destroy(&sem_rfid);
        return -1;
    }


    i2c_fd = i2c_init();

    if (i2c_fd < 0) {
        printf("ERROR: I2C initialization failed\n");

        close(spi_fd);
        sem_destroy(&sem_rfid);
        return -1;
    }


    hw.chip = gpio_init();

    if (hw.chip == NULL) {
        printf("ERROR: GPIO chip initialization failed\n");

        close(i2c_fd);
        close(spi_fd);
        sem_destroy(&sem_rfid);
        return -1;
    }


    hw.pir = pir_init(hw.chip);
    hw.yellow_led = yellow_led_init(hw.chip);
    hw.green_led = green_led_init(hw.chip);
    hw.buzzer = buzzer_init(hw.chip);
    hw.button_res = button_res_init(hw.chip);
    hw.button_pow_off = button_pow_off_init(hw.chip);


    if (pwm_init(&pwm) < 0) {
        printf("ERROR: PWM initialization failed\n");

        pwm_close(&pwm);
        gpio_resources_close(&hw);
        gpiod_chip_close(hw.chip);
        close(i2c_fd);
        close(spi_fd);
        sem_destroy(&sem_rfid);
        return -1;
    }


    pthread_create(&rfid, NULL, rfid_thread, &spi_fd);
    pthread_create(&light, NULL, light_thread, &light_args);
    pthread_create(&pir, NULL, pir_thread, &hw);
    pthread_create(&buzzer, NULL, buzzer_thread, &hw);
    pthread_create(&atmosphere, NULL, atmosphere_thread, &i2c_fd);
    pthread_create(&button_pow_off, NULL, button_pow_off_thread, &hw);
    pthread_create(&button_res, NULL, button_res_thread, &hw);


    pthread_join(pir, NULL);
    pthread_join(rfid, NULL);
    pthread_join(light, NULL);
    pthread_join(buzzer, NULL);
    pthread_join(atmosphere, NULL);
    pthread_join(button_pow_off, NULL);
    pthread_join(button_res, NULL);


    if (pwm_close(&pwm) < 0) {
        printf("ERROR: PWM closing failed\n");
    }

    gpio_resources_close(&hw);

    if (hw.chip != NULL) {
        gpiod_chip_close(hw.chip);
        hw.chip = NULL;
    }

    if (close(i2c_fd) < 0) {
        perror("I2C close");
    }

    if (close(spi_fd) < 0) {
        perror("SPI close");
    }

    if (sem_initialized) {
        if (sem_destroy(&sem_rfid) < 0) {
            perror("Semaphore destruction");
        }
    }

    return 0;
}

