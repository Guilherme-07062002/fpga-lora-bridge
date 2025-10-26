#include "rfm96.h"
#include <stdio.h>
#include <generated/csr.h>

// Funções de baixo nível para comunicação SPI com o RFM96
// Usa o periférico SPIMaster do LiteX

// Helpers para controlar o CS em modo manual (bit 16 = cs_mode, bits [0]=chip 0)
static inline void spi_cs_manual_enable(void) {
    // Habilita modo manual sem selecionar chip (sel=0, mode=1)
    lora_cs_write(1u << 16);
}
static inline void spi_cs_assert(void) {
    // Seleciona chip 0 e mantém modo manual
    lora_cs_write((1u << 0) | (1u << 16));
}
static inline void spi_cs_deassert(void) {
    // Desseleciona chip e mantém modo manual
    lora_cs_write(1u << 16);
}

static inline uint8_t spi_transfer(uint8_t val) {
    // Envia 8 bits e aguarda DONE; lê MISO
    lora_mosi_write(val);
    lora_control_write((8u << 8) | 1u); // length=8 bits, start=1
    while (!(lora_status_read() & 1));  // Aguarda DONE
    return (uint8_t)lora_miso_read();
}

static void rfm96_write_reg(uint8_t reg, uint8_t val) {
    spi_cs_assert();
    spi_transfer(reg | 0x80); // Bit de escrita
    spi_transfer(val);
    spi_cs_deassert();
}

static uint8_t rfm96_read_reg(uint8_t reg) {
    uint8_t val;
    spi_cs_assert();
    spi_transfer(reg & 0x7F); // Bit de leitura
    val = spi_transfer(0x00);
    spi_cs_deassert();
    return val;
}

void rfm96_set_mode(uint8_t mode) {
    rfm96_write_reg(RFM96_REG_OP_MODE, mode);
}

void rfm96_set_frequency(float freq_mhz) {
    // FRF = (freq_Hz << 19) / 32e6
    uint64_t frf = ((uint64_t)(freq_mhz * 1000000.0f) << 19) / 32000000ull;
    rfm96_write_reg(RFM96_REG_FRF_MSB, (uint8_t)(frf >> 16));
    rfm96_write_reg(RFM96_REG_FRF_MID, (uint8_t)(frf >> 8));
    rfm96_write_reg(RFM96_REG_FRF_LSB, (uint8_t)(frf >> 0));
}

bool rfm96_init(float freq) {
    // Coloca CS em modo manual desde o início (necessário para bursts na FIFO)
    spi_cs_manual_enable();
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
    // BW=125kHz, CR=4/5, SF=7, CRC on
    rfm96_write_reg(RFM96_REG_MODEM_CONFIG_1, 0x72);
    rfm96_write_reg(RFM96_REG_MODEM_CONFIG_2, 0x74);
    // Otimizações adicionais
    rfm96_write_reg(RFM96_REG_MODEM_CONFIG_3, 0x04);

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

    // 4. Escrever dados na FIFO (burst com CS mantido)
    spi_cs_assert();
    spi_transfer(RFM96_REG_FIFO | 0x80); // Acesso à FIFO com bit de escrita
    for (int i = 0; i < len; i++) {
        spi_transfer(data[i]);
    }
    spi_cs_deassert();

    // 5. Iniciar transmissão
    rfm96_set_mode(RFM96_MODE_TX);

    // 6. Aguardar o fim da transmissão
    while ((rfm96_read_reg(RFM96_REG_IRQ_FLAGS) & RFM96_IRQ_TX_DONE) == 0);

    // 7. Limpar flag de IRQ
    rfm96_write_reg(RFM96_REG_IRQ_FLAGS, RFM96_IRQ_TX_DONE);
}
