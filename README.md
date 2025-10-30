# FPGA LoRa Bridge (Tarefa 05)

Este projeto implementa uma ponte de telemetria via LoRa usando:
- FPGA ColorLight i5 rodando um SoC LiteX/VexRiscv (nó transmissor)
- BitDogLab como nó receptor com display OLED SSD1306

O transmissor lê temperatura/umidade do AHT10 (I2C) e envia periodicamente via LoRa RFM96. O receptor exibe os dados no display OLED.

## Estrutura

- hardware/
  - Código do SoC LiteX com IPs I2C e LoRa.
- software/ (BitDogLab receptor)
  - Firmware para RP2040 que recebe dados LoRa e exibe no OLED.


## Como Compilar e Executar

### Hardware (FPGA ColorLight i5)

### 1. Preparar o ambiente OSS CAD SUITE

É recomendado utilizar um ambiente virtual Python.

Baixe o oss-cad-suite de acordo com a release compatível com seu sistema operacional em:

[https://github.com/YosysHQ/oss-cad-suite-build/releases](https://github.com/YosysHQ/oss-cad-suite-build/releases)

Insira o arquivo compactado oss-cad-suite do baixado em `/tools` e realize a extração do conteúdo na mesma pasta.

Ou então para baixar por linha de comando:

```sh
# Acesse o diretório tools
cd tools

# Baixe a versão mais recente do oss-cad-suite (verifique a página de releases para a versão mais atual)
wget https://github.com/YosysHQ/oss-cad-suite-build/releases/download/2025-10-08/oss-cad-suite-linux-x64-20251008.tgz

# Ainda na mesma pasta, extraia o conteúdo do arquivo baixado
tar -xvzf oss-cad-suite-linux-x64-20251008.tgz
```

### 2. Acionar o ambiente do OSS CAD SUITE e Gere o SoC com LiteX

```sh
# Retorne ao diretório raiz do projeto
cd ..

# Acionar o ambiente do OSS CAD SUITE
source tools/oss-cad-suite/environment

# Busque o caminho do python3
which python3

# Gerar o SoC
caminho_do_python3 ./ip/colorlight_i5.py --board i9 --revision 7.2 --build --cpu-type=picorv32  --ecppack-compress
```

Se surgir alguma mensagem do tipo "No module named ...", faça a instalação do módulo faltante no ambiente virtual Python rodando:

```sh
pip3 install nome_do_modulo
```

E continue repetindo o processo até que não haja mais erros do tipo.

(Se assegure de estar baixando essas dependências no ambiente virtual Python, e não no sistema global.)

Caso essas dependências já estejam instaladas no sistema global, pode acontecer de o ambiente virtual não conseguir encontrá-las. Nesse caso, você pode tentar instalar as dependências diretamente no ambiente virtual com o comando acima.

### 3. Compilar o firmware
```sh
# Compile o firmware
make -C ip
```

Se houver algum erro, tente executar o comando:

```sh
# Limpa arquivos de build anteriores
make -C ip clean
```

E tente novamente.

### 4. Gravar o bitstream e o firmware na placa
Primeiro, execute no terminal o seguinte comando:

```sh
which openFPGALoader
```

Copie o caminho descoberto e execute os próximos passos, colocando o caminho no local indicado. O openFPGALoader é uma ferramenta utilizada para carregar arquivos para o FPGA, e já vem por padrão no OSS CAD Suite.

```sh
# Grave o bitstream na placa
/caminho/descoberto -b colorlight-i5 build/colorlight_i5/gateware/colorlight_i5.bit
```

### 5. Executar via terminal serial na placa FPGA

Execute o seguinte comando:

```sh
# Abra o terminal serial (verifique a porta correta, pode ser ttyACM0 ou ttyACM1)
litex_term /dev/ttyACM0 --kernel ip/firmware.bin
```

Caso ocorra algum erro com relação a porta, tente mudar para "ttyACM1", ou verifique a porta utilizada no momento em que foi colocado o FPGA no dispositivo.

Após executar o comando acima aperte "enter" e digite "reboot". Automaticamente o FPGA será reiniciado e o programa será executado e mostrado no terminal.

### Software (BitDogLab receptor)

Para facilitar a compilação e o upload do firmware no BitDogLab, utilize a extensão "Raspberry Pi Pico Project" do Visual Studio Code.

Abra o painel lateral da extensão e clique em "Compile Project" para compilar o firmware. Após a compilação, conecte o BitDogLab ao computador enquanto mantém pressionado o botão BOOTSEL para entrar no modo de bootloader USB. Em seguida, clique em "Run Project" na extensão para fazer o upload do firmware para o BitDogLab.