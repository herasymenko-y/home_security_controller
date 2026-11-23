
#include "pwm_config.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


int pwm_init(struct pwm_param *pwm)
{
    if (pwm == NULL) {
        return -1;
    }

    pwm->enable_fd = -1;
    pwm->period_fd = -1;
    pwm->duty_cycle_fd = -1;

int export_fd = open(PWM_EXPORT, O_WRONLY | O_CLOEXEC);
if (export_fd >= 0){
write(export_fd, "1", 1);
close (export_fd);
usleep(100000);
}

    pwm->enable_fd = open(PWM_PATH "/enable", O_WRONLY | O_CLOEXEC);

    if (pwm->enable_fd < 0) {
        perror("PWM enable open");
        pwm_close(pwm);
        return -1;
    }

    pwm->period_fd = open(PWM_PATH "/period", O_WRONLY | O_CLOEXEC);

    if (pwm->period_fd < 0) {
        perror("PWM period open");
        pwm_close(pwm);
        return -1;
    }

    pwm->duty_cycle_fd = open(PWM_PATH "/duty_cycle", O_WRONLY | O_CLOEXEC);

    if (pwm->duty_cycle_fd < 0) {
        perror("PWM duty_cycle open");
        pwm_close(pwm);
        return -1;
    }

    return 0;
}


int pwm_write(struct pwm_param *pwm, const char *filename, const char *value)
{
    if (pwm == NULL || filename == NULL || value == NULL) {
        return -1;
    }

    int fd = -1;

    if (strcmp(filename, "enable") == 0) {
        fd = pwm->enable_fd;
    } else if (strcmp(filename, "period") == 0) {
        fd = pwm->period_fd;
    } else if (strcmp(filename, "duty_cycle") == 0) {
        fd = pwm->duty_cycle_fd;
    } else {
        printf("Unknown PWM file\n");
        return -1;
    }

    if (fd < 0) {
        printf("PWM file is not open: %s\n", filename);
        return -1;
    }

    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("PWM lseek");
        return -1;
    }

    size_t len = strlen(value);

    ssize_t result = write(fd, value, len);

    if (result < 0) {
        perror("PWM write");
        return -1;
    }

    if ((size_t)result != len) {
        printf("Incomplete PWM write: %s\n", filename);
        return -1;
    }

    return 0;
}


int pwm_close(struct pwm_param *pwm)
{
    if (pwm == NULL) {
        return -1;
    }

    int result = 0;

    if (pwm->enable_fd >= 0) {
        if (close(pwm->enable_fd) < 0) {
            perror("PWM enable close");
            result = -1;
        }

        pwm->enable_fd = -1;
    }

    if (pwm->period_fd >= 0) {
        if (close(pwm->period_fd) < 0) {
            perror("PWM period close");
            result = -1;
        }

        pwm->period_fd = -1;
    }

    if (pwm->duty_cycle_fd >= 0) {
        if (close(pwm->duty_cycle_fd) < 0) {
            perror("PWM duty_cycle close");
            result = -1;
        }

        pwm->duty_cycle_fd = -1;
    }

    return result;
}

