#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <generated/csr.h>

// Nota importante:
// Este firmware pressupõe que o SoC foi gerado com:
// - I2C Master em `i2c0` (para AHT10 no endereço 0x38)
// - SPI Master em `lora`  (para RFM96/SX1276)
// Os nomes exatos das funções/macros dependem do header gerado em build/generated/csr.h.
// Ajuste os identificadores abaixo caso o gerador use prefixos diferentes.

// ---- AHT10 (I2C: 0x38) utilitários -----------------------------------------
#define AHT10_ADDR 0x38

static void busy_wait_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; i++) { __asm__ volatile("nop"); }
}

static void msleep(uint32_t ms) {
    // Se o SoC tiver timer, preferir usar. Aqui: atraso simples por loop.
    // Aproximado para sys_clk_freq ~ 60 MHz (ajuste fino conforme necessário).
    const uint32_t approx_cycles_per_ms = 60000; 
    while (ms--) busy_wait_cycles(approx_cycles_per_ms);
}

static int i2c0_write_bytes(uint8_t addr7, const uint8_t *buf, int len) {
    // Placeholder minimalista: muitos SoCs LiteX expõem um core I2C com registradores
    // de comando/estado. Sem o header real, deixamos um stub que retorna sucesso.
    // TODO: implementar usando os CSRs reais de i2c0.
    (void)addr7; (void)buf; (void)len;
    return 0;
}

static int i2c0_read_bytes(uint8_t addr7, uint8_t *buf, int len) {
    (void)addr7; (void)buf; (void)len;
    return 0;
}

static bool aht10_init(void) {
    // Comando de calibração: 0xE1, 0x08, 0x00 (ou 0xE1, 0x33, 0x00)
    uint8_t cmd[3] = {0xE1, 0x08, 0x00};
    if (i2c0_write_bytes(AHT10_ADDR, cmd, 3) != 0) return false;
    msleep(50);
    return true;
}

static bool aht10_measure(float *t_c, float *h_rh) {
    // Trigger: 0xAC, 0x33, 0x00
    uint8_t trigger[3] = {0xAC, 0x33, 0x00};
    uint8_t data[6];
    if (i2c0_write_bytes(AHT10_ADDR, trigger, 3) != 0) return false;
    msleep(85);
    if (i2c0_read_bytes(AHT10_ADDR, data, 6) != 0) return false;

    uint32_t raw_h = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] >> 4) & 0x0F);
    uint32_t raw_t = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5];
    *h_rh = (raw_h / 1048576.0f) * 100.0f;
    *t_c  = (raw_t / 1048576.0f) * 200.0f - 50.0f;
    return true;
}

// ---- LoRa RFM96 (SPI) utilitários ------------------------------------------
// Operações básicas de SPI no core LiteX SPIMaster expõem FIFOs/dados/ctrl.
// Sem o header real aqui, deixamos stubs. Substitua pelas funções geradas.

static void lora_cs_low(void)  { /* TODO: se cs_n for controlado internamente pelo core, não é necessário. */ }
static void lora_cs_high(void) { /* idem */ }

static uint8_t lora_xfer(uint8_t v) {
    (void)v; return 0x00; // TODO: escrever no registrador de TX e ler RX do core SPI
}

static void lora_write_reg(uint8_t addr, uint8_t val) {
    lora_cs_low();
    lora_xfer(addr | 0x80);
    lora_xfer(val);
    lora_cs_high();
}

static uint8_t lora_read_reg(uint8_t addr) {
    lora_cs_low();
    lora_xfer(addr & 0x7F);
    uint8_t v = lora_xfer(0x00);
    lora_cs_high();
    return v;
}

