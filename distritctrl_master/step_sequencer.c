// step_sequencer.c
#include "step_sequencer.h"
#include "led_ring.h"

// Número de pasos que queremos mostrar en el anillo
#define STEPSEQ_NUM_STEPS      16
// Ticks de MIDI Clock por paso (24 ticks por negra, 4 negras por compás, 16 pasos)
#define STEPSEQ_TICKS_PER_STEP 6

static uint8_t current_step = 0;  // 0..15
static uint8_t tick_count   = 0;
static bool    running      = false;

// ======================
// Funciones internas
// ======================
static void stepseq_update_ring(void)
{
    // Apaga todos los LEDs y enciende sólo el step actual en verde
    led_ring_fill(0, 0, 0);  // apaga todos

    if (current_step < STEPSEQ_NUM_STEPS) {
        led_ring_set_pixel(current_step, 0, 60, 0);  // verde
    }

    led_ring_show();
}

// ======================
// API pública
// ======================

void stepseq_init(void)
{
    current_step = 0;
    tick_count   = 0;
    running      = false;
    led_ring_clear();
}

void stepseq_on_start(void)
{
    running      = true;
    current_step = 0;
    tick_count   = 0;
    stepseq_update_ring();
}

void stepseq_on_stop(void)
{
    running = false;
    led_ring_clear();
}

void stepseq_on_clock_tick(void)
{
    if (!running) return;

    tick_count++;
    if (tick_count >= STEPSEQ_TICKS_PER_STEP) {
        tick_count = 0;

        // Avanza un step
        current_step++;
        if (current_step >= STEPSEQ_NUM_STEPS) {
            current_step = 0;
        }

        stepseq_update_ring();
    }
}

// Getters para la UI (OLED, etc.)

bool stepseq_is_running(void)
{
    return running;
}

uint8_t stepseq_get_current_step(void)
{
    // 0..STEPSEQ_NUM_STEPS-1 (para la OLED le puedes sumar +1 si quieres 1..16)
    return current_step;
}
