// midi_core.c - Núcleo MIDI/USB para Distrit CTRL01

#include "midi_core.h"

#include "bsp/board.h"
#include "tusb.h"

// ---------------- Parpadeo de LED según estado USB ----------------

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
    // Inicializa TinyUSB en el root port definido por el BSP
    tud_init(BOARD_TUD_RHPORT);
    blink_interval_ms = BLINK_NOT_MOUNTED;
}

void midi_core_task(void)
{
    // Atiende la pila USB (enumeración, transferencias, etc.)
    tud_task();

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

// ---------------- Callbacks de TinyUSB ----------------

// Estos los llama TinyUSB automáticamente para notificar cambios de estado USB

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
