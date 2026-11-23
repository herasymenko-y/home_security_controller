
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <gpiod.h>

// GPIO configuration 

struct gpio_config {
    unsigned int pin;
    enum gpiod_line_direction direction;
    enum gpiod_line_edge edge;
    int active_low;
    enum gpiod_line_value output_value;
};

#define GPIO_PATH "/dev/gpiochip4"

#define GPIO_PIR          25

#define GPIO_YELLOW_LED   24
#define GPIO_GREEN_LED    27

#define GPIO_BUZZER       19

#define GPIO_BUTTON_RES        5
#define GPIO_BUTTON_POW_OFF     6

// hardware parameters structure
struct gpio_param {
    struct gpiod_chip *chip;

    struct gpiod_line_request *pir;
    struct gpiod_line_request *yellow_led;
    struct gpiod_line_request *green_led;
    struct gpiod_line_request *buzzer;
    struct gpiod_line_request *button_res;
    struct gpiod_line_request *button_pow_off;
    
};

// initialization and cleanup
struct gpiod_line_request *pir_init(struct gpiod_chip *chip);
struct gpiod_line_request *buzzer_init(struct gpiod_chip *chip);
struct gpiod_line_request *button_res_init(struct gpiod_chip *chip);
struct gpiod_line_request *button_pow_off_init(struct gpiod_chip *chip);
struct gpiod_line_request *yellow_led_init(struct gpiod_chip *chip);
struct gpiod_line_request *green_led_init(struct gpiod_chip *chip);
void gpio_resources_close(struct gpio_param *hw);

// Hardware initialization 
struct gpiod_chip *gpio_init(void);

#endif // HARDWARE_CONFIG_H

