#include "ultra_driver.h"

#include "pico/stdlib.h"
#include "pico/time.h"

// ------------------------
// Configuración de pines
// ------------------------

// TRIG y ECHO para cada sensor (ajusta si usas otros pines)
static const uint ULTRA_TRIG_PINS[ULTRA_NUM_SENSORS] = {18, 20};
static const uint ULTRA_ECHO_PINS[ULTRA_NUM_SENSORS] = {19, 21};

// Tiempos en microsegundos
#define US_MEAS_PERIOD_US     60000   // cada 60 ms lanzo una medida por sensor
#define US_ECHO_TIMEOUT_US    30000   // timeout de eco ~30 ms (≈ 5 m máximo)

// Máquina de estados por sensor
typedef enum {
    US_IDLE = 0,
    US_TRIG_LOW,
    US_TRIG_HIGH,
    US_WAIT_ECHO_HIGH,
    US_WAIT_ECHO_LOW
} us_state_t;

static us_state_t      s_state[ULTRA_NUM_SENSORS];
static absolute_time_t s_state_time[ULTRA_NUM_SENSORS];
static absolute_time_t s_echo_start[ULTRA_NUM_SENSORS];

static float s_distance_cm[ULTRA_NUM_SENSORS];
static bool  s_valid[ULTRA_NUM_SENSORS];

void ultra_driver_init(void)
{
    absolute_time_t now = get_absolute_time();

    for (int i = 0; i < ULTRA_NUM_SENSORS; i++) {
        uint trig = ULTRA_TRIG_PINS[i];
        uint echo = ULTRA_ECHO_PINS[i];

        gpio_init(trig);
        gpio_set_dir(trig, GPIO_OUT);
        gpio_put(trig, 0);

        gpio_init(echo);
        gpio_set_dir(echo, GPIO_IN);  // flotante, el HC-SR04 lo maneja

        s_state[i]      = US_IDLE;
        s_state_time[i] = now;
        s_distance_cm[i] = 0.0f;
        s_valid[i]       = false;
    }
}

void ultra_driver_update(void)
{
    absolute_time_t now = get_absolute_time();

    for (int i = 0; i < ULTRA_NUM_SENSORS; i++) {
        uint trig = ULTRA_TRIG_PINS[i];
        uint echo = ULTRA_ECHO_PINS[i];

        switch (s_state[i]) {
        case US_IDLE:
            // Cada cierto tiempo disparamos una nueva medida
            if (absolute_time_diff_us(s_state_time[i], now) >= US_MEAS_PERIOD_US) {
                gpio_put(trig, 0);
                s_state[i]      = US_TRIG_LOW;
                s_state_time[i] = now;
            }
            break;

        case US_TRIG_LOW:
            // TRIG a 0 al menos 2 us antes del pulso
            if (absolute_time_diff_us(s_state_time[i], now) >= 2) {
                gpio_put(trig, 1);
                s_state[i]      = US_TRIG_HIGH;
                s_state_time[i] = now;
            }
            break;

        case US_TRIG_HIGH:
            // Pulso TRIG de 10 us
            if (absolute_time_diff_us(s_state_time[i], now) >= 10) {
                gpio_put(trig, 0);
                s_state[i]      = US_WAIT_ECHO_HIGH;
                s_state_time[i] = now;
            }
            break;

        case US_WAIT_ECHO_HIGH:
            if (gpio_get(echo)) {
                // Flanco de subida del eco
                s_echo_start[i] = now;
                s_state[i]      = US_WAIT_ECHO_LOW;
                s_state_time[i] = now;
            } else if (absolute_time_diff_us(s_state_time[i], now) >= US_ECHO_TIMEOUT_US) {
                // Timeout esperando ECHO alto
                s_valid[i]      = false;
                s_state[i]      = US_IDLE;
                s_state_time[i] = now;
            }
            break;

        case US_WAIT_ECHO_LOW:
            if (!gpio_get(echo)) {
                // Flanco de bajada del eco → medimos duración
                int64_t dt_us = absolute_time_diff_us(s_echo_start[i], now);
                if (dt_us > 0 && dt_us < US_ECHO_TIMEOUT_US) {
                    // Distancia aproximada:
                    // distancia (cm) ≈ tiempo_us * 0.017 (ida y vuelta del sonido)
                    float d = (float)dt_us * 0.01715f;
                    s_distance_cm[i] = d;
                    s_valid[i]       = true;
                } else {
                    s_valid[i] = false;
                }

                s_state[i]      = US_IDLE;
                s_state_time[i] = now;
            } else if (absolute_time_diff_us(s_state_time[i], now) >= US_ECHO_TIMEOUT_US) {
                // Timeout esperando que el eco baje
                s_valid[i]      = false;
                s_state[i]      = US_IDLE;
                s_state_time[i] = now;
            }
            break;

        default:
            s_state[i]      = US_IDLE;
            s_state_time[i] = now;
            break;
        }
    }
}

float ultra_driver_get_distance_cm(int idx)
{
    if (idx < 0 || idx >= ULTRA_NUM_SENSORS) return -1.0f;
    return s_distance_cm[idx];
}

bool ultra_driver_is_valid(int idx)
{
    if (idx < 0 || idx >= ULTRA_NUM_SENSORS) return false;
    return s_valid[idx];
}
