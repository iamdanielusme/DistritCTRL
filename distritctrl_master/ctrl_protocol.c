#include "ctrl_protocol.h"

uint8_t ctrl_protocol_calc_checksum(const uint8_t *payload, uint8_t len)
{
    uint16_t sum = 0;
    for (uint8_t i = 0; i < len; i++) {
        sum += payload[i];
    }
    return (uint8_t)(sum & 0xFF);
}
