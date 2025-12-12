/**
 * @file main.c
 * @brief Firmware principal del MASTER (Pico W) para el controlador Distrit CTRL01.
 *
 * Funciones principales:
 * - Recibir estado de botones y potenciómetros desde el SLAVE por UART.
 * - Leer 3 faders analógicos en el MASTER (ADC interno) y mapearlos a CC MIDI.
 * - Leer 2 sensores ultrasónicos HC-SR04 (alimentados a 3.3 V) y mapearlos a CC MIDI tipo "theremin".
 * - Enviar mensajes MIDI (notas + CC) hacia el computador vía USB (TinyUSB).
 * - Actualizar la pantalla OLED con el estado del step sequencer y BPM.
 */

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

#include "midi_core.h"
#include "led_ring.h"
#include "step_sequencer.h"
#include "display_oled.h"
#include "slave_link.h"
#include "ultra_driver.h"   ///< Driver para los sensores ultrasónicos

// -----------------------------------------------------------------------------
//  Configuración general y mapeos MIDI
// -----------------------------------------------------------------------------

/** @brief Rango máximo esperado de los potenciómetros que llegan del SLAVE (12 bits "recortados"). */
#define SLAVE_POT_MAX_RAW12   1600u

/** @brief Canal MIDI para los pots que vienen del SLAVE. */
#define SLAVE_POTS_MIDI_CHANNEL  0

/** @brief LED de debug en el MASTER (Pico W no tiene PICO_DEFAULT_LED_PIN "normal"). */
#define DEBUG_LED_PIN 15   // pin GPIO con LED + resistencia a GND

// ---------------- FADERS EN EL MASTER (ADC interno) ----------------

/** @brief Número de faders analógicos conectados al MASTER. */
#define NUM_FADERS 3

/** @brief Pines GPIO usados por los faders (GP26, GP27, GP28). */
static const uint8_t fader_gpio[NUM_FADERS]      = {26, 27, 28};
/** @brief Canal ADC correspondiente a cada GPIO de fader (ADC0, ADC1, ADC2). */
static const uint8_t fader_adc_input[NUM_FADERS] = {0,  1,  2};

/** @brief Números de CC MIDI asignados a cada fader del MASTER. */
static const uint8_t fader_cc[NUM_FADERS]        = {10, 11, 12};

/**
 * @brief Último valor de CC enviado para cada fader.
 *
 * Se usa para evitar enviar mensajes de control cuando el cambio es muy pequeño
 * (reduce ruido y tráfico MIDI).
 */
static uint8_t prev_fader_cc[NUM_FADERS]         = {0xFF, 0xFF, 0xFF};

/**
 * @brief Umbral mínimo de cambio (en pasos de CC) para reenviar un mensaje MIDI.
 *
 * Si la diferencia entre el valor actual y el anterior es menor que este umbral,
 * no se envía un nuevo mensaje CC.
 */
#define FADER_CC_THRESHOLD  2u   // sube a 3 si sigue muy sensible

// ---------------- ULTRASONIDOS (THEREMIN) ----------------

/** @brief Canal MIDI usado para los CC de los sensores ultrasónicos. */
#define ULTRA_MIDI_CHANNEL  0

/**
 * @brief Números de CC MIDI asignados a cada sensor ultrasónico.
 *
 * ULTRA 0  -> CC30 (ej. cutoff, pitch, etc.)
 * ULTRA 1  -> CC31 (ej. reverb, delay, etc.)
 */
static const uint8_t ultra_cc[ULTRA_NUM_SENSORS] = {30, 31};

/** @brief Último valor de CC enviado por cada sensor ultrasónico. */
static uint8_t prev_ultra_cc[ULTRA_NUM_SENSORS]  = {0xFF, 0xFF};

// -----------------------------------------------------------------------------
//  Estado de UI y mapeos de botones / pots del SLAVE
// -----------------------------------------------------------------------------

/** @brief Estructura con el estado que se muestra en la OLED. */
static controller_status_t ui_status;

/** @brief Notas MIDI asociadas a los 4 botones "arcade" del SLAVE. */
static const uint8_t arcade_notes[4] = {60, 61, 62, 63}; // C4, C#4, D4, D#4
/** @brief Notas MIDI asociadas a los 4 botones "normales" del SLAVE. */
static const uint8_t normal_notes[4] = {64, 65, 66, 67}; // E4, F4, F#4, G4

/** @brief Máscara anterior de los botones arcade (para detectar flancos). */
static uint8_t prev_arcade_mask = 0;
/** @brief Máscara anterior de los botones normales (para detectar flancos). */
static uint8_t prev_normal_mask = 0;

/** @brief Números de CC MIDI para los 4 pots conectados al SLAVE (via ADS1115). */
static const uint8_t pot_cc[4]      = {20, 21, 22, 23};
/** @brief Último valor de CC enviado para cada pot del SLAVE. */
static uint8_t       prev_pot_cc[4] = {0xFF, 0xFF, 0xFF, 0xFF};

