#include "pico/stdlib.h"
#include "pot_driver.h"

int main() {

    pot_init(26);   // GP26 = ADC0
    pot_set_cc(7); // Ej: CC74 (filtro), opcional

    while (1) {
        pot_update();
        sleep_ms(10);
    }
    return 0;
}
