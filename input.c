#include "input.h"
#include "midi_uart.h"

#define MIDI_CH         0
#define FADER_CC_NUM    7     // Volume
#define BUTTON_NOTE     60    // C4

static uint8_t map_adc_to_midi(uint16_t raw) {
    uint32_t v = (raw * 127u) / 4095u;
    if (v > 127) v = 127;
    return (uint8_t)v;
}

void input_init(Button *btn, Fader *fad) {
    button_init(btn, 15);   // GPIO15
    fader_init(fad, 0);     // ADC0 (GPIO26)
    midi_uart_init();
}

void input_task(Button *btn, Fader *fad) {
    // ----- Botón -----
    if (button_was_pressed(btn)) {
        midi_send_note_on(MIDI_CH, BUTTON_NOTE, 100);
    }

    if (!button_read(btn)) {
        midi_send_note_off(MIDI_CH, BUTTON_NOTE, 0);
    }

    // ----- Fader -----
    static uint8_t last_fader = 0xFF;
    uint16_t raw = fader_get_raw(fad);
    uint8_t midi_val = map_adc_to_midi(raw);

    if (midi_val != last_fader) {
        midi_send_cc(MIDI_CH, FADER_CC_NUM, midi_val);
        last_fader = midi_val;
    }
}
