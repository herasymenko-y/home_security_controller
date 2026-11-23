#ifndef PWM_CONFIG_H
#define PWM_CONFIG_H

#define PWM_PATH "/sys/class/pwm/pwmchip0/pwm1"
#define PWM_EXPORT "/sys/class/pwm/pwmchip0/export"

struct pwm_param {
    int enable_fd;
    int period_fd;
    int duty_cycle_fd;
};

int pwm_init(struct pwm_param *pwm);
int pwm_write(struct pwm_param *pwm, const char *filename, const char *value);
int pwm_close(struct pwm_param *pwm);

#endif // PWM_CONFIG_H

