#include "slave_comm.h"
#include "ctrl_protocol.h"
#include "button_driver.h"
#include "pot_driver.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/time.h"

// ----------------------------
// Config UART del SLAVE
// ----------------------------

// Ajusta estos pines a como cablees el UART entre slave y master
#define SLAVE_UART_ID        uart0
#define SLAVE_UART_BAUDRATE  115200
#define SLAVE_UART_TX_PIN    0   // TX del slave → RX del master
#define SLAVE_UART_RX_PIN    1   // RX del slave (por ahora no lo usamos, pero lo dejamos listo)

// Periodo de envío de frames (en microsegundos)
#define SLAVE_COMM_PERIOD_US  (5000)   // 5 ms -> 200 Hz aprox

static absolute_time_t s_last_send_time;

// ----------------------------
// Función interna: enviar un frame
// ----------------------------
static void slave_comm_send_frame(void)
{
    ctrl_payload_t pl;
    uint8_t frame[CTRL_FRAME_SIZE];

    // 1) Leer entradas
    pl.arcade_mask = button_driver_get_arcade_mask();  // bits 0..3
    pl.normal_mask = button_driver_get_normal_mask();  // bits 0..3

    for (int i = 0; i < CTRL_FRAME_NUM_POTS; i++) {
        pl.pot[i] = pot_driver_get_12bit(i); // 0..4095
    }

    // 2) Construir payload en bytes
    uint8_t payload[CTRL_FRAME_PAYLOAD_SIZE];
    int idx = 0;

    payload[idx++] = pl.arcade_mask;
    payload[idx++] = pl.normal_mask;

    for (int i = 0; i < CTRL_FRAME_NUM_POTS; i++) {
        uint16_t v = pl.pot[i];
        payload[idx++] = (uint8_t)(v >> 8);      // MSB
        payload[idx++] = (uint8_t)(v & 0xFF);    // LSB
    }

    // 3) Checksum
    uint8_t cs = ctrl_protocol_calc_checksum(payload, CTRL_FRAME_PAYLOAD_SIZE);

    // 4) Frame final: [H1][H2][payload...][cs]
    int f = 0;
    frame[f++] = CTRL_FRAME_HEADER_1;
    frame[f++] = CTRL_FRAME_HEADER_2;
    for (int i = 0; i < CTRL_FRAME_PAYLOAD_SIZE; i++) {
        frame[f++] = payload[i];
    }
    frame[f++] = cs;

    // 5) Enviar por UART (no bloquea demasiado; son pocos bytes)
    uart_write_blocking(SLAVE_UART_ID, frame, CTRL_FRAME_SIZE);
}

// ----------------------------
// API pública
// ----------------------------

void slave_comm_init(void)
{
    // Inicializa UART
    uart_init(SLAVE_UART_ID, SLAVE_UART_BAUDRATE);
    gpio_set_function(SLAVE_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(SLAVE_UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_format(SLAVE_UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(SLAVE_UART_ID, true);

    s_last_send_time = get_absolute_time();
}

void slave_comm_task(void)
{
    absolute_time_t now = get_absolute_time();
    int64_t dt_us = absolute_time_diff_us(s_last_send_time, now);

    if (dt_us < SLAVE_COMM_PERIOD_US) {
        // Todavía no toca enviar otro frame
        return;
    }

    s_last_send_time = now;

    // Importante: asumimos que en el main ya se llamaron:
    // button_driver_update();
    // pot_driver_update();
    slave_comm_send_frame();
}
