#include "button_driver.h"
#include "hardware/gpio.h"

void button_init(Button *b, uint8_t gpio) {
    b->gpio = gpio;
    b->last_state = false;

    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);  // botón a GND
}

bool button_read(Button *b) {
    return !gpio_get(b->gpio);  // activo en bajo
}

bool button_was_pressed(Button *b) {
    bool current = button_read(b);
    bool pressed  = (current && !b->last_state);
    b->last_state = current;
    return pressed;
}
