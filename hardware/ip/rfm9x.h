#ifndef __RFM9X_H
#define __RFM9X_H

#include <stdint.h>
#include <stdbool.h>

// Registradores do RFM9X (Modo LoRa)
#define REG_FIFO                  0x00
#define REG_OP_MODE               0x01
#define REG_FRF_MSB               0x06
#define REG_FRF_MID               0x07
#define REG_FRF_LSB               0x08
#define REG_PA_CONFIG             0x09
#define REG_PA_RAMP               0x0A
#define REG_OCP                   0x0B
#define REG_LNA                   0x0C
#define REG_FIFO_ADDR_PT          0x0D
#define REG_FIFO_TX_BASE_ADD     0x0E
#define REG_FIFO_RX_BASE_ADD     0x0F
#define REG_FIFO_RX_CURRENT_ADD  0x10
#define REG_IRQ_FLAGS_MASK        0x11
#define REG_IRQ_FLAGS             0x12
#define REG_RX_NB_BYTES           0x13
#define REG_RX_HEADER_CNT_MSB     0x14
#define REG_RX_HEADER_CNT_LSB     0x15
#define REG_MODEM_STAT            0x18
#define REG_MODEM_CONFIG_1        0x1D
#define REG_MODEM_CONFIG_2        0x1E
#define REG_PAYLOAD_LENGTH        0x22
#define REG_MODEM_CONFIG_3        0x26
#define REG_DIO_MAPPING_1         0x40
#define REG_VERSION               0x42

// Bits/Modos de Operação (RegOpMode)
// O datasheet costuma separar o bit de LongRange (LoRa) do valor do modo.
#define MODE_LONG_RANGE_MODE      0x80 // Bit para habilitar LoRa
#define MODE_SLEEP                0x00
#define MODE_STDBY                0x01
#define MODE_TX                   0x03
#define MODE_RX_CONTINUOUS        0x05

// Conveniência: valores com o bit LoRa já aplicado
#define MODE_SLEEP_LORA           (MODE_LONG_RANGE_MODE | MODE_SLEEP)
#define MODE_STDBY_LORA           (MODE_LONG_RANGE_MODE | MODE_STDBY)
#define MODE_TX_LORA              (MODE_LONG_RANGE_MODE | MODE_TX)
#define MODE_RXCONTINUOUS_LORA    (MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS)

// Flags de IRQ (valores e máscaras)
#define IRQ_TX_DONE               0x08
#define IRQ_RX_DONE               0x40
#define IRQ_TX_DONE_MASK          0x08
#define IRQ_RX_DONE_MASK          0x40

/**
 * @brief Inicializa o módulo RFM9X.
 * 
 * @param freq Frequência de operação em MHz (ex: 915.0).
 * @return true se a inicialização foi bem-sucedida, false caso contrário.
 */
bool rfm9x_init(float freq);

/**
 * @brief Envia um pacote de dados via LoRa.
 * 
 * @param data Ponteiro para o buffer de dados.
 * @param len Comprimento dos dados em bytes.
 */
void rfm9x_send_packet(uint8_t *data, uint8_t len);

/*
 * Declarações adicionais (implementadas em rfm9x.c / rfm9x.c)
 * As funções abaixo são usadas pelo driver presente em rfm9x.c.
 */

/* O tipo rfm9x_t é declarado opacamente aqui porque a implementação
 * define os campos concretos em outro lugar. Só precisamos do ponteiro
 * para as assinaturas das funções.
 */
typedef struct rfm9x_t rfm9x_t;

void rfm9x_select();
void rfm9x_deselect();
uint8_t rfm9x_read(uint8_t reg);
void rfm9x_write(uint8_t reg, uint8_t val);
void rfm9x_reset();
void rfm9x_setup(uint64_t frequency);
void rfm9x_send(const char *msg);
int rfm9x_receive(char *buf, int max_len);

#endif // __RFM9X_H
