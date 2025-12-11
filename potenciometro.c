#include "pot_driver.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

static uint pot_adc_pin;
static uint8_t midi_cc = 1;     
static int last_midi_value = -1;
static const int threshold = 2; 

#define MIDI_CHANNEL 0          
#define UART_ID uart0           
#define MIDI_BAUD 31250         

void pot_set_cc(uint8_t cc) {
    midi_cc = cc;
}


static void midi_send_cc(uint8_t cc, uint8_t value) {
    uint8_t status = 0xB0 | (MIDI_CHANNEL & 0x0F);
    uart_putc_raw(UART_ID, status);
    uart_putc_raw(UART_ID, cc);
    uart_putc_raw(UART_ID, value);
}

void pot_init(uint adc_pin) {
    stdio_init_all(); // permite printf por USB CDC para mensajes de confirmación

    if (adc_pin < 26 || adc_pin > 29) {
        printf("pot_init: pin ADC %u inválido, usando GP26\n", adc_pin);
        adc_pin = 26;
    }

    pot_adc_pin = adc_pin;

    // --- Inicializar UART MIDI ---
    uart_init(UART_ID, MIDI_BAUD);
    gpio_set_function(0, GPIO_FUNC_UART); // TX de uart0 en GP0

    // --- Inicializar ADC ---
    adc_init();
    adc_gpio_init(adc_pin);

    // Seleccionar canal ADC interno (0–3)
    uint channel = adc_pin - 26;
    adc_select_input(channel);

    // Mensaje de confirmación
    printf("pot_init: wiper en GP%u (ADC chan %u), MIDI TX en GP0\n", pot_adc_pin, channel);
}

void pot_update() {
    uint16_t raw = adc_read();  // 0–4095

    // Convertir a MIDI 0–127
    uint8_t midi_value = (raw * 127) / 4095;

    // Si el valor no cambió lo suficiente, no enviar
    if (last_midi_value < 0 || abs(midi_value - last_midi_value) >= threshold) {
        last_midi_value = midi_value;
        midi_send_cc(midi_cc, midi_value);
    }
}
