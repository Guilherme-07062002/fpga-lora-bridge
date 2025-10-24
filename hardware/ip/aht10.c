#include "aht10.h"
#include <stdio.h>
#include <sleep.h>
#include <generated/csr.h>

// Funções auxiliares para I2C (dependem da implementação do SoC LiteX)
// O periférico I2C do LiteX é controlado por CSRs.

static void i2c_write(uint8_t addr, uint8_t* data, uint8_t len, bool stop) {
    i2c0_w_write( (addr << 1) | 0x00 ); // Endereço + bit de escrita
    for (int i = 0; i < len; i++) {
        i2c0_w_write(data[i] | ( (i == len - 1 && stop) ? I2C_STOP : 0) );
    }
}

static void i2c_read(uint8_t addr, uint8_t* data, uint8_t len, bool stop) {
    i2c0_w_write( (addr << 1) | 0x01 ); // Endereço + bit de leitura
    for (int i = 0; i < len; i++) {
        data[i] = i2c0_r_read() & 0xff;
        // O último byte lido deve ser acompanhado de NACK e STOP
        if (i == len - 1) {
            i2c0_w_write(I2C_NACK | (stop ? I2C_STOP : 0));
        } else {
            i2c0_w_write(0); // Continua a leitura
        }
    }
}

bool aht10_init(void) {
    uint8_t cmd[] = {AHT10_CMD_INIT, 0x08, 0x00};
    i2c_write(AHT10_I2C_ADDR, cmd, sizeof(cmd), true);
    msleep(100); // Aguarda a inicialização

    uint8_t status;
    i2c_read(AHT10_I2C_ADDR, &status, 1, true);

    return (status & AHT10_STATUS_CALIBRATED) != 0;
}

bool aht10_trigger_measurement(void) {
    uint8_t cmd[] = {AHT10_CMD_TRIGGER, 0x33, 0x00};
    i2c_write(AHT10_I2C_ADDR, cmd, sizeof(cmd), true);
    return true;
}

bool aht10_read_raw_data(uint32_t *raw_h, uint32_t *raw_t) {
    uint8_t data[6];
    
    // Aguarda o fim da medição
    uint8_t status;
    do {
        i2c_read(AHT10_I2C_ADDR, &status, 1, true);
        msleep(10);
    } while ((status & AHT10_STATUS_BUSY) != 0);

    // Lê os 6 bytes de dados
    i2c_read(AHT10_I2C_ADDR, data, 6, true);

    *raw_h = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    *raw_t = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    return true;
}

float aht10_calculate_humidity(uint32_t raw_h) {
    return ((float)raw_h / 1048576.0) * 100.0;
}

float aht10_calculate_temperature(uint32_t raw_t) {
    return ((float)raw_t / 1048576.0) * 200.0 - 50.0;
}
