#ifndef ULTRA_DRIVER_H
#define ULTRA_DRIVER_H

#include <stdbool.h>

// Número de sensores ultrasónicos conectados
#define ULTRA_NUM_SENSORS  2

// Inicializa los pines TRIG/ECHO para todos los sensores
void ultra_driver_init(void);

// Debe llamarse frecuentemente (en el while(true))
// Implementa la máquina de estados sin bloquear
void ultra_driver_update(void);

// Devuelve la última distancia medida en cm para el sensor idx
// Si no hay medida válida, devuelve un valor “viejo” pero puedes
// consultar ultra_driver_is_valid() para saber si es confiable.
float ultra_driver_get_distance_cm(int idx);

// true si la última medida de ese sensor fue válida
bool ultra_driver_is_valid(int idx);

#endif // ULTRA_DRIVER_H
