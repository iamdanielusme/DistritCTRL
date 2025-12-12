#ifndef SLAVE_LINK_H
#define SLAVE_LINK_H

#include <stdint.h>
#include <stdbool.h>
#include "ctrl_protocol.h"

// Estado decodificado del slave
typedef struct {
    uint8_t  arcade_mask;
    uint8_t  normal_mask;
    uint16_t pot[CTRL_FRAME_NUM_POTS];

    bool     valid;           // true si hemos recibido al menos un frame válido
    uint64_t last_update_us;  // timestamp del último frame válido
} slave_state_t;

// Inicializa UART para hablar con el slave
void slave_link_init(void);

// Llamar frecuentemente en el main loop
void slave_link_task(void);

// Devuelve una copia del último estado
void slave_link_get_state(slave_state_t *out);

// Devuelve true si el slave está “vivo” (reciente)
bool slave_link_is_alive(uint32_t timeout_ms);

#endif // SLAVE_LINK_H
