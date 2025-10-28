#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "rfm9x.h"

void rfm9x_select(rfm9x_t *r) {
	gpio_put(r->spi_cs_pin, 0);
}

void rfm9x_deselect(rfm9x_t *r) {
	gpio_put(r->spi_cs_pin, 1);
}

uint8_t rfm9x_read(rfm9x_t *r, uint8_t reg) {
	uint8_t tx[2] = { reg & 0x7F, 0x00 };
	uint8_t rx[2] = { 0 };
	rfm9x_select(r);
	spi_write_read_blocking(r->spi_port, tx, rx, 2);
	rfm9x_deselect(r);
	return rx[1];
}

void rfm9x_write(rfm9x_t *r, uint8_t reg, uint8_t val) {
	uint8_t tx[2] = { reg | 0x80, val };
	rfm9x_select(r);
	spi_write_blocking(r->spi_port, tx, 2);
	rfm9x_deselect(r);
}

void rfm9x_reset(rfm9x_t *r) {
	gpio_put(r->spi_rst_pin, 0);
	sleep_ms(10);
	gpio_put(r->spi_rst_pin, 1);
	sleep_ms(10);
}

void rfm9x_setup(rfm9x_t *r, uint64_t frequency) {
	spi_init(r->spi_port, r->spi_freq);
	spi_set_function(r->spi_miso_pin, GPIO_FUNC_SPI);
	gpio_set_function(r->spi_mosi_pin, GPIO_FUNC_SPI);
	gpio_set_function(r->spi_clk_pin, GPIO_FUNC_SPI);

	gpio_init(r->spi_cs_pin); gpio_set_dir(r->spi_cs_pin, true); rfm9x_deselect(r);
	gpio_init(r->spi_rst_pin); gpio_set_dir(r->spi_rst_pin, true);
	gpio_init(r->spi_dio0_pin); gpio_set_dir(r->spi_dio0_pin, false);

	rfm9x_reset(r);
	rfm9x_write(r, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);

	uint64_t frf = (frequency << 19) / 32000000;
	rfm9x_write(r, REG_FRF_MSB, (frf >> 16) & 0xFF);
	rfm9x_write(r, REG_FRF_MID, (frf >> 8) & 0xFF);
	rfm9x_write(r, REG_FRF_LSB, frf & 0xFF);

	rfm9x_write(r, REG_FIFO_TX_BASE_ADDR, 0x00);
	rfm9x_write(r, REG_FIFO_RX_BASE_ADDR, 0x00);
	rfm9x_write(r, REG_PA_CONFIG, 0xFF);
	rfm9x_write(r, REG_MODEM_CONFIG_1, 0x72);
	rfm9x_write(r, REG_MODEM_CONFIG_2, 0x74);
	rfm9x_write(r, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void rfm9x_send(rfm9x_t *r, const char *msg) {
	rfm9x_write(r, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
	rfm9x_write(r, REG_FIFO_ADDR_PTR, 0);
	int len = strlen(msg);
	for (int i = 0; i < len; i++) {
		rfm9x_write(r, REG_FIFO, msg[i]);
	}
	rfm9x_write(r, REG_PAYLOAD_LENGTH, len);
	rfm9x_write(r, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

	while (!(rfm9x_read(r, REG_IRQ_FLAGS) & RFM96_IRQ_TX_DONE)) {
		// aguardando TX_DONE
		sleep_ms(1);
	}
	rfm9x_write(r, REG_IRQ_FLAGS, RFM96_IRQ_TX_DONE);
}

int rfm9x_receive(rfm9x_t *r, char *buf, int max_len) {
	rfm9x_write(r, REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);

	while (!(rfm9x_read(r, REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK)) {
		tight_loop_contents();
	}

	int len = rfm9x_read(r, REG_RX_NB_BYTES);
	rfm9x_write(r, REG_FIFO_ADDR_PTR, rfm9x_read(r, REG_FIFO_RX_CURRENT_ADDR));
	if (len > max_len) len = max_len;
	for (int i = 0; i < len; i++) {
		buf[i] = rfm9x_read(r, REG_FIFO);
	}

	rfm9x_write(r, REG_IRQ_FLAGS, IRQ_RX_DONE_MASK);
	return len;
}

