#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include <stdint.h>
#include <stdbool.h>

// Estado lógico del controlador que queremos mostrar en pantalla
typedef struct {
    uint16_t bpm;        // BPM actual
    bool     has_clock;  // ¿Recibiendo clock MIDI?
    bool     playing;    // ¿Reproduciendo (PLAY) o detenido (STOP)?
    uint8_t  step;       // Paso actual (1..total_steps)
    uint8_t  total_steps; // Total de pasos (típicamente 16)
} controller_status_t;

// Inicializa la OLED (I2C + SH1106)
void display_init(void);

// Actualiza el estado que se va a dibujar
void display_set_status(const controller_status_t *st);

// Redibuja la pantalla según el estado actual
void display_task(void);

#endif // DISPLAY_OLED_H
