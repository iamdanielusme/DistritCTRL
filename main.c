#include "pico/stdlib.h"
#include "input.h"

int main() {
    stdio_init_all();

    Button btn;
    Fader  fad;

    input_init(&btn, &fad);

    while (true) {
        input_task(&btn, &fad);
        sleep_ms(5);
    }
}
