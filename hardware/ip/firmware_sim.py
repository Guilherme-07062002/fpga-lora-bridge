#!/usr/bin/env python3

"""
Simulador do firmware para testar a lógica de comunicação CSR
"""

import sys
import time

# Simular registradores CSR como dicionário global
csr_regs = {}

# Inicializar CSRs
def init_csrs():
    global csr_regs
    # Entradas (a0-a7, b0-b7, start)
    for i in range(8):
        csr_regs[f'dotp_a{i}'] = 0
        csr_regs[f'dotp_b{i}'] = 0
    csr_regs['dotp_start'] = 0
    
    # Saídas (done, result_lo, result_hi)
    csr_regs['dotp_done'] = 0
    csr_regs['dotp_result_lo'] = 0
    csr_regs['dotp_result_hi'] = 0

# Simular hardware do acelerador
class DotProductAccelSim:
    def __init__(self):
        self.state = "IDLE"
        self.cycle_count = 0
        self.a_values = [0] * 8
        self.b_values = [0] * 8
        self.result = 0
        self.idx = 0
        self.acc = 0
        
    def tick(self):
        """Simula um ciclo de clock"""
        global csr_regs
        
        if self.state == "IDLE":
            if csr_regs['dotp_start'] == 1:
                # Captura valores
                for i in range(8):
                    self.a_values[i] = csr_regs[f'dotp_a{i}']
                    self.b_values[i] = csr_regs[f'dotp_b{i}']
                
                # Converte para signed 32-bit
                for i in range(8):
                    if self.a_values[i] >= 2**31:
                        self.a_values[i] -= 2**32
                    if self.b_values[i] >= 2**31:
                        self.b_values[i] -= 2**32
                
                self.state = "COMPUTING"
                self.cycle_count = 0
                self.idx = 0
                self.acc = 0
                csr_regs['dotp_done'] = 0
                print(f"🚀 Acelerador iniciou cálculo...")
                print(f"   A = {self.a_values}")
                print(f"   B = {self.b_values}")
                print(f"   Etapas do cálculo (hardware):")
                
        elif self.state == "COMPUTING":
            # Executa 1 multiplicação+acumulação por ciclo
            if self.idx < 8:
                a_i = self.a_values[self.idx]
                b_i = self.b_values[self.idx]
                prod = a_i * b_i
                self.acc += prod
                self.cycle_count += 1
                print(f"   [c{self.cycle_count:02d}] i={self.idx}: {a_i} * {b_i} = {prod}, acc={self.acc}")
                self.idx += 1

            if self.idx >= 8:
                # Finaliza e escreve registradores de saída
                self.result = self.acc
                if self.result < 0:
                    result_u64 = (1 << 64) + self.result
                else:
                    result_u64 = self.result

                csr_regs['dotp_result_lo'] = result_u64 & 0xFFFFFFFF
                csr_regs['dotp_result_hi'] = (result_u64 >> 32) & 0xFFFFFFFF
                csr_regs['dotp_done'] = 1
                self.state = "DONE"

                print(f"✅ Cálculo concluído em {self.cycle_count} ciclos")
                print(f"   Resultado signed: {self.result}")
                print(f"   result_lo: 0x{csr_regs['dotp_result_lo']:08X}")
                print(f"   result_hi: 0x{csr_regs['dotp_result_hi']:08X}")
                
        elif self.state == "DONE":
            if csr_regs['dotp_start'] == 1:
                self.state = "IDLE"  # Novo cálculo
            # Mantém done=1 até novo start

# Simular funções CSR do firmware
def dotp_a0_write(val): csr_regs['dotp_a0'] = val & 0xFFFFFFFF
def dotp_a1_write(val): csr_regs['dotp_a1'] = val & 0xFFFFFFFF
def dotp_a2_write(val): csr_regs['dotp_a2'] = val & 0xFFFFFFFF
def dotp_a3_write(val): csr_regs['dotp_a3'] = val & 0xFFFFFFFF
def dotp_a4_write(val): csr_regs['dotp_a4'] = val & 0xFFFFFFFF
def dotp_a5_write(val): csr_regs['dotp_a5'] = val & 0xFFFFFFFF
def dotp_a6_write(val): csr_regs['dotp_a6'] = val & 0xFFFFFFFF
def dotp_a7_write(val): csr_regs['dotp_a7'] = val & 0xFFFFFFFF