// -----------------------------------------------------------------------------
//  Callbacks y funciones internas
// -----------------------------------------------------------------------------

/**
 * @brief Callback periódico para actualizar la UI (OLED).
 *
 * Esta función se ejecuta con un timer repetitivo (~30 FPS) independiente de la
 * lógica MIDI. Consulta:
 *  - BPM detectado a partir del clock MIDI.
 *  - Estado de reproducción del step sequencer.
 *  - Paso actual.
 *
 * Y luego actualiza el contenido de la pantalla OLED.
 *
 * @param t Puntero al timer repetitivo (no usado).
 * @return true para mantener el timer activo.
 */
static bool ui_timer_cb(repeating_timer_t *t)
{
    (void)t;

    // BPM actual calculado en midi_core a partir del clock MIDI
    ui_status.bpm = midi_core_get_bpm();

    // Usamos el BPM como “proxy” de si hay clock.
    ui_status.has_clock = (ui_status.bpm > 0);

    // ¿El step sequencer está corriendo?
    ui_status.playing = stepseq_is_running();

    // Paso actual (stepseq_get_current_step devuelve 0..15)
    uint8_t cur_step  = stepseq_get_current_step();
    ui_status.step        = (uint8_t)(cur_step + 1); // 1..16 para la pantalla
    ui_status.total_steps = 16;

    // Actualizar lo que la OLED debe mostrar
    display_set_status(&ui_status);
    display_task();   // redibuja el frame en el SH1106

    return true; // seguir repitiendo el timer
}

// -----------------------------------------------------------------------------
//  main()
// -----------------------------------------------------------------------------

/**
 * @brief Punto de entrada principal del firmware del MASTER.
 *
 * Flujo general:
 *  - Inicializa periféricos locales (LED ring, step sequencer, OLED, ADC, ultrasónicos).
 *  - Inicializa MIDI USB y enlace UART con el SLAVE.
 *  - En el bucle principal:
 *      - Atiende la pila MIDI/USB.
 *      - Atiende el enlace UART con el SLAVE.
 *      - Actualiza máquina de estados de sensores ultrasónicos.
 *      - Cada ~5 ms:
 *          - Lee faders del MASTER y envía CC.
 *          - Lee estado del SLAVE (botones + pots) y envia notas/CC.
 */
