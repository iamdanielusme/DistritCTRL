// fader_driver.h - Driver de 3 faders analógicos en el MASTER (Pico W)

#ifndef FADER_DRIVER_H
#define FADER_DRIVER_H

#include <stdint.h>

#define FADER_DRIVER_NUM_CHANNELS  3

// Inicializa el ADC y los pines de los 3 faders
void fader_driver_init(void);

// Leer todos los faders (no bloquea, sólo hace 3 lecturas ADC)
void fader_driver_update(void);

// Devuelve el valor RAW de 12 bits (0..4095) del fader ch (0..2)
uint16_t fader_driver_get_raw(int ch);

#endif // FADER_DRIVER_H
