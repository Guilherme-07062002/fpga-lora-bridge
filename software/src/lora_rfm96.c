#include <pico/stdlib.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <stdio.h>
#include "lora_pins.h"
#include "lora_rfm96.h"

// Registradores essenciais (SX1276/77/78/79)
#define REG_FIFO            0x00
#define REG_OP_MODE         0x01
#define REG_FRF_MSB         0x06
#define REG_FRF_MID         0x07
#define REG_FRF_LSB         0x08
#define REG_FIFO_ADDR_PTR   0x0D
#define REG_FIFO_TX_BASE    0x0E
#define REG_FIFO_RX_BASE    0x0F
#define REG_FIFO_RX_CURRENT 0x10
#define REG_IRQ_FLAGS       0x12
#define REG_RX_NB_BYTES     0x13
#define REG_PKT_SNR_VALUE   0x19
#define REG_PKT_RSSI_VALUE  0x1A
#define REG_MODEM_CONFIG1   0x1D
#define REG_MODEM_CONFIG2   0x1E
#define REG_MODEM_CONFIG3   0x26
#define REG_DIO_MAPPING1    0x40

// Bits/flags
#define MODE_LONG_RANGE_MODE 0x80
#define MODE_SLEEP           0x00
#define MODE_STDBY           0x01
#define MODE_RX_CONTINUOUS   0x05

static inline void cs_low(void){ gpio_put(LORA_PIN_CS, 0); }
static inline void cs_high(void){ gpio_put(LORA_PIN_CS, 1); }

static uint8_t spi_xfer(uint8_t v) {
    uint8_t rx;
    spi_write_read_blocking(LORA_SPI_PORT, &v, &rx, 1);
    return rx;
}

static void write_reg(uint8_t addr, uint8_t val){
    cs_low();
    spi_xfer(addr | 0x80);
    spi_xfer(val);
    cs_high();
}

static uint8_t read_reg(uint8_t addr){
    cs_low();
    spi_xfer(addr & 0x7F);
    uint8_t v = spi_xfer(0);
    cs_high();
    return v;
}

static void set_frequency_hz(uint32_t hz){
    // FRF = (hz << 19) / 32e6
    uint64_t frf = ((uint64_t)hz << 19) / 32000000ULL;
    write_reg(REG_FRF_MSB, (frf >> 16) & 0xFF);
    write_reg(REG_FRF_MID, (frf >> 8)  & 0xFF);
    write_reg(REG_FRF_LSB, (frf >> 0)  & 0xFF);
}

static void enter_mode(uint8_t mode){
    write_reg(REG_OP_MODE, MODE_LONG_RANGE_MODE | mode);
}

bool lora_init(void){
    // Pinos SPI
    gpio_set_function(LORA_PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(LORA_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(LORA_PIN_MISO, GPIO_FUNC_SPI);
    // CS
    gpio_init(LORA_PIN_CS);
    gpio_set_dir(LORA_PIN_CS, GPIO_OUT);
    cs_high();
    // RST (opcional)
    gpio_init(LORA_PIN_RST);
    gpio_set_dir(LORA_PIN_RST, GPIO_OUT);
    gpio_put(LORA_PIN_RST, 1);
    sleep_ms(10);
    gpio_put(LORA_PIN_RST, 0);
    sleep_ms(10);
    gpio_put(LORA_PIN_RST, 1);
    sleep_ms(10);

    spi_init(LORA_SPI_PORT, 4 * 1000 * 1000); // 4 MHz
    spi_set_format(LORA_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    // Entrar em LoRa + Standby
    enter_mode(MODE_SLEEP);
    enter_mode(MODE_STDBY);
    set_frequency_hz(915000000); // ajuste para sua região
    // Modem configs: BW125k, CR4/5, SF7 (básico)
    write_reg(REG_MODEM_CONFIG1, 0x72);
    write_reg(REG_MODEM_CONFIG2, 0x74);
    write_reg(REG_MODEM_CONFIG3, 0x04);
    // FIFO
    write_reg(REG_FIFO_TX_BASE, 0x00);
    write_reg(REG_FIFO_RX_BASE, 0x00);
    // DIO0 -> RxDone (map 00)
    write_reg(REG_DIO_MAPPING1, 0x00);
    // RX contínuo
    enter_mode(MODE_RX_CONTINUOUS);
    return true;
}

bool lora_receive(uint8_t *buf, uint8_t maxlen, lora_rx_info_t *info){
    // Poll IRQ flags
    uint8_t irq = read_reg(REG_IRQ_FLAGS);
    if (irq & 0x40) { // RxDone
        // Verifica CRC (opcional): bit 0x20 PayloadCrcError
        if (irq & 0x20) {
            write_reg(REG_IRQ_FLAGS, 0xFF); // limpa tudo
            return false;
        }
        uint8_t bytes = read_reg(REG_RX_NB_BYTES);
        uint8_t cur = read_reg(REG_FIFO_RX_CURRENT);
        // set pointer
        write_reg(REG_FIFO_ADDR_PTR, cur);
        if (bytes > maxlen) bytes = maxlen;
        // burst read FIFO
        cs_low();
        spi_xfer(REG_FIFO & 0x7F);
        for (uint8_t i = 0; i < bytes; i++) buf[i] = spi_xfer(0x00);
        cs_high();

        // RSSI e SNR
        int8_t snr_raw = (int8_t)read_reg(REG_PKT_SNR_VALUE);
        uint8_t rssi_raw = read_reg(REG_PKT_RSSI_VALUE);
        float snr_db = snr_raw / 4.0f;
        // Aproximação RSSI para 915MHz
        int rssi_dbm = (int)rssi_raw - 157; 

        if (info) {
            info->len = bytes;
            info->snr_db = snr_db;
            info->rssi_dbm = rssi_dbm;
        }
        // limpar flags
        write_reg(REG_IRQ_FLAGS, 0xFF);
        return true;
    }
    return false;
}
