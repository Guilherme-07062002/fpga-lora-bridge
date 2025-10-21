# Makefile raiz para orquestrar build do SoC e compilação do firmware
# Uso:
#   make build-all CROSS_COMPILE=riscv32-unknown-elf-

CROSS_COMPILE ?= riscv32-unknown-elf-
# Prefere python3 quando disponível, senão usa python
PYTHON ?= $(shell command -v python3 2>/dev/null || command -v python)
BOARD ?= i9
REVISION ?= 7.2

.PHONY: help sim tb

help:
	@echo "Makefile de alto nível para este projeto"
	@echo "Targets:"
	@echo "  sim            - executa simulação do firmware (python)"
	@echo "  tb 		 - compila o testbench do acelerador (iverilog)"

sim:
	$(PYTHON) ./ip/firmware_sim.py

tb:
	mkdir -p sim
	iverilog -g2012 -o sim/dot_product_accel.vvp rtl/dot_product_accel.sv tb/tb_dot_product_accel.sv
	vvp sim/dot_product_accel.vvp