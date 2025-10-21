#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <stdio.h>
#include "ssd1306.h"
#include "lora_rfm96.h"

#define OLED_I2C i2c1
#define OLED_SDA 2
#define OLED_SCL 3
#define OLED_ADDR 0x3C

static ssd1306_t disp;

static void oled_init_and_clear(void){
    i2c_init(OLED_I2C, 400 * 1000);
    gpio_set_function(OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA);
    gpio_pull_up(OLED_SCL);
    sleep_ms(20);
    if (!ssd1306_init(&disp, 128, 64, OLED_ADDR, OLED_I2C)) {
        // fallback simples
        while(1){tight_loop_contents();}
    }
    ssd1306_clear(&disp);
    ssd1306_show(&disp);
}

static void oled_print_lines(const char* l1, const char* l2){
    ssd1306_clear(&disp);
    // Desenha texto básico com pixels (utilize seu próprio renderer se preferir)
    // Aqui: desenhar simples blocos como placeholder
    // Para simplicidade, mostramos só duas linhas com caracteres limitados
    // Recomenda-se integrar um drawText se disponível
    // Exibição mínima
    (void)l1; (void)l2;
    ssd1306_show(&disp);
}

int main() {
    stdio_init_all();
    sleep_ms(100);

    oled_init_and_clear();
    lora_init();

    char line1[22] = {0};
    char line2[22] = {0};
    uint8_t buf[64];
    lora_rx_info_t info;

    while (true) {
        if (lora_receive(buf, sizeof(buf), &info)) {
            // Espera formato ASCII: "T=xx.xx;H=yy.yy\n"
            buf[info.len < sizeof(buf) ? info.len : sizeof(buf)-1] = 0;
            float t=0, h=0;
            // Parse simples (tolerante a falhas)
            if (sscanf((char*)buf, "T=%f;H=%f", &t, &h) == 2) {
                snprintf(line1, sizeof(line1), "Temp: %.2f C", (double)t);
                snprintf(line2, sizeof(line2), "Umid: %.2f %%", (double)h);
            } else {
                snprintf(line1, sizeof(line1), "RX %u bytes", info.len);
                snprintf(line2, sizeof(line2), "RSSI %d SNR %.1f", info.rssi_dbm, (double)info.snr_db);
            }
            printf("RX: %s\n", buf);
            oled_print_lines(line1, line2);
        }
        sleep_ms(50);
    }
    return 0;
}