def dotp_b0_write(val): csr_regs['dotp_b0'] = val & 0xFFFFFFFF
def dotp_b1_write(val): csr_regs['dotp_b1'] = val & 0xFFFFFFFF
def dotp_b2_write(val): csr_regs['dotp_b2'] = val & 0xFFFFFFFF
def dotp_b3_write(val): csr_regs['dotp_b3'] = val & 0xFFFFFFFF
def dotp_b4_write(val): csr_regs['dotp_b4'] = val & 0xFFFFFFFF
def dotp_b5_write(val): csr_regs['dotp_b5'] = val & 0xFFFFFFFF
def dotp_b6_write(val): csr_regs['dotp_b6'] = val & 0xFFFFFFFF
def dotp_b7_write(val): csr_regs['dotp_b7'] = val & 0xFFFFFFFF

def dotp_start_write(val): 
    csr_regs['dotp_start'] = val & 0x1

def dotp_done_read(): 
    return csr_regs['dotp_done']

def dotp_result_lo_read(): 
    return csr_regs['dotp_result_lo']

def dotp_result_hi_read(): 
    return csr_regs['dotp_result_hi']

# Simular as funções do firmware original
def uart_write_str(s):
    print(s, end='')

def uart_write_hex32(v):
    print(f"0x{v:08X}", end='')

def uart_write_hex64(v):
    print(f"0x{v:016X}", end='')

def sw_dotp(a, b, verbose=False):
    """Implementação software do produto escalar com opção de log detalhado"""
    acc = 0
    if verbose:
        print("   Etapas do cálculo (software):")
    for i in range(8):
        prod = a[i] * b[i]
        acc += prod
        if verbose:
            print(f"   [s{i+1:02d}] i={i}: {a[i]} * {b[i]} = {prod}, acc={acc}")
    return acc

def hw_write_vectors(a, b):
    """Escreve vetores nos CSRs"""
    dotp_a0_write(a[0] & 0xFFFFFFFF); dotp_a1_write(a[1] & 0xFFFFFFFF); dotp_a2_write(a[2] & 0xFFFFFFFF); dotp_a3_write(a[3] & 0xFFFFFFFF)
    dotp_a4_write(a[4] & 0xFFFFFFFF); dotp_a5_write(a[5] & 0xFFFFFFFF); dotp_a6_write(a[6] & 0xFFFFFFFF); dotp_a7_write(a[7] & 0xFFFFFFFF)
    dotp_b0_write(b[0] & 0xFFFFFFFF); dotp_b1_write(b[1] & 0xFFFFFFFF); dotp_b2_write(b[2] & 0xFFFFFFFF); dotp_b3_write(b[3] & 0xFFFFFFFF)
    dotp_b4_write(b[4] & 0xFFFFFFFF); dotp_b5_write(b[5] & 0xFFFFFFFF); dotp_b6_write(b[6] & 0xFFFFFFFF); dotp_b7_write(b[7] & 0xFFFFFFFF)

def hw_start(accel: DotProductAccelSim):
    """Gera um pulso em 'start' equivalente ao firmware C.
    Seta start=1, avança um ciclo (tick) e então limpa para 0.
    """
    dotp_start_write(1)
    # Avança 1 ciclo para que o acelerador capture o comando
    accel.tick()
    # Limpa o start para evitar reexecuções involuntárias
    dotp_start_write(0)

def hw_done():
    return dotp_done_read()

