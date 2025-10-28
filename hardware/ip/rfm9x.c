#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <generated/csr.h>
#include "rfm9x.h"

static inline void rfm9x_spi_write(uint8_t val) {
	spi_mosi_out_write(val);
	spi_control_write((8 << CSR_SPI_CONTROL_LENGTH_OFFSET) | (1 << CSR_SPI_CONTROL_START_OFFSET));
	while (!(spi_status_read() & (1 << CSR_SPI_STATUS_TX_READY_OFFSET))) { }
}

static inline uint8_t rfm9x_spi_read(void) {
	spi_mosi_out_write(0x00);
	spi_control_write((8 << CSR_SPI_CONTROL_LENGTH_OFFSET) | (1 << CSR_SPI_CONTROL_START_OFFSET));
	while (!(spi_status_read() & (1 << CSR_SPI_STATUS_RX_READY_OFFSET))) { }
	return spi_miso_in_read();
}

void rfm9x_select(void) {
	spi_cs_write(SPI_MODE_MANUAL);
}

void rfm9x_deselect(void) {
	spi_cs_write(SPI_MODE_MANUAL | 0x0000);
	usleep(1);
}

uint8_t rfm9x_read(uint8_t reg) {
	uint8_t val;
	rfm9x_select();
	rfm9x_spi_write(reg & 0x7F);
	val = rfm9x_spi_read();
	rfm9x_deselect();
    return val;
}

void rfm9x_write(uint8_t reg, uint8_t val) {
	rfm9x_select();
	rfm9x_spi_write(reg | 0x80);
	rfm9x_spi_write(val);
	rfm9x_deselect();
}

void rfm9x_reset(void) {
	spi_rst_out_write(0);
	for (volatile int i = 0; i < 10000; i++);
	spi_rst_out_write(1);
    for (volatile int i = 0; i < 10000; i++);
}

void rfm9x_setup(uint64_t frequency) {
	rfm9x_reset();
	rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);

    uint64_t frf = (frequency << 19) / 32000000;
    rfm9x_write(REG_FRF_MSB, (frf >> 16) & 0xFF);
    rfm9x_write(REG_FRF_MID, (frf >> 8) & 0xFF);
    rfm9x_write(REG_FRF_LSB, frf & 0xFF);

	rfm9x_write(REG_FIFO_TX_BASE_ADD, 0x00);
	rfm9x_write(REG_FIFO_RX_BASE_ADD, 0x00);
	rfm9x_write(REG_PA_CONFIG, 0xFF);
	rfm9x_write(REG_MODEM_CONFIG_1, 0x72);
	rfm9x_write(REG_MODEM_CONFIG_2, 0x74);
	rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void rfm9x_send(const char *msg) {
	rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
	rfm9x_write(REG_FIFO_ADDR_PT 0);
	int len = strlen(msg);
	for (int i = 0; i < len; i++) {
		rfm9x_write( REG_FIFO, msg[i]);
	}
	rfm9x_write(REG_PAYLOAD_LENGTH, len);
	rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

	while (!(rfm9x_read(REG_IRQ_FLAGS) & IRQ_TX_DONE)) {
		// aguardando TX_DONE
		sleep_ms(1);
	}
	rfm9x_write(REG_IRQ_FLAGS, IRQ_TX_DONE);
}

int rfm9x_receive(char *buf, int max_len) {
	rfm9x_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);

	while (!(rfm9x_read(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK)) {
		tight_loop_contents();
	}

	int len = rfm9x_read(REG_RX_NB_BYTES);
	rfm9x_write( REG_FIFO_ADDR_PT rfm9x_read(REG_FIFO_RX_CURRENT_ADDR));
	if (len > max_len) len = max_len;
	for (int i = 0; i < len; i++) {
		buf[i] = rfm9x_read( REG_FIFO);
	}

	rfm9x_write( REG_IRQ_FLAGS, IRQ_RX_DONE_MASK);
	return len;
}

