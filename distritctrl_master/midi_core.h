// midi_core.h
#ifndef MIDI_CORE_H_
#define MIDI_CORE_H_

#include <stdbool.h>
#include <stdint.h>


/**
 * Inicializa la pila USB MIDI (TinyUSB) para el RP2040.
 * Debes llamar a board_init() antes en main().
 */
void midi_core_init(void);

/**
 * Tarea principal de MIDI/USB.
 * Llama internamente a tud_task() y gestiona el parpadeo del LED
 * según el estado de conexión USB.
 */
void midi_core_task(void);

/**
 * Indica si la interfaz MIDI está montada por el host.
 * Equivalente a tud_midi_mounted(), pero encapsulado.
 */
bool midi_core_is_mounted(void);

uint16_t midi_core_get_bpm(void); 
/**
 * Envia un mensaje Note On por MIDI.
 * channel: 0–15 (0 = canal 1)
 * note: número de nota (0–127)
 * velocity: 0–127
 */
void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Envia un mensaje Note Off por MIDI.
 */
void midi_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * Envia un mensaje Control Change (CC).
 * cc: número de controlador (0–127)
 * value: valor del controlador (0–127)
 */
void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value);

#endif // MIDI_CORE_H_
