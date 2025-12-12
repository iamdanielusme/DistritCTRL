#include "pot_driver.h"

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/time.h"
#include <stdint.h>

// ------------------------
// Configuración de hardware
// ------------------------

// ADS1115 conectado en I2C1, SDA=GP26, SCL=GP27
#define POT_I2C        i2c1
#define POT_I2C_SDA    26
#define POT_I2C_SCL    27

// Dirección típica del ADS1115 (ADDR a GND)
#define ADS1115_ADDR           0x48

// Registros del ADS1115
#define ADS1115_REG_CONVERSION 0x00
#define ADS1115_REG_CONFIG     0x01

// Tiempo de conversión aproximado a 860 SPS (~1.16 ms). Añadimos margen.
#define ADS1115_CONV_TIME_US   3000

// Config base (sin OS ni MUX):
// - PGA = ±4.096V (001 en PGA bits)
// - Modo single-shot (MODE=1)
// - Data rate = 860 SPS (DR=111)
// - Comparator desactivado (COMP_QUE = 11)
#define ADS1115_CONFIG_BASE    0x03E3

// Tabla MUX para entradas single-ended AINx vs GND
// MUX bits [14:12]:
// 100: AIN0-GND
// 101: AIN1-GND
// 110: AIN2-GND
// 111: AIN3-GND
static const uint16_t s_ads1115_mux[POT_DRIVER_NUM_CHANNELS] = {
    0x4000, // AIN0
    0x5000, // AIN1
    0x6000, // AIN2
    0x7000  // AIN3
};

// Buffer de lecturas RAW (tal como vienen del ADS1115, 16 bits signed)
static uint16_t s_pot_raw[POT_DRIVER_NUM_CHANNELS] = {0};

// Máquina de estados para hacer lecturas sin bloquear demasiado
static int             s_current_channel = -1;  // -1 = inactivo
static absolute_time_t s_conv_start_time;


// ------------------------
// Funciones internas I2C
// ------------------------

static void ads1115_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (uint8_t)(value >> 8);      // MSB
    buf[2] = (uint8_t)(value & 0xFF);    // LSB

    i2c_write_blocking(POT_I2C, ADS1115_ADDR, buf, 3, false);
}

static uint16_t ads1115_read_reg(uint8_t reg)
{
    uint8_t cmd = reg;
    uint8_t buf[2];

    // Seleccionar registro
    i2c_write_blocking(POT_I2C, ADS1115_ADDR, &cmd, 1, true);
    // Leer dos bytes
    i2c_read_blocking(POT_I2C, ADS1115_ADDR, buf, 2, false);

    uint16_t val = ((uint16_t)buf[0] << 8) | buf[1];
    return val;
}

static void ads1115_start_conversion(int channel)
{
    if (channel < 0 || channel >= POT_DRIVER_NUM_CHANNELS) return;

    // OS=1 (bit 15) para iniciar conversión single-shot
    uint16_t config = 0x8000;
    config |= s_ads1115_mux[channel];
    config |= ADS1115_CONFIG_BASE;

    ads1115_write_reg(ADS1115_REG_CONFIG, config);
}


// ------------------------
// API pública
// ------------------------

void pot_driver_init(void)
{
    // Inicializa I2C1 a 400 kHz
    i2c_init(POT_I2C, 400 * 1000);

    gpio_set_function(POT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(POT_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(POT_I2C_SDA);
    gpio_pull_up(POT_I2C_SCL);

    // Limpia el buffer y el estado
    for (int i = 0; i < POT_DRIVER_NUM_CHANNELS; i++) {
        s_pot_raw[i] = 0;
    }
    s_current_channel = -1;
    s_conv_start_time = get_absolute_time();
}

void pot_driver_update(void)
{
    absolute_time_t now = get_absolute_time();

    if (s_current_channel < 0) {
        // No hay conversión en curso → empieza por el canal 0
        s_current_channel = 0;
        s_conv_start_time = now;
        ads1115_start_conversion(s_current_channel);
        return;
    }

    // Hay una conversión en curso → ¿ya pasó el tiempo mínimo?
    int64_t dt_us = absolute_time_diff_us(s_conv_start_time, now);
    if (dt_us < ADS1115_CONV_TIME_US) {
        // Todavía no, salimos sin bloquear
        return;
    }

    // Ya pasó el tiempo → leemos el resultado del canal actual
    uint16_t raw = ads1115_read_reg(ADS1115_REG_CONVERSION);
    s_pot_raw[s_current_channel] = raw;  // guardamos tal cual

    // ¿Hay más canales por leer?
    if (s_current_channel < (POT_DRIVER_NUM_CHANNELS - 1)) {
        s_current_channel++;
        s_conv_start_time = now;
        ads1115_start_conversion(s_current_channel);
    } else {
        // Terminamos los 4 canales → volvemos a estado inactivo
        s_current_channel = -1;
    }
}

uint16_t pot_driver_get_raw(int ch)
{
    if (ch < 0 || ch >= POT_DRIVER_NUM_CHANNELS) return 0;
    return s_pot_raw[ch];
}

uint16_t pot_driver_get_12bit(int ch)
{
    if (ch < 0 || ch >= POT_DRIVER_NUM_CHANNELS) return 0;

    // El ADS1115 entrega 16 bits signed (two's complement).
    // En single-ended real debería ser >=0, pero por seguridad:
    int16_t raw_signed = (int16_t)s_pot_raw[ch];
    if (raw_signed < 0) raw_signed = 0;

    // Escala simple: de 16 bits (0..32767) a ~12 bits (0..4095)
    uint16_t raw_u = (uint16_t)raw_signed;
    return (raw_u >> 4); // quitamos 4 bits LSB → 0..4095 aprox
}
