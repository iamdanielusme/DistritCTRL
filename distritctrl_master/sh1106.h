#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>     // <-- aquí se definen uint8_t, uint16_t, etc.
#include <stdbool.h>
#include "hardware/i2c.h"

#define SH1106_WIDTH   128
#define SH1106_HEIGHT  64

// Inicializar pantalla
void sh1106_init(i2c_inst_t *i2c, uint8_t addr);

// Limpiar buffer (pantalla en negro, hace falta llamar a update)
void sh1106_clear(void);

// Dibujar un píxel en (x,y)
void sh1106_draw_pixel(uint8_t x, uint8_t y, bool color);

// Enviar buffer completo a la pantalla
void sh1106_update(void);

#endif

