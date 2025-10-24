#ifndef __RFM96_H
#define __RFM96_H

#include <stdint.h>
#include <stdbool.h>

// Registradores do RFM96 (Modo LoRa)
#define RFM96_REG_FIFO                  0x00
#define RFM96_REG_OP_MODE               0x01
#define RFM96_REG_FRF_MSB               0x06
#define RFM96_REG_FRF_MID               0x07
#define RFM96_REG_FRF_LSB               0x08
#define RFM96_REG_PA_CONFIG             0x09
#define RFM96_REG_LNA                   0x0C
#define RFM96_REG_FIFO_ADDR_PTR         0x0D
#define RFM96_REG_FIFO_TX_BASE_ADDR     0x0E
#define RFM96_REG_FIFO_RX_BASE_ADDR     0x0F
#define RFM96_REG_IRQ_FLAGS             0x12
#define RFM96_REG_MODEM_CONFIG_1        0x1D
#define RFM96_REG_MODEM_CONFIG_2        0x1E
#define RFM96_REG_PAYLOAD_LENGTH        0x22
#define RFM96_REG_DIO_MAPPING_1         0x40
#define RFM96_REG_VERSION               0x42

// Modos de Operação (RegOpMode)
#define RFM96_MODE_SLEEP                0x80 // LoRa mode bit + 0x00
#define RFM96_MODE_STDBY                0x81 // LoRa mode bit + 0x01
#define RFM96_MODE_TX                   0x83 // LoRa mode bit + 0x03
#define RFM96_MODE_RX_CONTINUOUS        0x85 // LoRa mode bit + 0x05

// Flags de IRQ
#define RFM96_IRQ_TX_DONE               0x08

/**
 * @brief Inicializa o módulo RFM96.
 * 
 * @param freq Frequência de operação em MHz (ex: 915.0).
 * @return true se a inicialização foi bem-sucedida, false caso contrário.
 */
bool rfm96_init(float freq);

/**
 * @brief Envia um pacote de dados via LoRa.
 * 
 * @param data Ponteiro para o buffer de dados.
 * @param len Comprimento dos dados em bytes.
 */
void rfm96_send_packet(uint8_t *data, uint8_t len);

#endif // __RFM96_H
