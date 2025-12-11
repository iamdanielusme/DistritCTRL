// midi_core.c - Núcleo MIDI/USB para Distrit CTRL01

#include "midi_core.h"
#include "step_sequencer.h" 
#include "bsp/board.h"
#include "tusb.h"

#include "pico/time.h"
#include <stdbool.h>
#include <stdint.h>

// ---------------- Estado BPM / Clock ----------------

static uint16_t g_bpm = 0;              // BPM filtrado actual (entero)
static float    g_bpm_lp = 0.0f;        // filtro exponencial para suavizar
static uint64_t g_last_tick_us = 0;     // tiempo del último tick 0xF8 (us)
static uint64_t g_last_clock_seen_us = 0; // último momento en que vimos clock
static uint32_t g_tick_count = 0;
static uint64_t g_window_start_us = 0;

// ---------------- Parpadeo de LED según estado USB ----------------

static void midi_core_process_input(void);

enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED     = 1000,
    BLINK_SUSPENDED   = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

static void led_blinking_task(void)
{
    static uint32_t last_ms = 0;
    static bool led_state = false;

    uint32_t now = board_millis();

    if (now - last_ms < blink_interval_ms) {
        return;
    }
    last_ms = now;

    board_led_write(led_state);
    led_state = !led_state;
}

// ---------------- API pública ----------------

void midi_core_init(void)
{
    // Inicializa BSP + TinyUSB
    board_init();
    stepseq_init();
    tud_init(BOARD_TUD_RHPORT);

    blink_interval_ms    = BLINK_NOT_MOUNTED;
    g_bpm                = 0;
    g_bpm_lp             = 0.0f;
    g_last_tick_us       = 0;
    g_last_clock_seen_us = 0;
    g_tick_count         = 0;
    g_window_start_us    = 0;;
}

void midi_core_task(void)
{
    // Atiende la pila USB
    tud_task();

    // Procesa mensajes MIDI
    midi_core_process_input();

    // LED de estado USB
    led_blinking_task();
}

bool midi_core_is_mounted(void)
{
    return tud_midi_mounted();
}

// ---------------- Envío de mensajes MIDI ----------------

void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (!tud_midi_mounted()) return;

    uint8_t msg[3];
    msg[0] = 0x90 | (channel & 0x0F);
    msg[1] = note;
    msg[2] = velocity;

    tud_midi_stream_write(0, msg, 3);
}

void midi_send_note_off(uint8_t channel, uint8_t note, uint8_t velocity)
{
    if (!tud_midi_mounted()) return;

    uint8_t msg[3];
    msg[0] = 0x80 | (channel & 0x0F);
    msg[1] = note;
    msg[2] = velocity;

    tud_midi_stream_write(0, msg, 3);
}

void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value)
{
    if (!tud_midi_mounted()) return;

    uint8_t msg[3];
    msg[0] = 0xB0 | (channel & 0x0F);
    msg[1] = cc;
    msg[2] = value;

    tud_midi_stream_write(0, msg, 3);
}

// ---------------- Cálculo de BPM ----------------

// Llamar en cada 0xF8 (MIDI Clock)
static void midi_core_on_clock_tick(void)
{
    uint64_t now = time_us_64();

    // Marcamos actividad de clock
    g_last_clock_seen_us = now;

    // Primera vez: arrancamos ventana
    if (g_window_start_us == 0) {
        g_window_start_us = now;
        g_tick_count      = 0;
        return;
    }

    g_tick_count++;

    // Usamos 24 ticks = 1 negra para estimar BPM
    if (g_tick_count >= 24) {
        uint64_t elapsed_us = now - g_window_start_us;

        // Seguridad: descartar cosas absurdas
        //  30 BPM  -> ~2.000.000 us por beat
        //  300 BPM ->   200.000 us por beat
        if (elapsed_us > 200000 && elapsed_us < 2000000) {
            float inst_bpm = 60.0f * 1000000.0f / (float)elapsed_us;

            if (g_bpm_lp == 0.0f) {
                g_bpm_lp = inst_bpm;
            } else {
                // filtro exponencial un poco más lento
                g_bpm_lp = g_bpm_lp * 0.7f + inst_bpm * 0.3f;
            }

            if (g_bpm_lp < 40.0f)  g_bpm_lp = 40.0f;
            if (g_bpm_lp > 300.0f) g_bpm_lp = 300.0f;

            g_bpm = (uint16_t)(g_bpm_lp + 0.5f);
        }

        // Reiniciamos ventana para el siguiente beat
        g_tick_count      = 0;
        g_window_start_us = now;
    }
}


uint16_t midi_core_get_bpm(void)
{
    uint64_t now = time_us_64();

    // Si hace más de 1 s que no vemos clock, asumimos que no hay
    if (g_last_clock_seen_us == 0 || (now - g_last_clock_seen_us) > 1000000) {
        return 0;
    }

    return g_bpm;
}

bool midi_core_has_clock(void)
{
    uint64_t now = time_us_64();

    if (g_last_clock_seen_us == 0) {
        return false;
    }

    // Consideramos "hay clock" si el último tick fue hace menos de 500 ms
    return (now - g_last_clock_seen_us) < 500000;
}

// ---------------- Lectura y manejo de mensajes MIDI ----------------

static void midi_core_process_input(void)
{
    uint8_t packet[4];

    while (tud_midi_available()) {
        tud_midi_packet_read(packet);

        uint8_t cin    = packet[0] & 0x0F;
        (void)cin; // por ahora no lo usamos
        uint8_t status = packet[1];

        // Mensajes de tiempo real MIDI (pueden venir intercalados)
        switch (status) {
            case 0xF8: // MIDI Clock
                midi_core_on_clock_tick();
                stepseq_on_clock_tick();
                break;

            case 0xFA: // Start
            case 0xFB: // Continue -> lo tratamos igual que Start
                stepseq_on_start();
                break;

            case 0xFC: // Stop
                stepseq_on_stop();
                break;

            default:
                // Aquí podrías procesar Note On/Off, CC, etc. si lo necesitas
                break;
        }
    }
}

// ---------------- Callbacks de TinyUSB ----------------

void tud_mount_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void)
{
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    blink_interval_ms = BLINK_SUSPENDED;
}

void tud_resume_cb(void)
{
    blink_interval_ms = BLINK_MOUNTED;
}