static void lora_set_frequency_hz(uint32_t freq_hz) {
    // FRF = freq_hz / (32e6 / 2^19) => FRF = (freq_hz << 19) / 32e6
    uint64_t frf = ((uint64_t)freq_hz << 19) / 32000000ULL;
    lora_write_reg(0x06, (frf >> 16) & 0xFF);
    lora_write_reg(0x07, (frf >> 8)  & 0xFF);
    lora_write_reg(0x08, (frf >> 0)  & 0xFF);
}

static void lora_enter_sleep_lora(void) {
    // RegOpMode: LoRa=1 (bit7), Sleep=0x00 -> 0x80
    lora_write_reg(0x01, 0x80);
}

static void lora_enter_standby(void) {
    // LoRa + Standby (0x81)
    lora_write_reg(0x01, 0x81);
}

static void lora_init_tx_implicit(uint8_t payload_len) {
    lora_enter_sleep_lora();
    lora_enter_standby();
    lora_set_frequency_hz(915000000); // ajuste conforme região
    // Modem config: SF7/BW125kHz/CR4/5 (valores típicos mínimos)
    lora_write_reg(0x1D, 0x72); // BW=125k (7), CR=4/5 (001), explicit/implicit config bits
    lora_write_reg(0x1E, 0x74); // SF=7 (0111), CRC on
    lora_write_reg(0x26, 0x04); // LowDataRateOptimize off, AgcAutoOn on? (depende)
    // Base addresses de FIFO
    lora_write_reg(0x0E, 0x00); // FifoTxBaseAddr
    lora_write_reg(0x0F, 0x00); // FifoRxBaseAddr (não usado em TX)
    // Implicit header mode + length
    // Para implicito: set 0x1D bit0=1; aqui mantemos exemplo simples (explicit)
    (void)payload_len;
}

static void lora_write_fifo(const uint8_t *buf, uint8_t len) {
    // Set pointer
    lora_write_reg(0x0D, 0x00); // FifoAddrPtr
    // Burst write
    lora_cs_low();
    lora_xfer(0x00 | 0x80); // RegFifo (0x00) write
    for (uint8_t i = 0; i < len; i++) lora_xfer(buf[i]);
    lora_cs_high();
    lora_write_reg(0x22, len); // PayloadLength
}

static void lora_start_tx(void) {
    // RegOpMode: LoRa + TX (0x83)
    lora_write_reg(0x01, 0x83);
}

static bool lora_tx_done(void) {
    uint8_t irq = lora_read_reg(0x12);
    if (irq & 0x08) { // TxDone
        lora_write_reg(0x12, 0x08); // limpar flag
        return true;
    }
    return false;
}

// ---- Aplicação ----------------------------------------------------------------

static void format_payload(float t_c, float h_rh, uint8_t *out, uint8_t *out_len) {
    // Formato simples ASCII: "T=xx.xx;H=yy.yy\n"
    // Em produção, prefira binário compacto.
    int n = snprintf((char*)out, 64, "T=%.2f;H=%.2f\n", (double)t_c, (double)h_rh);
    if (n < 0) n = 0; if (n > 64) n = 64;
    *out_len = (uint8_t)n;
}

int main(void) {
    printf("LoRa TX demo (LiteX/VexRiscv)\n");

    // Inicializa periféricos
    if (!aht10_init()) {
        printf("AHT10 init falhou (stub I2C)\n");
    }
    lora_init_tx_implicit(0); // payload_len para implicit (não usado aqui)

    // Loop: mede a cada ~10s e transmite
    while (1) {
        float t = 0, h = 0;
        if (!aht10_measure(&t, &h)) {
            printf("AHT10 measure falhou (stub I2C)\n");
        }
        uint8_t payload[64];
        uint8_t plen = 0;
        format_payload(t, h, payload, &plen);

        lora_write_fifo(payload, plen);
        lora_start_tx();
        // Espera TX concluir
        for (int i = 0; i < 5000; i++) { // ~timeout
            if (lora_tx_done()) break;
            msleep(2);
        }
        printf("TX: %s", payload);
        msleep(10000); // 10s
    }
    return 0;
}
