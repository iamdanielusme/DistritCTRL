#include "midi_uart.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"

#define UART_ID uart0
#define TX_PIN 0
#define RX_PIN 1

void midi_uart_init(void) {
    uart_init(UART_ID, 31250);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);
}

static void midi_write(uint8_t b) {
    uart_putc_raw(UART_ID, b);
}

void midi_send_cc(uint8_t chan, uint8_t cc, uint8_t value) {
    midi_write(0xB0 | (chan & 0x0F));
    midi_write(cc & 0x7F);
    midi_write(value & 0x7F);
}

void midi_send_note_on(uint8_t chan, uint8_t note, uint8_t vel) {
    midi_write(0x90 | (chan & 0x0F));
    midi_write(note & 0x7F);
    midi_write(vel & 0x7F);
}

void midi_send_note_off(uint8_t chan, uint8_t note, uint8_t vel) {
    midi_write(0x80 | (chan & 0x0F));
    midi_write(note & 0x7F);
    midi_write(vel & 0x7F);
}
