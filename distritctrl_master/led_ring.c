// led_ring.c - Driver para anillo WS2812B (NeoPixel) en RP2040

#include "led_ring.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <string.h>

// ---------------------------------------------------------------------
//  Programa PIO WS2812 (basado en el de Adafruit / ejemplo oficial)
// ---------------------------------------------------------------------

#define ws2812_wrap_target 0
#define ws2812_wrap        3

#define ws2812_T1 2
#define ws2812_T2 5
#define ws2812_T3 3

static const uint16_t ws2812_program_instructions[] = {
    // .wrap_target
    0x6221, // 0: out    x, 1            side 0 [2]
    0x1123, // 1: jmp    !x, 3           side 1 [1]
    0x1400, // 2: jmp    0               side 1 [4]
    0xa442, // 3: nop                    side 0 [4]
    // .wrap
};

static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length       = 4,
    .origin       = -1,
};

static inline pio_sm_config ws2812_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + ws2812_wrap_target, offset + ws2812_wrap);
    sm_config_set_sideset(&c, 1, false, false);
    return c;
}

static inline void ws2812_program_init(PIO pio,
                                       uint sm,
                                       uint offset,
                                       uint pin,
                                       float freq,
                                       uint bits)
{
    // Configura el pin como salida del PIO
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);

    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);

    // Shift de salida: MSB first, autopull, "bits" por palabra (24 bits/pixel)
    sm_config_set_out_shift(&c, false, true, bits);

    // FIFO TX unida
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    int cycles_per_bit = ws2812_T1 + ws2812_T2 + ws2812_T3;
    float div = clock_get_hz(clk_sys) / (freq * cycles_per_bit);
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// ---------------------------------------------------------------------
//  Estado del anillo y funciones de alto nivel
// ---------------------------------------------------------------------

static PIO  led_pio      = pio0;
static uint led_sm       = 0;
static uint led_offset   = 0;

// Guardamos los colores en formato R,G,B por LED
static uint8_t led_buffer[LED_RING_NUM_LEDS * 3];

static inline void put_pixel(uint32_t pixel_grb) {
    // El programa PIO espera 24 bits alineados en los 24 MSB.
    // Enviamos 32 bits pero desplazando 8: GRB << 8.
    pio_sm_put_blocking(led_pio, led_sm, pixel_grb << 8u);
}

// Inicialización del anillo
void led_ring_init(void)
{
    led_pio    = pio0;
    led_sm     = 0;
    led_offset = pio_add_program(led_pio, &ws2812_program);

    // 800 kHz, 24 bits por píxel
    ws2812_program_init(led_pio, led_sm, led_offset, LED_RING_PIN, 800000.0f, 24);

    memset(led_buffer, 0, sizeof(led_buffer));
    led_ring_show();
}

// Set de un píxel en RAM (no envía todavía)
void led_ring_set_pixel(uint index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= LED_RING_NUM_LEDS) return;

    uint i = index * 3;
    led_buffer[i + 0] = r;
    led_buffer[i + 1] = g;
    led_buffer[i + 2] = b;
}

// Rellena todos los píxeles en RAM (no envía todavía)
void led_ring_fill(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint i = 0; i < LED_RING_NUM_LEDS; i++) {
        led_ring_set_pixel(i, r, g, b);
    }
}

// Borra el buffer (todos a negro) y lo envía
void led_ring_clear(void)
{
    led_ring_fill(0, 0, 0);
}

// Envía el buffer completo al anillo
void led_ring_show(void)
{
    for (uint i = 0; i < LED_RING_NUM_LEDS; i++) {
        uint idx = i * 3;
        uint8_t r = led_buffer[idx + 0];
        uint8_t g = led_buffer[idx + 1];
        uint8_t b = led_buffer[idx + 2];

        // WS2812B es GRB
        uint32_t grb = ((uint32_t)g << 16) |
                       ((uint32_t)r <<  8) |
                       ((uint32_t)b <<  0);

        put_pixel(grb);
    }

    // Tiempo de latch mínimo ~300 µs
    sleep_us(300);
}
