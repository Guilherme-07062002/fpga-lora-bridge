// Firmware TX bare-metal (RISC-V) para enviar mock de temperatura/umidade via LoRa (RFM96)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <sleep.h>
#include <generated/csr.h>

#include "rfm96.h"

// Frequência do LoRa em MHz (US915 para BR)
#define LORA_FREQUENCY 915.0f

// Gera valores mock (sem usar I2C/AHT10 por enquanto)
static void mock_read_temp_hum(float *t_c, float *h_rh, uint32_t iter) {
    // Varia entre 22.0..28.0 C e 40..70 %RH de forma suave
    float base_t = 25.0f;
    float base_h = 55.0f;
    float dt = (float)((iter % 50) - 25) / 50.0f; // -0.5 .. +0.5
    float dh = (float)((iter % 60) - 30) / 30.0f; // -1.0 .. +1.0
    *t_c = base_t + dt * 6.0f;
    *h_rh = base_h + dh * 15.0f;
}

int main(void) {
    // Inicializa IRQ e UART para console
    irq_setmask(0);
    irq_setie(1);
    uart_init();

    printf("--- FPGA LoRa Transmitter (mock AHT10) ---\n");

    // Inicializa o módulo LoRa RFM96 via SPI do LiteX (CSR prefixo: lora_*)
    printf("Initializing LoRa RFM96... ");
    if (!rfm96_init(LORA_FREQUENCY)) {
        printf("FAILED (check SPI/pins)\n");
        return -1;
    }
    printf("OK\n");

    uint32_t count = 0;
    while (1) {
        // Mock de leitura de sensores
        float temp_c = 0.0f, hum = 0.0f;
        mock_read_temp_hum(&temp_c, &hum, count);

        // Formato ASCII simples: T=xx.xx;H=yy.yy\n
        char payload[64];
        int n = snprintf(payload, sizeof(payload), "T=%.2f;H=%.2f\n", (double)temp_c, (double)hum);
        if (n < 0) n = 0; if (n > (int)sizeof(payload)) n = sizeof(payload);

        // Envia via LoRa
        rfm96_send_packet((uint8_t*)payload, (uint8_t)n);
        printf("TX #%lu: %s", (unsigned long)(++count), payload);

        // Periodicidade de 10s
        msleep(10000);
    }

    return 0;
}
