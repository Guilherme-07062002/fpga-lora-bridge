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
# Abaixo está um mapeamento para o conector CN4 e J4. Substitua se usar outros.

lora_spi_io = [
    ("lora_spi", 0,
        Subsignal("sclk", Pins("CN4:1")),  # CN4 SCK
        Subsignal("mosi", Pins("CN4:2")),  # CN4 MOSI
        Subsignal("miso", Pins("CN4:3")),  # CN4 MISO
        Subsignal("cs_n", Pins("CN4:4")),  # CN4 CS (NSS)
        IOStandard("LVCMOS33")
    ),
]

i2c0_io = [
    ("i2c0", 0,
        Subsignal("scl", Pins("J4:1")),   # JST SCL
        Subsignal("sda", Pins("J4:2")),   # JST SDA
        IOStandard("LVCMOS33")
    ),
]
