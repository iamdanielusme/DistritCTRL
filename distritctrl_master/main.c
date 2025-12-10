// main.c - MASTER Distrit CTRL01
// Solo 1 botón físico (PLAY) -> Nota MIDI hacia Ableton

#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "bsp/board.h"

#include "midi_core.h"

// ---------------- Configuración de pines y MIDI ----------------

#define BTN_PLAY_PIN     2      // Botón físico en GP2

#define MIDI_CHANNEL     0      // Canal 1 (0 = ch1)
#define MIDI_NOTE_PLAY   60     // Nota que mapearemos a PLAY en Ableton (C4)

// Prototipo de la tarea de botón
static void button_play_task(void);

// ---------------------------- main -----------------------------

int main(void)
{
    // Inicializa hardware base de la placa
    board_init();
    stdio_init_all();

    // Configurar botón PLAY: entrada con pull-up en GP2
    gpio_init(BTN_PLAY_PIN);
    gpio_set_dir(BTN_PLAY_PIN, GPIO_IN);
    gpio_pull_up(BTN_PLAY_PIN);

    // Inicializar USB MIDI (TinyUSB) vía módulo midi_core
    midi_core_init();

    while (1) {
        // MIDI USB (enumeración, transfers, LED de estado)
        midi_core_task();

        // Lógica del botón de PLAY
        button_play_task();

        // Más adelante aquí añadiremos:
        // - app_master_task()
        // - recepción por UART desde el SLAVE
        // - actualización de OLED y anillo de LEDs
    }

    return 0;
}

// ----------------------- Botón PLAY ----------------------------

static void button_play_task(void)
{
    static bool btn_prev = true;  // por pull-up, "sin oprimir" = 1
    bool btn_now = gpio_get(BTN_PLAY_PIN);

    // Si aún no estamos montados como dispositivo MIDI, no mandamos nada
    if (!midi_core_is_mounted()) {
        btn_prev = btn_now;
        return;
    }

    // Flanco 1 -> 0: botón oprimido (Note On)
    if (btn_prev && !btn_now) {
        midi_send_note_on(MIDI_CHANNEL, MIDI_NOTE_PLAY, 100);
    }

    // Flanco 0 -> 1: botón liberado (Note Off)
    if (!btn_prev && btn_now) {
        midi_send_note_off(MIDI_CHANNEL, MIDI_NOTE_PLAY, 0);
    }

    btn_prev = btn_now;
}