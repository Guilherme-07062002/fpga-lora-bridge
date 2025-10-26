"""
Extensão de pinos para ColorLight i9

ATENÇÃO: Ajuste os valores de Pins() para refletir os pinos reais na sua placa.
Este exemplo usa CN4 para LoRa SPI e J4 para I2C AHT10, seguindo padrão BitDogLab.

Objetivo:
- Mapear um conector IDC 14 pinos (CNx) para o módulo LoRa RFM96 (SPI): SCK, MOSI, MISO, CS.
- Mapear um conector JST de 4 pinos para I2C do AHT10: SCL, SDA (3V3, GND vêm do cabeçalho).

Notas:
- IOStandard: LVCMOS33
- Se incorreto, síntese falha ao request("lora_spi") ou request("i2c0").
- Consulte litex_boards.platforms.colorlight_i5 para sintaxe (ex.: "J1:1").
"""

from litex.build.generic_platform import Subsignal, Pins, IOStandard

# Mapeamento para LoRa RFM96 via SPI (conector CN4, padrão BitDogLab)
# Observação: o core SPIMaster espera o subsinal "clk" (não "sclk").
lora_spi_io = [
    ("lora_spi", 0,
        Subsignal("clk",  Pins("CN4:1")),  # SCK (nome esperado pelo core)
        Subsignal("mosi", Pins("CN4:2")),  # MOSI
        Subsignal("miso", Pins("CN4:3")),  # MISO
        Subsignal("cs_n", Pins("CN4:4")),  # CS (NSS)
        IOStandard("LVCMOS33")
    ),
]

# Mapeamento para I2C AHT10 (conector J4, padrão BitDogLab)
i2c0_io = [
    ("i2c0", 0,
        Subsignal("scl", Pins("J4:1")),   # SCL
        Subsignal("sda", Pins("J4:2")),   # SDA
        IOStandard("LVCMOS33")
    ),
]
