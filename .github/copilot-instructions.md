## Visão rápida do repositório

Este repositório contém dois subsistemas principais:
- hardware/: gateware e IP em LiteX/Migen e SystemVerilog — o acelerador de produto escalar está em `rtl/dot_product_accel.sv` com wrapper em `hardware/ip/dot_product_wrapper.py`.
- software/: firmware para Raspberry Pi Pico (Pico W) usando o SDK da Raspberry — código fonte em `software/src/` e CMake em `software/CMakeLists.txt`.

Foco principal para agentes: entender a integração entre o firmware (C), o wrapper LiteX/Python e o RTL do acelerador — comunicação ocorre por CSRs (Control/Status Registers).

## Arquitetura e fluxo de dados (essencial)
- O acelerador é instanciado pelo wrapper LiteX `hardware/ip/dot_product_wrapper.py`. O wrapper cria CSRs nomeadas (`a0..a7`, `b0..b7`, `start`, `done`, `result_lo`, `result_hi`) e instancia o módulo SystemVerilog.
- Firmware (em `hardware/ip/firmware.c`) escreve valores nos CSRs de entrada, gera um pulso em `start`, aguarda `done` e então lê `result_lo`/`result_hi`.
- `hardware/ip/firmware_sim.py` é um simulador Python que replica a lógica do firmware/CSRs para testar o RTL localmente sem toolchain — útil para desenvolvimento rápido e verificação funcional.

Arquivos-chave para leitura rápida:
- Wrapper/CSR: `hardware/ip/dot_product_wrapper.py`
- Firmware C que usa CSRs: `hardware/ip/firmware.c`
- Simulador Python e utilitários: `hardware/ip/firmware_sim.py`
- RTL: `rtl/dot_product_accel.sv`

## Convenções específicas do projeto
- Nomes de CSR no wrapper são curtos (`a0..a7`, `b0..b7`, `start`, `done`, `result_lo`, `result_hi`). O gerador de LiteX/C em tempo de build pode produzir símbolos C como `dotprod_start_write()` ou macros `CSR_DOTPROD_A0_ADDR` — sempre verifique o cabeçalho gerado em `build/generated/` após configurar o build.
- Observação prática: o simulador Python usa chaves como `dotp_a0` enquanto o firmware C usa `dotprod_*` — agentes devem procurar o cabeçalho gerado (`build/generated/...`) para confirmar os nomes exatos antes de alterar o firmware.

## Comandos de build e fluxo de desenvolvimento
- Simulação rápida (sem toolchain):
  - Na raiz do projeto execute: `make sim` (usa `hardware/ip/firmware_sim.py`).
  - Para testar o testbench do RTL: `make tb` (compila com Icarus/iverilog e roda `vvp`).
- Gateware / SoC (LiteX) e firmware:
  - Hardware/SoC: o Makefile raiz orquestra targets, mas a construção do SoC geralmente usa LiteX build scripts (ver `hardware/Makefile`).
  - Firmware (Pico): entre em `software/` e use a sequência usual CMake + Ninja:
    - mkdir -p build && cd build
    - cmake .. [-DPICO_SDK_PATH=/home/user/.pico-sdk/sdk/1.5.1]
    - ninja
  - VSCode já possui tasks configuradas (`software/.vscode/tasks.json`) que chamam o Ninja do `~/.pico-sdk` e ferramentas como `picotool` ou `openocd`.

## Debugging e depuração
- Serial/UART: o firmware imprime via UART — ver `firmware.c` (funções `uart_write_*` e `console_service`) para comandos interativos (`dotp`, `led`, `reboot`).
- Debug com OpenOCD/GDB: VSCode `launch.json` contém configurações para `cortex-debug` usando OpenOCD e `gdb`. Preencha `PICO_SDK_PATH`/toolchain no ambiente local ou use a extensão Pico VSCode.

## Pontos a checar antes de editar firmware ou wrapper
- Sempre gerar/inspecionar os headers gerados pelo LiteX/CMake (procure `build/generated/` ou `build/` no firmware build) para confirmar nomes de funções/macros CSR.
- Quando alterar o wrapper CSR names, atualize o firmware e o simulador Python juntos (nomenclatura inconsistente causa bugs difíceis de rastrear).

## Exemplos práticos (onde buscar/alterar)
- Para adicionar um CSR ou mudar largura: editar `hardware/ip/dot_product_wrapper.py` (adiciona CSRStorage/CSRStatus) e regenerar o SoC/build para atualizar `generated/csr.h`.
- Para testar localmente sem toolchain: rodar `make sim` (executa `hardware/ip/firmware_sim.py`) e `make tb` (iverilog + vvp) para validar o RTL.

