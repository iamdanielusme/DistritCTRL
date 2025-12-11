// led_ring.h
#ifndef LED_RING_H
#define LED_RING_H

#include "pico/stdlib.h"
#include <stdint.h>

// Ajusta este pin al que realmente usas para DI del anillo
#define LED_RING_PIN      2      
#define LED_RING_NUM_LEDS 16

// API básica
void led_ring_init(void);
void led_ring_set_pixel(uint index, uint8_t r, uint8_t g, uint8_t b);
void led_ring_fill(uint8_t r, uint8_t g, uint8_t b);
void led_ring_clear(void);
void led_ring_show(void);

#endif // LED_RING_H
