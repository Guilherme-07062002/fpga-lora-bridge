#ifndef __AHT10_H
#define __AHT10_H

#include <stdbool.h>
#include <stdint.h>

// Endereço I2C do AHT10
#define AHT10_I2C_ADDR 0x38

// Comandos do AHT10
#define AHT10_CMD_INIT 0xE1
#define AHT10_CMD_TRIGGER 0xAC
#define AHT10_STATUS_BUSY 0x80
#define AHT10_STATUS_CALIBRATED 0x08

/**
 * @brief Inicializa o sensor AHT10.
 * 
 * @return true se a inicialização foi bem-sucedida, false caso contrário.
 */
bool aht10_init(void);

/**
 * @brief Solicita uma leitura de temperatura e umidade.
 * 
 * @return true se o comando foi enviado com sucesso, false caso contrário.
 */
bool aht10_trigger_measurement(void);

/**
 * @brief Lê os dados brutos de temperatura e umidade do sensor.
 * 
 * @param raw_h O ponteiro para armazenar o dado bruto de umidade.
 * @param raw_t O ponteiro para armazenar o dado bruto de temperatura.
 * @return true se a leitura foi bem-sucedida, false caso contrário.
 */
bool aht10_read_raw_data(uint32_t *raw_h, uint32_t *raw_t);

/**
 * @brief Converte o dado bruto de umidade para umidade relativa (%).
 * 
 * @param raw_h O dado bruto de umidade.
 * @return float O valor da umidade em %.
 */
float aht10_calculate_humidity(uint32_t raw_h);

/**
 * @brief Converte o dado bruto de temperatura para graus Celsius.
 * 
 * @param raw_t O dado bruto de temperatura.
 * @return float O valor da temperatura em °C.
 */
float aht10_calculate_temperature(uint32_t raw_t);

#endif // __AHT10_H
