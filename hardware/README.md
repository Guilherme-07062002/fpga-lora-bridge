# Acelerador de Produto Escalar com LiteX

Este projeto é um System-on-Chip (SoC) para a FPGA Colorlight i5, construído com o framework LiteX. O SoC contém um processador RISC-V e um acelerador de hardware para cálculo de produto escalar, com o qual a CPU se comunica via barramento CSR (Control and Status Register).

O repositório cobre o fluxo completo: o design do acelerador em SystemVerilog, sua integração ao SoC usando Python/Migen, e um firmware em C que valida o hardware e compara seu desempenho com uma versão em software.

## Arquitetura do Projeto

O projeto está organizado nos seguintes diretórios principais:

-   `ip/`: Contém os arquivos de integração com o LiteX (`wrapper`, `SoC`) e o firmware.
-   `rtl/`: Contém o código-fonte do acelerador de produto escalar em SystemVerilog.
-   `sim/`: Arquivos relacionados à simulação do projeto.
-   `tb/`: Inclui o testbench para a verificação funcional do acelerador.
-   `tools/`: Scripts e toolchains auxiliares para o processo de build.

### Componentes Principais

1.  **Acelerador (`rtl/dot_product_accel.sv`)**: Módulo em SystemVerilog que calcula o produto escalar entre dois vetores de 8 elementos (32-bit signed). A operação leva 8 ciclos de clock e o resultado é um valor de 64 bits.
2.  **Wrapper LiteX (`ip/dot_product_wrapper.py`)**: Uma classe Python que "envolve" o módulo SystemVerilog, expondo suas portas de entrada e saída como registradores no barramento CSR. É a ponte entre o hardware customizado e o ecossistema LiteX.
3.  **SoC (`ip/colorlight_i5.py`)**: Script principal que define o SoC, baseado no target `colorlight_i5` do LiteX. Ele instancia a CPU, a memória e mantém os periféricos padrão do target (ex.: LED chaser, SPI flash), adicionando o acelerador de produto escalar como um novo periférico.
4.  **Firmware (`ip/firmware.c`)**: Aplicação bare-metal em C que roda na CPU RISC-V. Ele inicializa a comunicação serial, calcula o produto escalar em software, depois usa o acelerador de hardware e, por fim, compara os dois resultados, imprimindo o status no terminal.

## Como Compilar e Executar

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

O `Makefile` na raiz do projeto automatiza o processo de simulação e testbench do acelerador. Para compilar e testar o projeto completo, siga os passos abaixo:

### 1. Simular o Acelerador (RTL)

Caso queira simular o acelerador e obter uma comparação entre a execução em hardware e software, execute:

```bash
make sim
```

#### Log de Execução (exemplo esperado)

```text
🔄 Inicializando simulação...

LiteX Dot-Product Accelerator Demo
CPU: VexRiscv (Simulado)

📊 Vetores de teste:
   A = [1, -2, 3, -4, 5, -6, 7, -8]
   B = [8, 7, -6, -5, 4, 3, -2, -1]
   Etapas do cálculo (software):
   [s01] i=0: 1 * 8 = 8, acc=8
   [s02] i=1: -2 * 7 = -14, acc=-6
   [s03] i=2: 3 * -6 = -18, acc=-24
   [s04] i=3: -4 * -5 = 20, acc=-4
   [s05] i=4: 5 * 4 = 20, acc=16
   [s06] i=5: -6 * 3 = -18, acc=-2
   [s07] i=6: 7 * -2 = -14, acc=-16
   [s08] i=7: -8 * -1 = 8, acc=-8
Software: 0xFFFFFFFFFFFFFFF8
Software time (wall): 44.18 us

⚙️  Executando no acelerador...
🚀 Acelerador iniciou cálculo...
   A = [1, -2, 3, -4, 5, -6, 7, -8]
   B = [8, 7, -6, -5, 4, 3, -2, -1]
   Etapas do cálculo (hardware):
   [c01] i=0: 1 * 8 = 8, acc=8
   [c02] i=1: -2 * 7 = -14, acc=-6
   [c03] i=2: 3 * -6 = -18, acc=-24
   [c04] i=3: -4 * -5 = 20, acc=-4
   [c05] i=4: 5 * 4 = 20, acc=16
   [c06] i=5: -6 * 3 = -18, acc=-2
   [c07] i=6: 7 * -2 = -14, acc=-16
   [c08] i=7: -8 * -1 = 8, acc=-8
✅ Cálculo concluído em 8 ciclos
   Resultado signed: -8
   result_lo: 0xFFFFFFF8
   result_hi: 0xFFFFFFFF
Hardware: 0xFFFFFFFFFFFFFFF8
Hardware cycles: 8
Hardware time (from cycles): 0.16 us
Hardware time (wall): 35.76 us
Speedup (software / hw cycles): 276.11x
Speedup (software / hw wall): 1.24x
[OK] Resultado coincide!

🎉 Simulação concluída com sucesso!
```

