// dot_product_accel.sv
// Acelerador de Produto Escalar 8x32-bit (signed) com resultado 64-bit (signed)
// Interface simples: start/done e leitura do resultado
// Implementação sequencial: 8 ciclos por operação

`timescale 1ns/1ps

module dot_product_accel (
    input  logic                 clk,
    input  logic                 rst,          // síncrono, ativo alto

    // Controle
    input  logic                 start,        // pulso de 1 ciclo (ou nível) para iniciar
    output logic                 done,         // fica em 1 até novo start ou reset

    // Operandos (signed 32-bit)
    input  logic signed [31:0]   a0,
    input  logic signed [31:0]   a1,
    input  logic signed [31:0]   a2,
    input  logic signed [31:0]   a3,
    input  logic signed [31:0]   a4,
    input  logic signed [31:0]   a5,
    input  logic signed [31:0]   a6,
    input  logic signed [31:0]   a7,
    input  logic signed [31:0]   b0,
    input  logic signed [31:0]   b1,
    input  logic signed [31:0]   b2,
    input  logic signed [31:0]   b3,
    input  logic signed [31:0]   b4,
    input  logic signed [31:0]   b5,
    input  logic signed [31:0]   b6,
    input  logic signed [31:0]   b7,

    // Resultado (signed 64-bit)
    output logic signed [63:0]   result
);

    // Estados
    typedef enum logic [1:0] {
        S_IDLE = 2'b00,
        S_RUN  = 2'b01,
        S_DONE = 2'b10
    } state_t;

    state_t state, state_n;

    // Registradores internos para latência/consistência dos dados
    logic signed [31:0] A [0:7];
    logic signed [31:0] B [0:7];

    logic [3:0]               idx;      // 0..7
    logic [3:0]               idx_n;
    logic signed [63:0]       acc;      // acumulador
    logic signed [63:0]       acc_n;
    logic signed [63:0]       prod;     // produto intermediário

    // Latch dos operandos em start
    always_ff @(posedge clk) begin
        if (rst) begin
            A[0] <= '0; A[1] <= '0; A[2] <= '0; A[3] <= '0;
            A[4] <= '0; A[5] <= '0; A[6] <= '0; A[7] <= '0;
            B[0] <= '0; B[1] <= '0; B[2] <= '0; B[3] <= '0;
            B[4] <= '0; B[5] <= '0; B[6] <= '0; B[7] <= '0;
        end else if (start && (state != S_RUN)) begin
            A[0] <= a0; A[1] <= a1; A[2] <= a2; A[3] <= a3;
            A[4] <= a4; A[5] <= a5; A[6] <= a6; A[7] <= a7;
            B[0] <= b0; B[1] <= b1; B[2] <= b2; B[3] <= b3;
            B[4] <= b4; B[5] <= b5; B[6] <= b6; B[7] <= b7;
        end
    end

    // Multiplicação signed 32x32 -> 64 bits
    always_comb begin
        prod = $signed(A[idx]) * $signed(B[idx]);
    end

    // Próximos estados/valores
    always_comb begin
        state_n = state;
        idx_n   = idx;
        acc_n   = acc;

    case (state)
            S_IDLE: begin
                if (start) begin
                    idx_n = 4'd0;
                    acc_n = 64'sd0;
                    state_n = S_RUN;
                end
            end
            S_RUN: begin
                acc_n = acc + prod; // acumula produto do índice atual
                if (idx == 4'd7) begin
                    state_n = S_DONE;
                end else begin
                    idx_n = idx + 4'd1;
                end
            end
            S_DONE: begin
                // Mantém DONE até novo start
                if (start) begin
                    idx_n = 4'd0;
                    acc_n = 64'sd0;
                    state_n = S_RUN;
                end
            end
            default: begin
                state_n = S_IDLE;
            end
        endcase
    end

    // Registros de estado e acumulador
    always_ff @(posedge clk) begin
        if (rst) begin
            state  <= S_IDLE;
            idx    <= '0;
            acc    <= '0;
            result <= '0;
            done   <= 1'b0;
        end else begin
            state <= state_n;
            idx   <= idx_n;
            acc   <= acc_n;

            if (state == S_RUN && state_n == S_DONE) begin
                result <= acc_n; // valor final pós última acumulação
            end

            // Sinal done
            case (state_n)
                S_IDLE: done <= 1'b0;
                S_RUN:  done <= 1'b0;
                S_DONE: done <= 1'b1;
                default: done <= 1'b0;
            endcase
        end
    end

endmodule
