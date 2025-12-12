#include "button_driver.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/time.h"

#define DEBOUNCE_US   5000u  // 5 ms de debounce

typedef struct {
    bool     stable;          // estado estable (debounced)
    bool     last_reading;    // última lectura cruda del pin
    uint64_t last_change_us;  // tiempo de la última variación cruda
} button_state_t;

// Orden: primero 4 arcade, luego 4 normales
static const uint8_t s_button_pins[BUTTON_DRIVER_NUM_TOTAL] = {
    BTN_ARCADE_1_PIN,
    BTN_ARCADE_2_PIN,
    BTN_ARCADE_3_PIN,
    BTN_ARCADE_4_PIN,
    BTN_NORMAL_1_PIN,
    BTN_NORMAL_2_PIN,
    BTN_NORMAL_3_PIN,
    BTN_NORMAL_4_PIN
};

static button_state_t s_buttons[BUTTON_DRIVER_NUM_TOTAL];

void button_driver_init(void)
{
    for (int i = 0; i < BUTTON_DRIVER_NUM_TOTAL; i++) {
        uint8_t pin = s_button_pins[i];

        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        // Asumimos botón a GND => pull-up interno
        gpio_pull_up(pin);

        bool raw = (gpio_get(pin) == 0); // activo en LOW => presionado si 0

        s_buttons[i].stable         = raw;
        s_buttons[i].last_reading   = raw;
        s_buttons[i].last_change_us = time_us_64();
    }
}

void button_driver_update(void)
{
    uint64_t now = time_us_64();

    for (int i = 0; i < BUTTON_DRIVER_NUM_TOTAL; i++) {
        uint8_t pin = s_button_pins[i];

        bool raw = (gpio_get(pin) == 0); // activo en LOW => presionado

        if (raw != s_buttons[i].last_reading) {
            // cambio crudo → reinicia debounce
            s_buttons[i].last_reading   = raw;
            s_buttons[i].last_change_us = now;
        } else {
            // si se mantiene estable durante DEBOUNCE_US, actualiza estable
            if ((now - s_buttons[i].last_change_us) >= DEBOUNCE_US &&
                s_buttons[i].stable != raw)
            {
                s_buttons[i].stable = raw;
            }
        }
    }
}

uint8_t button_driver_get_arcade_mask(void)
{
    uint8_t mask = 0;

    for (int i = 0; i < BUTTON_DRIVER_NUM_ARCADE; i++) {
        if (s_buttons[i].stable) {
            mask |= (1u << i);
        }
    }
    return mask;
}

uint8_t button_driver_get_normal_mask(void)
{
    uint8_t mask = 0;

    for (int i = 0; i < BUTTON_DRIVER_NUM_NORMAL; i++) {
        int idx = BUTTON_DRIVER_NUM_ARCADE + i;
        if (s_buttons[idx].stable) {
            mask |= (1u << i);
        }
    }
    return mask;
}

uint8_t button_driver_get_all_mask(void)
{
    uint8_t arcade = button_driver_get_arcade_mask();
    uint8_t normal = button_driver_get_normal_mask();

    return (uint8_t)(arcade | (normal << 4));
}

bool button_driver_is_pressed(button_id_t id)
{
    if (id < 0 || id >= BUTTON_DRIVER_NUM_TOTAL) {
        return false;
    }
    return s_buttons[id].stable;
}
