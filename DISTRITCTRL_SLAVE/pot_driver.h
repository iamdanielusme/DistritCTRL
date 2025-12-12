#ifndef POT_DRIVER_H
#define POT_DRIVER_H

#include <stdint.h>

// Número de potenciómetros (canales del ADS1115 que usamos)
#define POT_DRIVER_NUM_CHANNELS 4

// Inicializa I2C y deja listo el ADS1115
void pot_driver_init(void);

// Debe llamarse periódicamente en el loop principal
// Va recorriendo los 4 canales y actualizando las lecturas
void pot_driver_update(void);

// Devuelve el valor “normalizado” a 12 bits (0..4095)
// index: 0..POT_DRIVER_NUM_CHANNELS-1
uint16_t pot_driver_get_12bit(int index);

#endif // POT_DRIVER_H