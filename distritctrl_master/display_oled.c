#include "display_oled.h"
#include "sh1106.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define I2C_PORT   i2c0
#define I2C_SDA    4
#define I2C_SCL    5

#define FONT_WIDTH    5
#define FONT_HEIGHT   7
#define FONT_SPACING  1

// =========================
// Fuente 5x7 muy básica
// =========================

typedef struct {
    char ch;
    uint8_t rows[FONT_HEIGHT]; // cada fila usa 5 bits (bit4 = columna izquierda)
} glyph_t;

// Sólo los caracteres que vamos a usar: dígitos, espacio, ':', y letras necesarias
static const glyph_t font_glyphs[] = {
    // dígitos 0-9
    { '0', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
    { '3', { 0x0E, 0x11, 0x01, 0x06, 0x01, 0x11, 0x0E } },
    { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
    { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
    { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
    { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
    { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
    { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },

    // espacio
    { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },

    // :
    { ':', { 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 } },

    // letras que necesitamos: A,B,C,D,K,L,M,O,P,R,S,T,Y
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
    { 'I', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F } },
    { 'K', { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
    { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
};

static const glyph_t* find_glyph(char c) {
    size_t n = sizeof(font_glyphs) / sizeof(font_glyphs[0]);
    for (size_t i = 0; i < n; i++) {
        if (font_glyphs[i].ch == c) return &font_glyphs[i];
    }
    return NULL;
}

static void draw_char(int x, int y, char c) {
    const glyph_t *g = find_glyph(c);
    if (!g) return;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_bits = g->rows[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            int bit = 4 - col; // bit4 = columna izquierda
            if (row_bits & (1 << bit)) {
                sh1106_draw_pixel((uint8_t)(x + col),
                                  (uint8_t)(y + row),
                                  true);
            }
        }
    }
}

static void draw_text(int x, int y, const char *text) {
    int cur_x = x;
    while (*text) {
        if (*text == '\n') {
            y += FONT_HEIGHT + 1;
            cur_x = x;
        } else {
            draw_char(cur_x, y, *text);
            cur_x += FONT_WIDTH + FONT_SPACING;
        }
        text++;
    }
}

// =========================
// Primitivas gráficas
// =========================

static void draw_line(int x0, int y0, int x1, int y1, bool color) {
    int dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int sx = (x0 < x1) ? 1 : -1;
    int dy = (y1 > y0) ? (y0 - y1) : (y1 - y0);
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if (x0 >= 0 && x0 < SH1106_WIDTH &&
            y0 >= 0 && y0 < SH1106_HEIGHT) {
            sh1106_draw_pixel((uint8_t)x0, (uint8_t)y0, color);
        }
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_rect(int x, int y, int w, int h, bool color) {
    draw_line(x, y, x + w - 1, y, color);
    draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    draw_line(x, y, x, y + h - 1, color);
    draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

static void fill_rect(int x, int y, int w, int h, bool color) {
    for (int yy = y; yy < y + h; yy++) {
        draw_line(x, yy, x + w - 1, yy, color);
    }
}


// Barra de 16 pasos, con el actual resaltado
static void draw_steps_bar(uint8_t current_step, uint8_t total_steps) {
    if (total_steps == 0) return;
    int x0 = 4;
    int y0 = SH1106_HEIGHT - 12;
    int step_w = 4;
    int step_gap = 3;

    for (uint8_t i = 0; i < total_steps; i++) {
        int x = x0 + i * (step_w + step_gap);
        if (x + step_w >= SH1106_WIDTH - 2) break;

        bool active = ((i + 1) == current_step);
        if (active) {
            fill_rect(x, y0, step_w, 8, true);
        } else {
            draw_rect(x, y0, step_w, 8, true);
        }
    }
}

static void draw_clock_icon(bool has_clock) {
    int cx = 10;
    int cy = 18;
    int r  = 6;


    if (has_clock) {
        // Agujas simples
        draw_line(cx, cy, cx, cy - 3, true);
        draw_line(cx, cy, cx + 3, cy, true);
    }
}

static void draw_playstop_icon(bool playing) {
    int box_w = 10;
    int box_h = 10;
    int x = SH1106_WIDTH - box_w - 4;
    int y = 2;

    draw_rect(x, y, box_w, box_h, true);

    if (playing) {
        // Triángulo "play"
        draw_line(x + 2,     y + 2,     x + 2,         y + box_h - 3, true);
        draw_line(x + 2,     y + 2,     x + box_w - 3, y + box_h / 2, true);
        draw_line(x + 2,     y + box_h - 3, x + box_w - 3, y + box_h / 2, true);
    } else {
        // Cuadrado "stop"
        fill_rect(x + 3, y + 3, box_w - 6, box_h - 6, true);
    }
}

// =========================
// Estado global de display
// =========================

static controller_status_t g_status = {
    .bpm         = 120,
    .has_clock   = false,
    .playing     = false,
    .step        = 1,
    .total_steps = 16
};

// =========================
// API pública
// =========================

void display_init(void) {
    // Por si no se ha hecho antes desde main:
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    sh1106_init(I2C_PORT, 0x3C);
    sh1106_clear();
    sh1106_update();
}

void display_set_status(const controller_status_t *st) {
    if (st) {
        g_status = *st;
    }
}

void display_task(void) {
    char bpm_text[8];

    sh1106_clear();

    // Marco general
    draw_rect(0, 0, SH1106_WIDTH, SH1106_HEIGHT, true);

    // Título arriba izquierda: "CTRL01"
    draw_text(4, 2, "DISTRIT CTRL01");

    // Indicador PLAY / STOP arriba derecha
    draw_playstop_icon(g_status.playing);

    // Texto BPM
    snprintf(bpm_text, sizeof(bpm_text), "%3u", (unsigned)g_status.bpm);
    draw_text(4, 18, "BPM");
    draw_text(4 + (FONT_WIDTH + FONT_SPACING) * 4, 18, bpm_text);

    // Indicador de clock (CLK)
    draw_text(4, 32, "CLK");
    draw_clock_icon(g_status.has_clock);

    // Barra de pasos abajo
    draw_steps_bar(g_status.step, g_status.total_steps);

    sh1106_update();
}