def hw_result():
    lo = dotp_result_lo_read()
    hi = dotp_result_hi_read()
    # Reconstruir signed 64-bit
    result_u64 = (hi << 32) | lo
    if result_u64 >= (1 << 63):
        return result_u64 - (1 << 64)
    else:
        return result_u64

def main():
    """Simula o main() do firmware"""
    
    print("🔄 Inicializando simulação...")
    init_csrs()
    accel = DotProductAccelSim()
    
    uart_write_str("\nLiteX Dot-Product Accelerator Demo\n")
    uart_write_str("CPU: VexRiscv (Simulado)\n")

    # Vetores de teste (mesmo que no firmware)
    A = [1, -2, 3, -4, 5, -6, 7, -8]
    B = [8, 7, -6, -5, 4, 3, -2, -1]

    print(f"\n📊 Vetores de teste:")
    print(f"   A = {A}")
    print(f"   B = {B}")

    # Software
    sw_start = time.perf_counter()
    sw = sw_dotp(A, B, verbose=True)
    sw_elapsed = time.perf_counter() - sw_start
    uart_write_str("Software: "); uart_write_hex64(sw & 0xFFFFFFFFFFFFFFFF); uart_write_str("\n")
    uart_write_str("Software time (wall): "); uart_write_str(f"{sw_elapsed*1e6:.2f} us\n")

    # Hardware
    print(f"\n⚙️  Executando no acelerador...")
    hw_write_vectors(A, B)
    # Pulso start como no firmware C
    hw_start(accel)
    
    # Simular ciclos até done (o start já foi limpo dentro de hw_start)
    # Medir tempo de hardware em ciclos e também wall-clock (simulado)
    hw_wall_start = time.perf_counter()
    max_cycles = 1024
    for cycle in range(max_cycles):
        if hw_done():
            break
        accel.tick()
    hw_wall_elapsed = time.perf_counter() - hw_wall_start
    # ciclos efetivos do acelerador
    hw_cycles = accel.cycle_count
    # Converter ciclos em tempo assumindo frequência do SoC (padrão 50 MHz)
    CLOCK_FREQ_HZ = 50e6
    hw_time_from_cycles = hw_cycles / CLOCK_FREQ_HZ
    
    hw = hw_result()
    uart_write_str("Hardware: "); uart_write_hex64(hw & 0xFFFFFFFFFFFFFFFF); uart_write_str("\n")

    # Mostrar tempos e ciclos
    uart_write_str("Hardware cycles: "); uart_write_str(f"{hw_cycles}\n")
    uart_write_str("Hardware time (from cycles): "); uart_write_str(f"{hw_time_from_cycles*1e6:.2f} us\n")
    uart_write_str("Hardware time (wall): "); uart_write_str(f"{hw_wall_elapsed*1e6:.2f} us\n")

    # Calcular speedup: quanto mais rápido o hardware é em relação ao software
    # Usa-se como referência o tempo de software medido (sw_elapsed, em segundos)
    if hw_time_from_cycles > 0:
        speedup_cycles = sw_elapsed / hw_time_from_cycles
        uart_write_str("Speedup (software / hw cycles): "); uart_write_str(f"{speedup_cycles:.2f}x\n")
    else:
        uart_write_str("Speedup (software / hw cycles): inf\n")

    # Comparação com base no tempo wall-clock da simulação (menos representativo do hardware real)
    if hw_wall_elapsed > 0:
        speedup_wall = sw_elapsed / hw_wall_elapsed
        uart_write_str("Speedup (software / hw wall): "); uart_write_str(f"{speedup_wall:.2f}x\n")
    else:
        uart_write_str("Speedup (software / hw wall): inf\n")

    if hw == sw:
        uart_write_str("[OK] Resultado coincide!\n")
    else:
        uart_write_str("[ERRO] Resultado diferente!\n")
        print(f"\n❌ Erro detectado:")
        print(f"   Software: {sw}")
        print(f"   Hardware: {hw}")
        sys.exit(1)
    
    print(f"\n🎉 Simulação concluída com sucesso!")

if __name__ == "__main__":
    main()