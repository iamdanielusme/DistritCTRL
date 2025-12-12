#include "pico/stdlib.h"
#include "pico/time.h"

#include "button_driver.h"
#include "pot_driver.h"
#include "slave_comm.h"

#include <stdio.h>

int main(void)
{
    // Serial por USB del SLAVE
    stdio_init_all();

    // Inicializar drivers locales
    button_driver_init();
    pot_driver_init();     // ADS1115 en I2C1 (GP26 = SDA, GP27 = SCL)
    slave_comm_init();     // Envío de frames por UART al master

    // LED onboard para ver actividad (botones o pots)
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Timer para que el debug no sature el USB
    absolute_time_t last_dbg = get_absolute_time();

    // Últimos valores de pot para detectar movimiento
    uint16_t last_pot[POT_DRIVER_NUM_CHANNELS] = {0, 0, 0, 0};

    while (true) {
        // Actualizar estado de botones (con antirrebote)
        button_driver_update();

        // Actualizar potenciómetros (ADS1115, lectura no bloqueante)
        pot_driver_update();

        // Seguir mandando frames por UART al master
        slave_comm_task();

        // Leer máscaras actuales de botones
        uint8_t arcade = button_driver_get_arcade_mask();
        uint8_t normal = button_driver_get_normal_mask();

        bool any_pressed = ((arcade | normal) != 0);
        bool pot_moved   = false;

        // Cada 100 ms imprimimos el estado por USB y revisamos cambios de pots
        absolute_time_t now = get_absolute_time();
        if (absolute_time_diff_us(last_dbg, now) >= 100000) { // 100 ms
            last_dbg = now;

            // Leer los 4 potenciómetros ya normalizados a 12 bits
            uint16_t p[4];
            p[0] = pot_driver_get_12bit(0);
            p[1] = pot_driver_get_12bit(1);
            p[2] = pot_driver_get_12bit(2);
            p[3] = pot_driver_get_12bit(3);

            // Detectar si algún pot cambió "suficiente"
            const uint16_t THRESH = 10;  // umbral de cambio
            for (int i = 0; i < POT_DRIVER_NUM_CHANNELS; i++) {
                uint16_t prev = last_pot[i];
                uint16_t nowv = p[i];
                uint16_t diff = (prev > nowv) ? (prev - nowv) : (nowv - prev);
                if (diff > THRESH) {
                    pot_moved = true;
                }
                last_pot[i] = nowv;
            }

            for (int i = 0; i < 4; i++) {
            uint16_t v = pot_driver_get_12bit(i);
            printf("CH%d = %4u  ", i, v);
            }
printf("\r\n");

            // LED ON si hay botones o movimiento de cualquier pot
            bool any_activity = any_pressed || pot_moved;
            gpio_put(LED_PIN, any_activity ? 1 : 0);

            printf("Arcade=0x%02X  Normal=0x%02X  "
                   "P0=%4u  P1=%4u  P2=%4u  P3=%4u\r\n",
                   arcade, normal,
                   p[0], p[1], p[2], p[3]);
        }

        tight_loop_contents();
    }

    return 0;
}
