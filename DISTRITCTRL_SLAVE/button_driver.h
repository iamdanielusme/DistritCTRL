#ifndef BUTTON_DRIVER_H
#define BUTTON_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

// =============================
// Mapeo de pines (SLAVE BOARD)
// =============================

// Botones tipo arcade (4)
#define BTN_ARCADE_1_PIN   2
#define BTN_ARCADE_2_PIN   3
#define BTN_ARCADE_3_PIN   4
#define BTN_ARCADE_4_PIN   5

// Botones tipo pulsador normal (4)
#define BTN_NORMAL_1_PIN   6
#define BTN_NORMAL_2_PIN   7
#define BTN_NORMAL_3_PIN   8
#define BTN_NORMAL_4_PIN   9

// Cantidades
#define BUTTON_DRIVER_NUM_ARCADE   4
#define BUTTON_DRIVER_NUM_NORMAL   4
#define BUTTON_DRIVER_NUM_TOTAL   (BUTTON_DRIVER_NUM_ARCADE + BUTTON_DRIVER_NUM_NORMAL)

// Índices lógicos para cada botón
typedef enum {
    BUTTON_ARCADE_1 = 0,
    BUTTON_ARCADE_2,
    BUTTON_ARCADE_3,
    BUTTON_ARCADE_4,

    BUTTON_NORMAL_1,
    BUTTON_NORMAL_2,
    BUTTON_NORMAL_3,
    BUTTON_NORMAL_4
} button_id_t;

// =============================
// API
// =============================

void button_driver_init(void);

/**
 * @brief Debe llamarse periódicamente (sin sleep), por ejemplo
 * desde el loop principal o un timer repetitivo.
 */
void button_driver_update(void);

/**
 * @brief Máscara de 4 bits para arcade:
 * bit0 = ARCADE_1, bit1 = ARCADE_2, etc. (1 = presionado).
 */
uint8_t button_driver_get_arcade_mask(void);

/**
 * @brief Máscara de 4 bits para normales:
 * bit0 = NORMAL_1, bit1 = NORMAL_2, etc. (1 = presionado).
 */
uint8_t button_driver_get_normal_mask(void);

/**
 * @brief Máscara de 8 bits con TODOS los botones:
 * bits 0..3 = arcade, bits 4..7 = normales.
 */
uint8_t button_driver_get_all_mask(void);

/**
 * @brief Estado de un botón concreto (true = presionado).
 */
bool button_driver_is_pressed(button_id_t id);

#endif // BUTTON_DRIVER_H