int main(void)
{
    stdio_init_all(); // aunque tengas stdio USB apagado en CMake, no estorba

    // LED de debug en GPIO normal
    const uint LED_PIN = DEBUG_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // --- Inicializar hardware propio ---
    led_ring_init();    // WS2812 (usa PIO)
    stepseq_init();     // Estado del step sequencer
    display_init();     // I2C + SH1106 (OLED)

    // Inicializar MIDI + TinyUSB (device MIDI hacia el PC)
    midi_core_init();

    // Inicializar enlace UART con el SLAVE (uart0 GP0/GP1)
    slave_link_init();

    // --- Inicializar ADC para los FADERS (misma lógica que en "hello") ---
    adc_init();
    for (int i = 0; i < NUM_FADERS; i++) {
        adc_gpio_init(fader_gpio[i]);   // GP26, GP27, GP28 como entradas ADC
    }

    // --- Inicializar sensores ultrasónicos (HC-SR04 a 3.3 V) ---
    ultra_driver_init();

    // --- Timer periódico para la OLED (solo UI, nada de USB aquí) ---
    struct repeating_timer ui_timer;
    add_repeating_timer_ms(-33, ui_timer_cb, NULL, &ui_timer); // ~30 FPS

    // Para comprobar "aliveness" del slave cada cierto rato
    absolute_time_t last_check = get_absolute_time();

    // --- Bucle principal ---
    while (true) {
        // Procesar USB + MIDI (TinyUSB + clock, etc.)
        midi_core_task();

        // Procesar bytes/frames que lleguen del SLAVE por UART
        slave_link_task();

        // Actualizar máquina de estados de los ultrasonidos (no bloquea)
        ultra_driver_update();

        // Cada ~5 ms hacemos la lógica de botones + pots + faders + ultrasónicos → MIDI
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_check, now) >= 5000) { // 5 ms
            last_check = now;

            slave_state_t st;
            slave_link_get_state(&st);

            bool alive = slave_link_is_alive(200); // datos en los últimos 200 ms

            // --------- FADERS DEL MASTER → CC MIDI (SIEMPRE) ----------
            for (int f = 0; f < NUM_FADERS; f++) {
                // Igual que en tu ejemplo "hello": seleccionar canal ADC y leer
                adc_select_input(fader_adc_input[f]);
                sleep_us(5);                      // pequeño settle time
                uint16_t raw = adc_read();        // 0..4095

                // Escalar a 0..127
                uint8_t cc = (uint8_t)(((uint32_t)raw * 127u) / 4095u);

                uint8_t prev = prev_fader_cc[f];
                uint8_t diff = (cc > prev) ? (cc - prev) : (prev - cc);

                if (prev == 0xFF || diff >= FADER_CC_THRESHOLD) {
                    midi_send_cc(0, fader_cc[f], cc);  // canal 1
                    prev_fader_cc[f] = cc;
                }
            }
            // --------- FIN FADERS MASTER -----------------------------

            // --------- ULTRASONIDOS → CC MIDI (THEREMIN) -------------
            for (int i = 0; i < ULTRA_NUM_SENSORS; i++) {
                if (!ultra_driver_is_valid(i)) {
                    continue;   // no hay medida buena todavía
                }

                float d_cm = ultra_driver_get_distance_cm(i);
                if (d_cm <= 0.0f) {
                    continue;
                }

                // Rango útil de distancia para "tocar" con la mano.
                // Ejemplo: 10 cm (muy cerca) -> valor alto, 60 cm -> valor bajo.
                const float D_MIN = 10.0f;
                const float D_MAX = 60.0f;

                if (d_cm < D_MIN) d_cm = D_MIN;
                if (d_cm > D_MAX) d_cm = D_MAX;

                // Normalizamos para que cerca = 127, lejos = 0
                float norm = (D_MAX - d_cm) / (D_MAX - D_MIN);  // 0..1
                if (norm < 0.0f) norm = 0.0f;
                if (norm > 1.0f) norm = 1.0f;

                uint8_t ccval = (uint8_t)(norm * 127.0f + 0.5f);

                uint8_t prev_cc = prev_ultra_cc[i];
                uint8_t diff    = (ccval > prev_cc) ? (ccval - prev_cc)
                                                    : (prev_cc - ccval);

                if (prev_cc == 0xFF || diff >= 1) {
                    midi_send_cc(ULTRA_MIDI_CHANNEL, ultra_cc[i], ccval);
                    prev_ultra_cc[i] = ccval;
                }
            }
            // --------- FIN ULTRASONIDOS ------------------------------

            if (alive && st.valid) {
                // --------- EDGE DETECTION: ARCADE ---------------------
                for (int i = 0; i < 4; i++) {
                    uint8_t mask = (1u << i);

                    bool prev = (prev_arcade_mask & mask) != 0;
                    bool curr = (st.arcade_mask    & mask) != 0;

                    if (curr && !prev) {
                        // Flanco de subida → NOTE ON
                        midi_send_note_on(0, arcade_notes[i], 100);
                    } else if (!curr && prev) {
                        // Flanco de bajada → NOTE OFF
                        midi_send_note_off(0, arcade_notes[i], 0);
                    }
                }

                // --------- EDGE DETECTION: NORMALES -------------------
                for (int i = 0; i < 4; i++) {
                    uint8_t mask = (1u << i);

                    bool prev = (prev_normal_mask & mask) != 0;
                    bool curr = (st.normal_mask    & mask) != 0;

                    if (curr && !prev) {
                        midi_send_note_on(0, normal_notes[i], 100);
                    } else if (!curr && prev) {
                        midi_send_note_off(0, normal_notes[i], 0);
                    }
                }

                // Actualizar estados previos de botones
                prev_arcade_mask = st.arcade_mask;
                prev_normal_mask = st.normal_mask;

                // LED de debug encendido si hay algún botón pulsado
                bool any_pressed = ((st.arcade_mask | st.normal_mask) != 0);
                gpio_put(LED_PIN, any_pressed ? 1 : 0);

                // --------- POTS DEL SLAVE → CC MIDI -------------------
                for (int i = 0; i < 4; i++) {
                    uint16_t v12 = st.pot[i];   // valor que viene del slave (0..~1600)

                    // Limitar a un máximo esperado para que dé "toda la vuelta"
                    if (v12 > SLAVE_POT_MAX_RAW12) {
                        v12 = SLAVE_POT_MAX_RAW12;
                    }

                    // Escalar 0..SLAVE_POT_MAX_RAW12 -> 0..127
                    uint8_t cc = (uint8_t)(((uint32_t)v12 * 127u) / SLAVE_POT_MAX_RAW12);

                    // Enviar solo si cambió (simple filtro)
                    if (prev_pot_cc[i] == 0xFF || cc != prev_pot_cc[i]) {
                        midi_send_cc(SLAVE_POTS_MIDI_CHANNEL, pot_cc[i], cc);
                        prev_pot_cc[i] = cc;
                    }
                }
            } else {
                // Si el slave no está vivo, apagamos LED y reseteamos máscaras
                gpio_put(LED_PIN, 0);
                prev_arcade_mask = 0;
                prev_normal_mask = 0;

                // Reset de los pots para que al volver el slave mandemos
                // de nuevo los valores correctos
                for (int i = 0; i < 4; i++) {
                    prev_pot_cc[i] = 0xFF;
                }

                // Los faders y ultrasónicos son del MASTER, se siguen actualizando
                // normalmente; no hace falta resetearlos aquí.
            }
        }

        tight_loop_contents();
    }

    return 0;
}