## Erros e armadilhas comuns
- Divergência de nomes entre wrapper <-> headers gerados <-> simulador Python (confira `build/generated` após build).
- Dependências de ferramenta (Pico SDK, ninja, picotool, openocd) são referenciadas por caminhos em `.vscode/settings.json` — agentes devem usar variáveis de ambiente (HOME/USERPROFILE) ou instruir o dev a ajustar os caminhos.

Se quiser, atualizo a versão com comandos de build mais detalhados adaptados ao seu ambiente (ex.: caminhos do Pico SDK) ou adiciono exemplos de alteração de CSR no wrapper e regeneração dos headers. Qual parte você quer que eu detalhe primeiro?


## Requisitos da tarefa que deverá ser implementada

Tarefa 05 – Transmissão de dados via LoRa
Nesta atividade, o objetivo é desenvolver um sistema de comunicação sem fio entre um SoC customizado rodando em uma FPGA ColorLight i9 e um dispositivo externo, no caso a BitDogLab utilizando módulos LoRa RFM96.

O nosso SoC atuará como nó transmissor, coletando dados ambientais a partir de um sensor de temperatura e umidade conectado via I2C e enviando-os periodicamente pelo módulo LoRa conectado via SPI.

A BitDogLab, por sua vez, será o nó receptor, responsável por receber os dados transmitidos via LoRa e exibi-los em um display OLED.

O projeto deverá integrar tanto o desenvolvimento de hardware (configuração do SoC com LiteX) quanto de software (firmware para o processador VexRiscv e firmware para o microcontrolador da BitDogLab).

Requisitos do Projeto
FPGA (ColorLight i9)
Implementar um SoC customizado com base no target “colorlight_i5” no LiteX, incluindo:

Core: VexRiscv.
Barramento SPI: conectado ao módulo LoRa RFM96.
Barramento I2C: conectado ao sensor de temperatura e umidade AHT10.
Timer
A nossa placa FPGA possui 4 conectores IDC de 14 pinos, no mesmo padrão da BitDogLab: CN2-CN5. Defina um deles para ser utilizado pelo módulo LoRa e configure os pinos seguindo o padrão utilizado pela BitDogLab.

Além disso, temos 8 conectores JST de 4 pinos, que seguem o mesmo padrão das portas I2C da BitDogLab. Defina um deles para ser nossa porta I2C, seguindo o mesmo padrão de pinos da BitDogLab.

Desenvolver o firmware bare-metal em C para:

Inicializar os periféricos SPI e I2C.
Ler temperatura e umidade a cada 10 segundos.
Enviar os dados via LoRa (defina o seu formato).
O sistema deve operar de forma autônoma após a inicialização.
 
BitDogLab
Utilize o header IDC do centro da placa para conectar um módulo LoRa RFM96.
Desenvolva um firmware em C/C++ (ou MicroPython) que:

Receba os dados transmitidos pela FPGA.
Exiba temperatura e umidade no OLED.
Atualize os valores recebidos a cada nova transmissão.
O que não houver sido especificado aqui, ou não é necessário, ou pode ser implementado da maneira que for mais conveniente.


Entrega
Link de repositório público no Github contendo (só serão considerados os commits dentro do prazo de entrega):

Código-fonte completo do firmware da FPGA e da BitDogLab (pastas separadas).
Pasta com scripts ou configurações do LiteX para geração do bitstream (python).
Arquivo README.md com descrição do projeto, diagrama do sistema (blocos), dados de frequência e pinos utilizados e instruções de compilação e execução.
Link de vídeo curto (máx. 2 minutos) hospedado no YouTube demonstrando:

O funcionamento da comunicação entre os dispositivos.
A atualização dos dados no display OLED.

Avaliação (50 pontos)
Estrutura do projeto (5 pontos)
Organização do repositório.
README explicativo.
Uso adequado de versionamento.
Implementação do SoC (10 pontos)
Inclusão correta dos periféricos SPI e I2C.
Integração do core VexRiscv.
Geração funcional do bitstream.
Firmware do SoC (15 pontos)
Inicialização dos periféricos.
Leitura correta do sensor.
Transmissão periódica via LoRa.
Firmware da BitDogLab (15 pontos)
Recepção dos dados LoRa.
Exibição funcional no display OLED.
Demonstração em vídeo (5 pontos)
Clareza e objetividade.
Demonstração funcional do sistema