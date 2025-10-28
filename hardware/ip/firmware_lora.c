// Firmware TX bare-metal (RISC-V) para enviar mock de temperatura/umidade via LoRa (RFM96)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <generated/csr.h>

#include "rfm96.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>

static char *readstr(void)
{
    char c[2];
    static char s[64];
    static int ptr = 0;

    if(readchar_nonblock()) {
        c[0] = readchar();
        c[1] = 0;
        switch(c[0]) {
            case 0x7f:
            case 0x08:
                if(ptr > 0) {
                    ptr--;
                    putsnonl("\x08 \x08");
                }
                break;
            case 0x07:
                break;
            case '\r':
            case '\n':
                s[ptr] = 0x00;
                putsnonl("\n");
                ptr = 0;
                return s;
            default:
                if(ptr >= (sizeof(s) - 1))
                    break;
                putsnonl(c);
                s[ptr] = c[0];
                ptr++;
                break;
        }
    }
    return NULL;
}

static char *get_token(char **str)
{
    char *c, *d;

    c = (char *)strchr(*str, ' ');
    if(c == NULL) {
        d = *str;
        *str = *str+strlen(*str);
        return d;
    }
    *c = 0;
    d = *str;
    *str = c+1;
    return d;
}

static void prompt(void)
{
    printf("RUNTIME>");
}

static void help(void)
{
    puts("Available commands:");
    puts("help                            - this command");
    puts("reboot                          - reboot CPU");
    puts("led                             - led test");
    puts("lora-bridge                     - lora bridge");
}

static void reboot(void)
{
    ctrl_reset_write(1);
}

static void toggle_led(void)
{
    int i;
    printf("invertendo led...\n");
    i = leds_out_read();
    leds_out_write(!i);
}

// Frequência do LoRa em MHz (US915 para BR)
#define LORA_FREQUENCY 915.0f

// Modo bridge: tudo que chegar como linha no console é enviado via LoRa.
// Digite 'exit' (ou 'quit'/'q') para sair e voltar ao prompt.
static void lora_bridge(void)
{
    printf("\n[LoRa Bridge] Inicializando RFM96 em %.1f MHz... ", (double)LORA_FREQUENCY);
    if (!rfm96_init(LORA_FREQUENCY)) {
        printf("FALHA (verifique SPI/pinos)\n");
        return;
    }
    printf("OK\n");

    puts("[LoRa Bridge] Digite uma linha e pressione Enter para transmitir.");
    puts("[LoRa Bridge] Para sair: 'exit' | 'quit' | 'q'.");
    putsnonl("BRIDGE>");

    while (1) {
        char *line = readstr();
        if (line == NULL) continue; // aguarda linha completa

        if ((strcmp(line, "exit") == 0) || (strcmp(line, "quit") == 0) || (strcmp(line, "q") == 0)) {
            puts("[LoRa Bridge] Encerrando bridge.\n");
            break;
        }

        size_t len = strlen(line);
        if (len == 0) {
            putsnonl("BRIDGE>");
            continue;
        }
        if (len > 255) len = 255; // rfm96_send_packet aceita uint8_t

        rfm96_send_packet((uint8_t*)line, (uint8_t)len);
        printf("[LoRa Bridge] TX (%uB): %s\n", (unsigned)len, line);
        putsnonl("BRIDGE>");
    }
}

static void console_service(void) {
    char *str;
    char *token;

    str = readstr();
    if(str == NULL) return;
    token = get_token(&str);
    if(strcmp(token, "help") == 0)
        help();
    else if(strcmp(token, "reboot") == 0)
        reboot();
    else if(strcmp(token, "led") == 0)
        toggle_led();
    else if(strcmp(token, "lora-bridge") == 0)
        lora_bridge();
    prompt();
}

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

    #ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif
    uart_init();

    printf("Hellorld!\n");
    help();
    prompt();

    while(1) {
        console_service();
    }

    return 0;

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
        sleep_ms(10000);
    }

    return 0;
}
