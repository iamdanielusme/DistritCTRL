#ifndef CTRL_PROTOCOL_H
#define CTRL_PROTOCOL_H

#include <stdint.h>

// Bytes de cabecera del frame
#define CTRL_FRAME_HEADER_1  0xAA
#define CTRL_FRAME_HEADER_2  0x55

// Número de pots que mandamos (4 canales del ADS1115)
#define CTRL_FRAME_NUM_POTS  4

// Payload fijo:
//  arcade_mask: bits 0..3 = 4 botones arcade
//  normal_mask: bits 0..3 = 4 botones normales
//  pot[i]: valor 12 bits (0..4095), enviado como 2 bytes (MSB, LSB)
typedef struct {
    uint8_t  arcade_mask;
    uint8_t  normal_mask;
    uint16_t pot[CTRL_FRAME_NUM_POTS];
} ctrl_payload_t;

// Tamaños de payload y frame
#define CTRL_FRAME_PAYLOAD_SIZE  (2 + CTRL_FRAME_NUM_POTS * 2)  // 2 bytes de máscaras + 8 de pots
#define CTRL_FRAME_SIZE          (2 + CTRL_FRAME_PAYLOAD_SIZE + 1) // 2 header + payload + checksum

// Calcula checksum 8 bits (suma de todos los bytes de payload)
uint8_t ctrl_protocol_calc_checksum(const uint8_t *payload, uint8_t len);

#endif // CTRL_PROTOCOL_H
