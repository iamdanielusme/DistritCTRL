// fader_driver.c - Implementación de 3 faders en ADC0,1,2 (GP26,27,28)

#include "fader_driver.h"

#include "pico/stdlib.h"
#include "hardware/adc.h"

// Pines físicos donde conectas los faders en el MASTER
// GP26 -> ADC0, GP27 -> ADC1, GP28 -> ADC2
static const uint fader_gpio_pins[FADER_DRIVER_NUM_CHANNELS]   = {26, 27, 28};
static const uint fader_adc_inputs[FADER_DRIVER_NUM_CHANNELS]  = { 0,  1,  2};

static uint16_t s_fader_raw[FADER_DRIVER_NUM_CHANNELS] = {0};

void fader_driver_init(void)
{
    adc_init();

    for (int i = 0; i < FADER_DRIVER_NUM_CHANNELS; i++) {
        adc_gpio_init(fader_gpio_pins[i]);
    }

    // Lecturas iniciales
    fader_driver_update();
}

void fader_driver_update(void)
{
    for (int i = 0; i < FADER_DRIVER_NUM_CHANNELS; i++) {
        adc_select_input(fader_adc_inputs[i]);
        // Pequeño delay opcional si ves ruido raro:
        // sleep_us(5);
        s_fader_raw[i] = adc_read();  // 0..4095
    }
}

uint16_t fader_driver_get_raw(int ch)
{
    if (ch < 0 || ch >= FADER_DRIVER_NUM_CHANNELS) return 0;
    return s_fader_raw[ch];
}
