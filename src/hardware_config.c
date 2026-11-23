#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "hardware_config.h"


struct gpiod_chip *gpio_init(){
struct gpiod_chip *chip = gpiod_chip_open (GPIO_PATH);
if (chip == NULL){
return NULL;
}
return chip;
}


static struct gpiod_line_request *gpio_request(struct gpiod_chip *chip, const struct gpio_config *config) {
if (chip == NULL){
return NULL;
}

if (config == NULL){
return NULL;
}

struct gpiod_line_settings *settings = gpiod_line_settings_new();
if (settings == NULL){
return NULL;
}
if (gpiod_line_settings_set_direction(settings, config->direction) < 0){
gpiod_line_settings_free(settings);
return NULL;
}

if (gpiod_line_settings_set_edge_detection(settings, config->edge) < 0 ){
gpiod_line_settings_free(settings);
return NULL;
}

gpiod_line_settings_set_active_low(settings, config->active_low);
if (config->direction == GPIOD_LINE_DIRECTION_OUTPUT){
if (gpiod_line_settings_set_output_value(settings, config->output_value) < 0){
gpiod_line_settings_free(settings);
return NULL;
}
}

struct gpiod_line_config *line_config = gpiod_line_config_new();

if (line_config == NULL) {
    gpiod_line_settings_free(settings);
    return NULL;
}
unsigned int offset = config->pin;
if (gpiod_line_config_add_line_settings(line_config, &offset, 1, settings) < 0){
gpiod_line_config_free (line_config);
gpiod_line_settings_free(settings);
return NULL;
}

struct gpiod_line_request *request = gpiod_chip_request_lines(chip, NULL, line_config);
if (request == NULL){
gpiod_line_config_free(line_config);
gpiod_line_settings_free(settings);
return NULL;
}

gpiod_line_config_free(line_config);
gpiod_line_settings_free(settings);
return request;
}


struct gpiod_line_request *pir_init(struct gpiod_chip *chip)
{
    static const struct gpio_config config = {
        .pin = GPIO_PIR,
        .direction = GPIOD_LINE_DIRECTION_INPUT,
        .edge = GPIOD_LINE_EDGE_BOTH,
        .active_low = 0,
    };

    return gpio_request(chip, &config);
}

struct gpiod_line_request *buzzer_init(struct gpiod_chip *chip)
{
    static const struct gpio_config config = {
        .pin = GPIO_BUZZER,
        .direction = GPIOD_LINE_DIRECTION_OUTPUT,
        .edge = GPIOD_LINE_EDGE_NONE,
        .active_low = 1,
        .output_value = GPIOD_LINE_VALUE_INACTIVE
    };

    return gpio_request(chip, &config);
}

struct gpiod_line_request *yellow_led_init(struct gpiod_chip *chip)
{
    static const struct gpio_config config = {
        .pin = GPIO_YELLOW_LED,
        .direction = GPIOD_LINE_DIRECTION_OUTPUT,
        .edge = GPIOD_LINE_EDGE_NONE,
        .active_low = 0,
        .output_value = GPIOD_LINE_VALUE_INACTIVE
    };

    return gpio_request(chip, &config);
}

struct gpiod_line_request *green_led_init(struct gpiod_chip *chip)
{
    static const struct gpio_config config = {
        .pin = GPIO_GREEN_LED,
        .direction = GPIOD_LINE_DIRECTION_OUTPUT,
        .edge = GPIOD_LINE_EDGE_NONE,
        .active_low = 0,
        .output_value = GPIOD_LINE_VALUE_INACTIVE
    };

    return gpio_request(chip, &config);
}


struct gpiod_line_request *button_pow_off_init(struct gpiod_chip *chip)
{
    static const struct gpio_config config = {
        .pin = GPIO_BUTTON_POW_OFF,
        .direction = GPIOD_LINE_DIRECTION_INPUT,
        .edge = GPIOD_LINE_EDGE_FALLING,
        .active_low = 0,
    };

    return gpio_request(chip, &config);
}


struct gpiod_line_request *button_res_init(struct gpiod_chip *chip)
{
    static const struct gpio_config config = {
        .pin = GPIO_BUTTON_RES,
        .direction = GPIOD_LINE_DIRECTION_INPUT,
        .edge = GPIOD_LINE_EDGE_FALLING,
        .active_low = 0,
    };

    return gpio_request(chip, &config);
}


void gpio_resources_close(struct gpio_param *hw){
if (hw == NULL){
return;
}

if (hw->pir != NULL) {
    gpiod_line_request_release(hw->pir);
    hw->pir = NULL;
}

if (hw->buzzer != NULL) {
    gpiod_line_request_release(hw->buzzer);
    hw->buzzer = NULL;
}

if (hw->yellow_led != NULL) {
    gpiod_line_request_release(hw->yellow_led);
    hw->yellow_led = NULL;
}

if (hw->green_led != NULL) {
    gpiod_line_request_release(hw->green_led);
    hw->green_led = NULL;
}

if (hw->button_pow_off != NULL) {
    gpiod_line_request_release(hw->button_pow_off);
    hw->button_pow_off = NULL;
}

if (hw->button_res != NULL) {
    gpiod_line_request_release(hw->button_res);
    hw->button_res = NULL;
}
}










