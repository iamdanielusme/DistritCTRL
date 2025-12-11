#pragma once
#include <stdint.h>

// Inicializa ADC + UART para MIDI.
// adc_pin = 26, 27 o 28 típicamente
void pot_init(uint adc_pin);

// Lee el pot y si cambió envía MIDI CC por UART
void pot_update();

// Cambia el CC que envía (por defecto CC#1 Mod Wheel)
void pot_set_cc(uint8_t cc);
