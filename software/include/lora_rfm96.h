#ifndef LORA_RFM96_H
#define LORA_RFM96_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    // RSSI em dBm aproximado e SNR em dB
    int rssi_dbm;
    float snr_db;
    uint8_t len;
} lora_rx_info_t;

// Inicializa SPI e pinos do RFM96, configura rádio em modo RX contínuo
bool lora_init(void);

// Tenta receber um pacote; retorna true se recebeu; preenche buf (até maxlen) e info
bool lora_receive(uint8_t *buf, uint8_t maxlen, lora_rx_info_t *info);

#endif // LORA_RFM96_H