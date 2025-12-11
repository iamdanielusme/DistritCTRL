#ifndef STEP_SEQUENCER_H
#define STEP_SEQUENCER_H

#include <stdint.h>
#include <stdbool.h>

// Inicialización del secuenciador
void stepseq_init(void);

// Callbacks desde MIDI Clock
void stepseq_on_clock_tick(void);  // se llama en cada 0xF8
void stepseq_on_start(void);       // MIDI START
void stepseq_on_stop(void);        // MIDI STOP

// Getters para UI (OLED, anillo, etc.)
bool    stepseq_is_running(void);       // true = PLAY, false = STOP
uint8_t stepseq_get_current_step(void); // 0..15 (para 16 pasos)

#endif // STEP_SEQUENCER_H

