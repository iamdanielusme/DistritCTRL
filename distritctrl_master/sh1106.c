#include "sh1106.h"
#include "pico/stdlib.h"
#include <string.h>     // memset, memcpy

// =====================
// Estado interno driver
// =====================
static i2c_inst_t *sh_i2c = NULL;
static uint8_t sh_addr = 0x3C;  // dirección típica 0x3C
static uint8_t sh_buffer[SH1106_WIDTH * SH1106_HEIGHT / 8];

// =====================
// Funciones internas
// =====================

static void sh_send_command(uint8_t cmd) {
    uint8_t buf[2];
    buf[0] = 0x00;   // control byte: 0x00 = command
    buf[1] = cmd;
    i2c_write_blocking(sh_i2c, sh_addr, buf, 2, false);
}

static void sh_send_command_list(const uint8_t *cmds, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sh_send_command(cmds[i]);
    }
}

static void sh_send_data(const uint8_t *data, size_t len) {
    uint8_t buf[1 + SH1106_WIDTH];  // 1 control byte + hasta 128 datos
    buf[0] = 0x40;  // control byte: 0x40 = data

    while (len > 0) {
        size_t chunk = len;
        if (chunk > SH1106_WIDTH) {
            chunk = SH1106_WIDTH;
        }
        memcpy(&buf[1], data, chunk);
        i2c_write_blocking(sh_i2c, sh_addr, buf, chunk + 1, false);
        data += chunk;
        len  -= chunk;
    }
}

// =====================
// API pública
// =====================

void sh1106_init(i2c_inst_t *i2c, uint8_t addr) {
    sh_i2c = i2c;
    sh_addr = addr;

    const uint8_t init_cmds[] = {
        0xAE,       // Display OFF
        0xD5, 0x80, // Clock divide
        0xA8, 0x3F, // Multiplex (1/64)
        0xD3, 0x00, // Display offset = 0
        0x40,       // Start line = 0
        0xAD, 0x8B, // DC-DC ON
        0xA1,       // Segment remap
        0xC8,       // COM scan direction remap
        0xDA, 0x12, // COM pins
        0x81, 0x80, // Contraste
        0xD9, 0x1F, // Pre-charge
        0xDB, 0x40, // VCOM detect
        0xA4,       // Resume to RAM
        0xA6,       // Normal display
        0xAF        // Display ON
    };

    sh_send_command_list(init_cmds, sizeof(init_cmds));

    sh1106_clear();
    sh1106_update();
}

void sh1106_clear(void) {
    memset(sh_buffer, 0x00, sizeof(sh_buffer));
}

void sh1106_draw_pixel(uint8_t x, uint8_t y, bool color) {
    if (x >= SH1106_WIDTH || y >= SH1106_HEIGHT) {
        return; // fuera de rango
    }

    uint16_t page  = (uint16_t)(y >> 3);         // y / 8
    uint8_t  bit   = (uint8_t)(1u << (y & 0x07)); // y % 8
    uint16_t index = (uint16_t)(page * SH1106_WIDTH + x);

    if (color) {
        sh_buffer[index] |= bit;
    } else {
        sh_buffer[index] &= (uint8_t)~bit;
    }
}

void sh1106_update(void) {
    for (uint8_t page = 0; page < (SH1106_HEIGHT / 8); page++) {
        sh_send_command(0xB0 | page); // Page address (0xB0..0xB7)

        // Columna inicial (muchos módulos usan offset 2)
        sh_send_command(0x02);        // lower column
        sh_send_command(0x10);        // higher column

        const uint8_t *ptr = &sh_buffer[page * SH1106_WIDTH];
        sh_send_data(ptr, SH1106_WIDTH);
    }
}
