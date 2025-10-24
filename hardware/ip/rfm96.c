#include "rfm96.h"
#include <stdio.h>
#include <generated/csr.h>

// Funções de baixo nível para comunicação SPI com o RFM96
// Usa o periférico SPIMaster do LiteX

static uint8_t spi_transfer(uint8_t val) {
    // O SPIMaster do LiteX tem um registrador `mosi` e um `miso`.
    // A escrita em `mosi` inicia a transferência.
    // O bit `done` no registrador de status indica o fim.
    lora_mosi_write(val);
    while (!(lora_status_read() & 1)); // Aguarda o bit 'done'
    return lora_miso_read();
}

static void rfm96_write_reg(uint8_t reg, uint8_t val) {
    lora_cs_write(1); // Ativa CS
    spi_transfer(reg | 0x80); // Bit de escrita
    spi_transfer(val);
    lora_cs_write(0); // Desativa CS
}

static uint8_t rfm96_read_reg(uint8_t reg) {
    uint8_t val;
    lora_cs_write(1); // Ativa CS
    spi_transfer(reg & 0x7F); // Bit de leitura
    val = spi_transfer(0x00);
    lora_cs_write(0); // Desativa CS
    return val;
}

void rfm96_set_mode(uint8_t mode) {
    rfm96_write_reg(RFM96_REG_OP_MODE, mode);
}

void rfm96_set_frequency(float freq) {
    uint64_t frf = (uint64_t)((freq * 1e6) / 32.0); // 32 MHz é o clock do cristal
    rfm96_write_reg(RFM96_REG_FRF_MSB, (uint8_t)(frf >> 16));
    rfm96_write_reg(RFM96_REG_FRF_MID, (uint8_t)(frf >> 8));
    rfm96_write_reg(RFM96_REG_FRF_LSB, (uint8_t)(frf >> 0));
}

bool rfm96_init(float freq) {
    // 1. Verificar a versão para garantir que a comunicação SPI está funcionando
    uint8_t version = rfm96_read_reg(RFM96_REG_VERSION);
    if (version != 0x12) {
        return false;
    }

    // 2. Colocar em modo Sleep e habilitar o modo LoRa
    rfm96_set_mode(RFM96_MODE_SLEEP);

    // 3. Configurar a frequência
    rfm96_set_frequency(freq);

    // 4. Configurar endereços da FIFO
    rfm96_write_reg(RFM96_REG_FIFO_TX_BASE_ADDR, 0);
    rfm96_write_reg(RFM96_REG_FIFO_RX_BASE_ADDR, 0);

    // 5. Configurar LNA (ganho máximo)
    rfm96_write_reg(RFM96_REG_LNA, 0x23);

    // 6. Configurar potência de saída (PA_BOOST, ~17dBm)
    rfm96_write_reg(RFM96_REG_PA_CONFIG, 0x8F);

    // 7. Configurar parâmetros do modem LoRa
    // BW=125kHz, CR=4/5, SF=7, CRC implícito
    rfm96_write_reg(RFM96_REG_MODEM_CONFIG_1, 0x72);
    // SF=7, CRC on
    rfm96_write_reg(RFM96_REG_MODEM_CONFIG_2, 0x74);

    // 8. Colocar em modo Standby
    rfm96_set_mode(RFM96_MODE_STDBY);

    return true;
}

void rfm96_send_packet(uint8_t *data, uint8_t len) {
    // 1. Colocar em modo Standby
    rfm96_set_mode(RFM96_MODE_STDBY);

    // 2. Limpar flag de IRQ
    rfm96_write_reg(RFM96_REG_IRQ_FLAGS, 0xFF);

    // 3. Configurar ponteiro da FIFO e tamanho do payload
    rfm96_write_reg(RFM96_REG_FIFO_ADDR_PTR, 0);
    rfm96_write_reg(RFM96_REG_PAYLOAD_LENGTH, len);

    // 4. Escrever dados na FIFO
    lora_cs_write(1);
    spi_transfer(RFM96_REG_FIFO | 0x80); // Acesso à FIFO com bit de escrita
    for (int i = 0; i < len; i++) {
        spi_transfer(data[i]);
    }
    lora_cs_write(0);

    // 5. Iniciar transmissão
    rfm96_set_mode(RFM96_MODE_TX);

    // 6. Aguardar o fim da transmissão
    while ((rfm96_read_reg(RFM96_REG_IRQ_FLAGS) & RFM96_IRQ_TX_DONE) == 0);

    // 7. Limpar flag de IRQ
    rfm96_write_reg(RFM96_REG_IRQ_FLAGS, RFM96_IRQ_TX_DONE);
}
