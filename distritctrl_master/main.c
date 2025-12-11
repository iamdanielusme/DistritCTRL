#include "pico/stdlib.h"
#include "hardware/timer.h"

#include "midi_core.h"
#include "led_ring.h"
#include "step_sequencer.h"
#include "display_oled.h"

// Estado que le pasamos a la OLED
static controller_status_t ui_status;

// Timer para refrescar SOLO la UI (OLED)
static bool ui_timer_cb(repeating_timer_t *t)
{
    // --- Leer estado lógico desde los módulos ---

    // BPM actual calculado en midi_core a partir del clock MIDI
    ui_status.bpm = midi_core_get_bpm();

    // Usamos el BPM como “proxy” de si hay clock.
    // Si tu midi_core pone 0 cuando no hay clock, esto funciona bien.
    ui_status.has_clock = (ui_status.bpm > 0);

    // ¿El step sequencer está corriendo?
    ui_status.playing = stepseq_is_running();

    // Paso actual (stepseq_get_current_step devuelve 0..15)
    uint8_t cur_step = stepseq_get_current_step();
    ui_status.step        = (uint8_t)(cur_step + 1); // 1..16 para la pantalla
    ui_status.total_steps = 16;

    // Actualizar lo que la OLED debe mostrar
    display_set_status(&ui_status);
    display_task();   // redibuja el frame en el SH1106

    return true; // seguir repitiendo el timer
}

int main(void)
{
    stdio_init_all();   // Opcional, por si quieres debug por USB CDC

    // --- Inicializar hardware propio ---

    led_ring_init();    // WS2812 (usa PIO)
    stepseq_init();     // Estado del step sequencer
    display_init();     // I2C + SH1106 (OLED)

    // Inicializar MIDI + TinyUSB
    midi_core_init();

    // --- Timer periódico para la OLED (solo UI, nada de USB aquí) ---
    struct repeating_timer ui_timer;
    // Cada ~33 ms ≈ 30 FPS de refresco de display
    add_repeating_timer_ms(-33, ui_timer_cb, NULL, &ui_timer);

    // --- Bucle principal: aquí se atiende USB/MIDI de forma continua ---

    while (true) {
        // Procesar USB + MIDI (dentro de esta función deberías estar llamando
        // tud_task() y manejando los mensajes MIDI IN/OUT)
        midi_core_task();

        tight_loop_contents();  // pista al compilador de que este bucle es intencional
    }

    return 0;
}
