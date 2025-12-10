#ifndef FADER_DRIVER_H
#define FADER_DRIVER_H

#include <stdint.h>

typedef struct {
    uint32_t adc_channel;
} Fader;

void fader_init(Fader *f, uint32_t adc_channel);
uint16_t fader_get_raw(Fader *f);

#endif
