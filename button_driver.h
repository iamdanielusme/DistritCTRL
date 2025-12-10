#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t gpio;
    bool last_state;
} Button;

void button_init(Button *b, uint8_t gpio);
bool button_read(Button *b);
bool button_was_pressed(Button *b);

#endif
