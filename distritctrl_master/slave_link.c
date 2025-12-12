#include "slave_link.h"
#include "ctrl_protocol.h"

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/time.h"

// ----------------------------
// Config UART del MASTER
// ----------------------------

// Deben coincidir baudrate y formato con el slave.
// OJO: Pin de RX del master debe ir al TX del slave.
#define MASTER_UART_ID        uart0
#define MASTER_UART_BAUDRATE  115200
#define MASTER_UART_TX_PIN    0   // opcional, por si luego queremos enviar algo al slave
#define MASTER_UART_RX_PIN    1   // RX del master ← TX del slave

// Máquina de estados del parser
typedef enum {
    RX_STATE_WAIT_H1 = 0,
    RX_STATE_WAIT_H2,
    RX_STATE_PAYLOAD,
    RX_STATE_CHECKSUM
} rx_state_t;

static rx_state_t s_rx_state = RX_STATE_WAIT_H1;
static uint8_t    s_payload_buf[CTRL_FRAME_PAYLOAD_SIZE];
static uint8_t    s_payload_pos = 0;

static slave_state_t s_slave_state = {0};

// ----------------------------
// Función interna: procesar un payload completo
// ----------------------------
static void slave_link_on_payload_complete(const uint8_t *payload)
{
    // Verificar checksum se hace afuera, aquí lo asumimos válido.

    // Decodificar en la estructura
    slave_state_t st = s_slave_state; // empezamos de la actual

    int idx = 0;
    st.arcade_mask = payload[idx++];
    st.normal_mask = payload[idx++];

    for (int i = 0; i < CTRL_FRAME_NUM_POTS; i++) {
        uint16_t hi = payload[idx++];
        uint16_t lo = payload[idx++];
        st.pot[i] = (uint16_t)((hi << 8) | lo);
    }

    st.valid          = true;
    st.last_update_us = time_us_64();

    s_slave_state = st;
}

// ----------------------------
// API pública
// ----------------------------

void slave_link_init(void)
{
    uart_init(MASTER_UART_ID, MASTER_UART_BAUDRATE);

    gpio_set_function(MASTER_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(MASTER_UART_RX_PIN, GPIO_FUNC_UART);

    uart_set_format(MASTER_UART_ID, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(MASTER_UART_ID, true);

    s_rx_state   = RX_STATE_WAIT_H1;
    s_payload_pos = 0;

    s_slave_state.valid = false;
    s_slave_state.last_update_us = 0;
}

void slave_link_task(void)
{
    while (uart_is_readable(MASTER_UART_ID)) {
        uint8_t b = (uint8_t)uart_getc(MASTER_UART_ID);

        switch (s_rx_state) {
        case RX_STATE_WAIT_H1:
            if (b == CTRL_FRAME_HEADER_1) {
                s_rx_state   = RX_STATE_WAIT_H2;
            }
            break;

        case RX_STATE_WAIT_H2:
            if (b == CTRL_FRAME_HEADER_2) {
                s_rx_state   = RX_STATE_PAYLOAD;
                s_payload_pos = 0;
            } else {
                // No era el segundo header → volver a buscar H1
                s_rx_state = RX_STATE_WAIT_H1;
            }
            break;

        case RX_STATE_PAYLOAD:
            s_payload_buf[s_payload_pos++] = b;
            if (s_payload_pos >= CTRL_FRAME_PAYLOAD_SIZE) {
                s_rx_state = RX_STATE_CHECKSUM;
            }
            break;

        case RX_STATE_CHECKSUM:
        {
            uint8_t cs_rx = b;
            uint8_t cs_ok = ctrl_protocol_calc_checksum(s_payload_buf,
                                                       CTRL_FRAME_PAYLOAD_SIZE);
            if (cs_rx == cs_ok) {
                // Frame válido
                slave_link_on_payload_complete(s_payload_buf);
            }
            // En cualquier caso, reiniciar parser
            s_rx_state   = RX_STATE_WAIT_H1;
            s_payload_pos = 0;
        }
        break;

        default:
            s_rx_state   = RX_STATE_WAIT_H1;
            s_payload_pos = 0;
            break;
        }
    }
}

void slave_link_get_state(slave_state_t *out)
{
    if (!out) return;
    *out = s_slave_state;
}

bool slave_link_is_alive(uint32_t timeout_ms)
{
    if (!s_slave_state.valid) return false;

    uint64_t now = time_us_64();
    uint64_t dt  = now - s_slave_state.last_update_us;

    return (dt <= (uint64_t)timeout_ms * 1000ULL);
}
