# dot_product_wrapper.py
# Wrapper LiteX/Migen para o módulo SystemVerilog dot_product_accel

import os
from migen import *
from litex.gen import LiteXModule
from litex.soc.interconnect.csr import CSRStorage, CSRStatus


class DotProductAccel(LiteXModule):
    def __init__(self, platform):
        self.start = CSRStorage(1, name="start")
        self.done = CSRStatus(1, name="done")
        self.result_lo = CSRStatus(32, name="result_lo")
        self.result_hi = CSRStatus(32, name="result_hi")

        # --- Sinais Internos ---
        a_sigs = [Signal(32, name=f"a{i}") for i in range(8)]
        b_sigs = [Signal(32, name=f"b{i}") for i in range(8)]
        start_signal = Signal()
        done_signal = Signal()
        result_signal = Signal(64)

        # --- Lógica de Geração de CSR e Conexões ---
        # Gera dinamicamente os CSRs de entrada para os vetores A e B
        # e conecta seus valores aos sinais internos.
        for i in range(8):
            # Cria o CSRStorage para a[i]
            csr_a = CSRStorage(32, name=f"a{i}")
            setattr(self, f"a{i}", csr_a)  # Adiciona como self.a<i>
            self.comb += a_sigs[i].eq(csr_a.storage)

            # Cria o CSRStorage para b[i]
            csr_b = CSRStorage(32, name=f"b{i}")
            setattr(self, f"b{i}", csr_b)  # Adiciona como self.b<i>
            self.comb += b_sigs[i].eq(csr_b.storage)

        # Conecta os sinais de controle e status
        self.comb += start_signal.eq(self.start.storage)
        self.sync += [
            self.done.status.eq(done_signal),
            self.result_lo.status.eq(result_signal[:32]),
            self.result_hi.status.eq(result_signal[32:]),
        ]

        # --- Instanciação do Módulo SystemVerilog ---
        # Adiciona o arquivo-fonte do acelerador ao build
        rtl_path = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "rtl", "dot_product_accel.sv"))
        platform.add_source(rtl_path)

        # Mapeia os sinais do wrapper para as portas do módulo SV
        self.specials += Instance("dot_product_accel",
            i_clk=ClockSignal(),
            i_rst=ResetSignal(),
            i_start=start_signal,
            o_done=done_signal,
            o_result=result_signal,
            **{f"i_a{i}": a_sigs[i] for i in range(8)},
            **{f"i_b{i}": b_sigs[i] for i in range(8)},
        )
