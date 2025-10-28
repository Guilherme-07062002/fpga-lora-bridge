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
#define RFM96_REG_PA_RAMP               0x0A
#define RFM96_REG_OCP                   0x0B
#define RFM96_REG_LNA                   0x0C
#define RFM96_REG_FIFO_ADDR_PTR         0x0D
#define RFM96_REG_FIFO_TX_BASE_ADDR     0x0E
#define RFM96_REG_FIFO_RX_BASE_ADDR     0x0F
#define RFM96_REG_FIFO_RX_CURRENT_ADDR  0x10
#define RFM96_REG_IRQ_FLAGS_MASK        0x11
#define RFM96_REG_IRQ_FLAGS             0x12
#define RFM96_REG_RX_NB_BYTES           0x13
#define RFM96_REG_RX_HEADER_CNT_MSB     0x14
#define RFM96_REG_RX_HEADER_CNT_LSB     0x15
#define RFM96_REG_MODEM_STAT            0x18
#define RFM96_REG_MODEM_CONFIG_1        0x1D
#define RFM96_REG_MODEM_CONFIG_2        0x1E
#define RFM96_REG_PAYLOAD_LENGTH        0x22
#define RFM96_REG_MODEM_CONFIG_3        0x26
#define RFM96_REG_DIO_MAPPING_1         0x40
#define RFM96_REG_VERSION               0x42

// Bits/Modos de Operação (RegOpMode)
// O datasheet costuma separar o bit de LongRange (LoRa) do valor do modo.
#define RFM96_MODE_LONG_RANGE_MODE      0x80 // Bit para habilitar LoRa
#define RFM96_MODE_SLEEP                0x00
#define RFM96_MODE_STDBY                0x01
#define RFM96_MODE_TX                   0x03
#define RFM96_MODE_RX_CONTINUOUS        0x05

// Conveniência: valores com o bit LoRa já aplicado
#define RFM96_MODE_SLEEP_LORA           (RFM96_MODE_LONG_RANGE_MODE | RFM96_MODE_SLEEP)
#define RFM96_MODE_STDBY_LORA           (RFM96_MODE_LONG_RANGE_MODE | RFM96_MODE_STDBY)
#define RFM96_MODE_TX_LORA              (RFM96_MODE_LONG_RANGE_MODE | RFM96_MODE_TX)
#define RFM96_MODE_RXCONTINUOUS_LORA    (RFM96_MODE_LONG_RANGE_MODE | RFM96_MODE_RX_CONTINUOUS)

// Flags de IRQ (valores e máscaras)
#define RFM96_IRQ_TX_DONE               0x08
#define RFM96_IRQ_RX_DONE               0x40
#define RFM96_IRQ_TX_DONE_MASK          0x08
#define RFM96_IRQ_RX_DONE_MASK          0x40

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

/*
 * Declarações adicionais (implementadas em rfm96.c / rfm9x.c)
 * As funções abaixo são usadas pelo driver presente em rfm96.c.
 */

/* O tipo rfm9x_t é declarado opacamente aqui porque a implementação
 * define os campos concretos em outro lugar. Só precisamos do ponteiro
 * para as assinaturas das funções.
 */
typedef struct rfm9x_t rfm9x_t;

void rfm9x_select(rfm9x_t *r);
void rfm9x_deselect(rfm9x_t *r);
uint8_t rfm9x_read(rfm9x_t *r, uint8_t reg);
void rfm9x_write(rfm9x_t *r, uint8_t reg, uint8_t val);
void rfm9x_reset(rfm9x_t *r);
void rfm9x_setup(rfm9x_t *r, uint64_t frequency);
void rfm9x_send(rfm9x_t *r, const char *msg);
int rfm9x_receive(rfm9x_t *r, char *buf, int max_len);

#endif // __RFM96_H
