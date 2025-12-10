#include "fader_driver.h"
#include "hardware/adc.h"
#include "pico/stdlib.h"

void fader_init(Fader *f, uint32_t adc_channel) {
    // Inicializa ADC (safe si se llama múltiples veces)
    static bool adc_inited = false;
    if (!adc_inited) {
        adc_init();
        adc_inited = true;
    }

    // Configura el pin GPIO correspondiente al canal ADC
    // ADC0 -> GPIO26, ADC1 -> GPIO27, ADC2 -> GPIO28
    uint gpio = 26 + adc_channel;
    adc_gpio_init(gpio);

    // Selecciona el canal (se volverá a seleccionar en fader_get_raw)
    adc_select_input(adc_channel);

    f->adc_channel = adc_channel;
}

uint16_t fader_get_raw(Fader *f) {
    // Asegurarse de seleccionar el canal antes de leer (si hay múltiples faders)
    adc_select_input(f->adc_channel);

    // adc_read() devuelve 12-bit (0..4095)
    return adc_read();
}
