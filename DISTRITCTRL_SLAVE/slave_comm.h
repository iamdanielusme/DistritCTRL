#ifndef SLAVE_COMM_H
#define SLAVE_COMM_H

#include <stdint.h>

// Inicializa UART y estado interno de envío
void slave_comm_init(void);

// Llamar periódicamente en el loop principal
// Se encarga de, cada cierto tiempo, leer botones + pots y enviar un frame.
void slave_comm_task(void);

#endif // SLAVE_COMM_H
