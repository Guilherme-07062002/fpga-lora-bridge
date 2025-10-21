# FPGA LoRa Bridge (Tarefa 05)

Este projeto implementa uma ponte de telemetria via LoRa usando:
- FPGA ColorLight i9 rodando um SoC LiteX/VexRiscv (nó transmissor)
- BitDogLab (RP2040) como nó receptor com OLED SSD1306

O transmissor lê temperatura/umidade do AHT10 (I2C) e envia periodicamente via LoRa (RFM96/SX1276). O receptor exibe no OLED.

## Estrutura
- hardware/
  - ip/colorlight_i5.py: script de SoC (LiteX) agora com flags opcionais `--with-lora`, `--with-aht10`, `--use-example-pins`.
  - ip/pins_colorlight_i9_ext.py: TEMPLATE de mapeamento de pinos (edite para a sua placa).
  - ip/firmware_lora.c: firmware bare-metal (scaffolding) para TX (AHT10 + LoRa via CSRs).
  - ip/firmware.c: demo anterior do acelerador (produto escalar).
- software/ (BitDogLab receptor)
  - include/lora_pins.h: pinos do IDC central (ajuste conforme sua fiação).
  - include/lora_rfm96.h, src/lora_rfm96.c: driver mínimo do RFM96 (RX contínuo).
  - include/ssd1306.h, src/ssd1306.c: driver OLED.
  - software.c: app receptor (inicializa OLED + LoRa e mostra T/H recebidos).

## Pinos
- LoRa (FPGA): selecione um conector IDC de 14 pinos (CN2–CN5). Sinais mínimos: SCK, MOSI, MISO, CS (NSS). Opcional: RESET, DIO0.
- I2C (FPGA): escolha um JST de 4 pinos para SCL/SDA (3V3/GND pelo conector).
- LoRa (BitDogLab): por padrão usa SPI0 do RP2040 (SCK=18, MOSI=19, MISO=16, CS=17, DIO0=20, RST=21). Edite `software/include/lora_pins.h`.

> Importante: ajuste `hardware/ip/pins_colorlight_i9_ext.py` para os pinos reais da sua placa (a síntese falhará se não corresponder).

## Build/execução
### Simulação rápida (sem toolchain FPGA)
- Acelerador demo: `make -C hardware sim`
- Testbench RTL: `make -C hardware tb`

### FPGA (SoC LiteX)
1) Ajuste os pinos em `hardware/ip/pins_colorlight_i9_ext.py`.
2) Gere o bitstream com LoRa/I2C:

```
make -C hardware lora-soc
```

3) Faça o load como de costume (programmer da ColorLight i9).

### BitDogLab (RP2040 receptor)
1) Instale Raspberry Pi Pico SDK e toolchain (VSCode tasks já apontam para `~/.pico-sdk`).
2) Compile:
```
mkdir -p software/build && cd software/build
cmake ..
ninja
```
3) Grave o UF2 com `picotool` ou arraste para o dispositivo em modo BOOTSEL.

## Protocolo
- Payload TX (FPGA): ASCII simples `T=xx.xx;H=yy.yy\n` (ver `firmware_lora.c`).
- RX (BitDogLab): faz parsing do formato acima e exibe no OLED; mostra RSSI/SNR quando não reconhece.

## Próximos passos (para produção)
- Implementar acessos reais aos CSRs de `i2c0` e `lora` no firmware do SoC (conforme `build/generated/csr.h`).
- Opcional: usar DIO0 (IRQ) e RESET no módulo LoRa, adicionar GPIOs no SoC.
- Validar frequência LoRa (EU868/US915) e parâmetros de modulação (BW/SF/CR).
- Caso use cabeçalho diferente na BitDogLab, ajuste `lora_pins.h`.

## Avisos
- O arquivo `pins_colorlight_i9_ext.py` contém pinos fictícios como exemplo, substitua antes do build.
- O driver LoRa incluído é mínimo para RX contínuo e pode requerer ajustes finos conforme a placa/módulo.
