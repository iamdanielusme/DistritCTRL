#ifndef MIDI_UART_H
#define MIDI_UART_H

#include <stdint.h>

void midi_uart_init(void);
void midi_send_cc(uint8_t chan, uint8_t cc, uint8_t value);
void midi_send_note_on(uint8_t chan, uint8_t note, uint8_t vel);
void midi_send_note_off(uint8_t chan, uint8_t note, uint8_t vel);

#endif
