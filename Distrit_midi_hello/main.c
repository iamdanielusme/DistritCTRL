// main.c - Distrit CTRL01: botón (nota) + pot (CC7) + fader (CC10) en MIDI USB

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "bsp/board.h"
#include "tusb.h"

// ---------------- Configuración de pines ----------------

#define BTN_PIN          2      // Botón en GP2

#define POT_ADC_GPIO     26     // Pot en GP26 (ADC0)
#define POT_ADC_INPUT    0      // Canal ADC0

#define FADER_ADC_GPIO   27     // Fader en GP27 (ADC1)
#define FADER_ADC_INPUT  1      // Canal ADC1

// ---------------- Configuración MIDI ----------------

#define MIDI_CHANNEL     0      // Canal 1 (0 = ch1, 1 = ch2, etc.)
#define MIDI_NOTE        60     // Nota C4 para el botón

#define MIDI_CC_POT      7      // CC7 (volumen, por ejemplo) para el pot
#define MIDI_CC_FADER    10     // CC10 (pan, por ejemplo) para el fader

// ---------------- Blink según estado USB --------------

enum {
    BLINK_NOT_MOUNTED = 250,
    BLINK_MOUNTED     = 1000,
    BLINK_SUSPENDED   = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

// Prototipos
static void led_blinking_task(void);
static void button_midi_task(void);
static void pot_midi_task(void);
static void fader_midi_task(void);

// ---------------------- main --------------------------

int main(void)
{
    // Inicializa hardware base de la placa
    board_init();
    stdio_init_all();

    // Botón: entrada con pull-up en GP2
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    // ADC para pot y fader
    adc_init();
    adc_gpio_init(POT_ADC_GPIO);     // GP26 -> ADC0
    adc_gpio_init(FADER_ADC_GPIO);   // GP27 -> ADC1

    // Inicializar USB dispositivo (TinyUSB)
    tud_init(BOARD_TUD_RHPORT);

    while (1) {
        // Atiende pila USB
        tud_task();

        // LED según estado USB
        led_blinking_task();

        // MIDI desde botón, pot y fader
        button_midi_task();
        pot_midi_task();
        fader_midi_task();
    }

    return 0;
}

// ---------------- Callbacks TinyUSB --------------------

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

// ---------------- LED blinking task -------------------

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

// ---------------- Botón → Nota MIDI -------------------

static void button_midi_task(void)
{
    static bool btn_prev = true;  // por pull-up, "sin oprimir" = 1
    bool btn_now = gpio_get(BTN_PIN);

    if (!tud_midi_mounted()) {
        btn_prev = btn_now;
        return;
    }

    // Flanco 1 -> 0 (oprimido)
    if (btn_prev && !btn_now) {
        uint8_t msg_on[3];
        msg_on[0] = 0x90 | (MIDI_CHANNEL & 0x0F);  // Note On
        msg_on[1] = MIDI_NOTE;
        msg_on[2] = 100;

        tud_midi_stream_write(0, msg_on, 3);
    }

    // Flanco 0 -> 1 (liberado)
    if (!btn_prev && btn_now) {
        uint8_t msg_off[3];
        msg_off[0] = 0x80 | (MIDI_CHANNEL & 0x0F); // Note Off
        msg_off[1] = MIDI_NOTE;
        msg_off[2] = 0;

        tud_midi_stream_write(0, msg_off, 3);
    }

    btn_prev = btn_now;
}

// --------------- Pot → Control Change MIDI ------------

static void pot_midi_task(void)
{
    static uint8_t last_cc = 255;
    static uint32_t last_ms = 0;

    uint32_t now = board_millis();

    // Leer pot cada ~10 ms
    if (now - last_ms < 10) {
        return;
    }
    last_ms = now;

    if (!tud_midi_mounted()) {
        return;
    }

    // Seleccionar ADC0 (GP26) y leer
    adc_select_input(POT_ADC_INPUT);
    uint16_t raw = adc_read();   // 0..4095

    // Escalar a 0..127
    uint8_t cc = (uint8_t)((raw * 127) / 4095);

    if (last_cc == 255) {
        last_cc = cc;
    }

    // Enviar solo si cambia (para no spamear)
    if (abs((int)cc - (int)last_cc) >= 1) {
        uint8_t msg[3];
        msg[0] = 0xB0 | (MIDI_CHANNEL & 0x0F);  // CC en canal 1
        msg[1] = MIDI_CC_POT;                  // número de CC (7)
        msg[2] = cc;                           // valor 0..127

        tud_midi_stream_write(0, msg, 3);
        last_cc = cc;
    }
}

// --------------- Fader → Control Change MIDI ----------

static void fader_midi_task(void)
{
    static uint8_t last_cc = 255;
    static uint32_t last_ms = 0;

    uint32_t now = board_millis();

    // Leer fader cada ~10 ms
    if (now - last_ms < 10) {
        return;
    }
    last_ms = now;

    if (!tud_midi_mounted()) {
        return;
    }

    // Seleccionar ADC1 (GP27) y leer
    adc_select_input(FADER_ADC_INPUT);
    uint16_t raw = adc_read();   // 0..4095

    // Escalar a 0..127
    uint8_t cc = (uint8_t)((raw * 127) / 4095);

    if (last_cc == 255) {
        last_cc = cc;
    }

    // Enviar solo si cambia
    if (abs((int)cc - (int)last_cc) >= 1) {
        uint8_t msg[3];
        msg[0] = 0xB0 | (MIDI_CHANNEL & 0x0F);  // CC en canal 1
        msg[1] = MIDI_CC_FADER;                // número de CC (10)
        msg[2] = cc;                           // valor 0..127

        tud_midi_stream_write(0, msg, 3);
        last_cc = cc;
    }
}
