"""
Extensão de pinos (EXEMPLO) para ColorLight i9

ATENÇÃO: Este arquivo é um TEMPLATE para facilitar o mapeamento.
Edite os valores de Pins() para refletir os pinos reais na sua placa.

Objetivo:
- Mapear um conector IDC 14 pinos (CNx) para o módulo LoRa RFM96 (SPI): SCK, MOSI, MISO, CS.
- Mapear um conector JST de 4 pinos para I2C do AHT10: SCL, SDA (3V3, GND vêm do cabeçalho).

Notas:
- IOStandard típico: LVCMOS33
- Se o arquivo não estiver correto, a síntese irá falhar ao `request("lora_spi")` ou `request("i2c0")`.
- Consulte o arquivo de plataforma `litex_boards.platforms.colorlight_i5` para a sintaxe de conectores (ex.: "J1:1").
"""

from litex.build.generic_platform import Subsignal, Pins, IOStandard

# EXEMPLO: ajuste estes pinos. Use nomes conforme os conectores do platform.
# Abaixo está um mapeamento fictício apenas para referência. Substitua pelos pinos reais.

lora_spi_io = [
    ("lora_spi", 0,
        Subsignal("sclk", Pins("J3:1")),  # CNx SCK
        Subsignal("mosi", Pins("J3:2")),  # CNx MOSI
        Subsignal("miso", Pins("J3:3")),  # CNx MISO
        Subsignal("cs_n", Pins("J3:4")),  # CNx CS (NSS)
        IOStandard("LVCMOS33")
    ),
]

i2c0_io = [
    ("i2c0", 0,
        Subsignal("scl", Pins("J8:1")),   # JST SCL
        Subsignal("sda", Pins("J8:2")),   # JST SDA
        IOStandard("LVCMOS33")
    ),
]