### Validação do Resultado com Números Negativos

O log de execução exibe 0xFFFFFFFFFFFFFFF8, um valor que valida a capacidade do acelerador de processar corretamente inteiros com sinal. Este formato hexadecimal é a representação padrão para o número -8 em 64 bits, conhecida como Complemento de Dois (Two's Complement). Nesta notação, o bit mais significativo atua como um sinalizador negativo, o que explica a sequência de Fs no início do valor. A adoção desse padrão é crucial no design de hardware, pois unifica a lógica para operações de soma e subtração, tornando o circuito mais eficiente.

---

Para executar o testbench apenas do acelerador, execute:

```bash
make tb
```

#### Log de Execução do Testbench (exemplo esperado)

```text
[OK] seed=1 resultado=3802990229 (0x00000000e2ad0695)
[OK] seed=42 resultado=4221255804 (0x00000000fb9b407c)
[OK] seed=2025 resultado=7719476028 (0x00000001cc1ddb3c)
Todos os testes passaram.
tb/tb_dot_product_accel.sv:116: $finish called at 375000 (1ps)
```

Este trecho do log é a saída do testbench (`make tb`) que verifica funcionalmente o módulo RTL do acelerador contra um modelo de referência. Abaixo está o significado das linhas mais importantes:

- `[OK] seed=... resultado=... (0x...)` — para cada seed (semente) usada pelo testbench para gerar vetores de entrada aleatórios, o TB calcula o produto escalar esperado (modelo golden) e compara com a saída do DUT. Se o resultado coincide, o TB imprime `[OK]` seguido da seed e do resultado em decimal e hexadecimal.
- `Todos os testes passaram.` — indica que todas as seeds testadas produziram o mesmo resultado no DUT e no modelo de referência.
- `tb/tb_dot_product_accel.sv:116: $finish called at 375000 (1ps)` — é a mensagem do Verilog indicando que o testbench chamou `$finish` na linha 116; o número `375000 (1ps)` é o tempo de simulação no instante do `$finish`. Com a unidade entre parênteses (`1ps`) isso significa 375000 picosegundos, ou seja 375 ns.

Observações sobre formatos numéricos


- O testbench imprime o `resultado` em decimal e em hexadecimal para facilitar a inspeção. A representação hexadecimal exibida é a forma natural de visualizar o valor binário completo (64 bits). Para interpretação como inteiro com sinal (two's complement) use a regra:

   - se o valor >= 2^63, então o equivalente signed = valor - 2^64
   - caso contrário, o valor já é o inteiro signed.

- Nos exemplos acima os resultados têm os 32 bits superiores iguais a zero (0x00000000...), portanto são valores positivos e a interpretação decimal corresponde ao valor signed.


### 2. Acionar o ambiente do OSS CAD SUITE e Gere o SoC com LiteX

```sh
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
# Assumindo que você já está no diretório ip
cd ip

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

### 6. Mapa de registradores (CSR)

A comunicação com o acelerador é realizada através dos seguintes registradores CSR, mapeados em memória a partir do endereço base `0x82000000`:

| Registrador           | Endereço     | Tamanho (bits) | Acesso          | Descrição                                         |
| :-------------------- | :----------- | :------------- | :-------------- | :------------------------------------------------ |
| `dotprod_a0`          | `0x82000000` | 32             | Escrita/Leitura | Elemento A[0] do vetor de entrada                 |
| `dotprod_a1`          | `0x82000004` | 32             | Escrita/Leitura | Elemento A[1] do vetor de entrada                 |
| `dotprod_a2`          | `0x82000008` | 32             | Escrita/Leitura | Elemento A[2] do vetor de entrada                 |
| `dotprod_a3`          | `0x8200000c` | 32             | Escrita/Leitura | Elemento A[3] do vetor de entrada                 |
| `dotprod_a4`          | `0x82000010` | 32             | Escrita/Leitura | Elemento A[4] do vetor de entrada                 |
| `dotprod_a5`          | `0x82000014` | 32             | Escrita/Leitura | Elemento A[5] do vetor de entrada                 |
| `dotprod_a6`          | `0x82000018` | 32             | Escrita/Leitura | Elemento A[6] do vetor de entrada                 |
| `dotprod_a7`          | `0x8200001c` | 32             | Escrita/Leitura | Elemento A[7] do vetor de entrada                 |
| `dotprod_b0`          | `0x82000020` | 32             | Escrita/Leitura | Elemento B[0] do vetor de entrada                 |
| `dotprod_b1`          | `0x82000024` | 32             | Escrita/Leitura | Elemento B[1] do vetor de entrada                 |
| `dotprod_b2`          | `0x82000028` | 32             | Escrita/Leitura | Elemento B[2] do vetor de entrada                 |
| `dotprod_b3`          | `0x8200002c` | 32             | Escrita/Leitura | Elemento B[3] do vetor de entrada                 |
| `dotprod_b4`          | `0x82000030` | 32             | Escrita/Leitura | Elemento B[4] do vetor de entrada                 |
| `dotprod_b5`          | `0x82000034` | 32             | Escrita/Leitura | Elemento B[5] do vetor de entrada                 |
| `dotprod_b6`          | `0x82000038` | 32             | Escrita/Leitura | Elemento B[6] do vetor de entrada                 |
| `dotprod_b7`          | `0x8200003c` | 32             | Escrita/Leitura | Elemento B[7] do vetor de entrada                 |
| `dotprod_start`       | `0x82000040` | 1              | Escrita/Leitura | Sinaliza o início do cálculo (escrita com '1')     |
| `dotprod_done`        | `0x82000044` | 1              | Leitura         | Indica que o cálculo foi concluído (lê '1')       |
| `dotprod_result_lo`   | `0x82000048` | 32             | Leitura         | Parte baixa (bits 31:0) do resultado de 64 bits   |
| `dotprod_result_hi`   | `0x8200004c` | 32             | Leitura         | Parte alta (bits 63:32) do resultado de 64 bits   |

## Referências

- <https://github.com/enjoy-digital/litex>
- <https://github.com/enjoy-digital/litex/wiki>
- <https://github.com/enjoy-digital/litex/wiki/Tutorials-Resources>
- <https://www.controlpaths.com/2022/01/17/building-soc-litex/>
- <https://github.com/litex-hub/litex-boards>
- <https://github.com/litex-hub/fpga_101>
- <https://github.com/matheus-555/Projeto-Embarcatech-FPGA>
- <https://github.com/JN513/Utilizando-FPGAs-com-ferramentas-OpenSource>
- <https://www.youtube.com/watch?v=qPougRqk_SY>
