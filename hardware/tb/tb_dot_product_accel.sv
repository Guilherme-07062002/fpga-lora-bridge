// tb_dot_product_accel.sv
`timescale 1ns/1ps

module tb_dot_product_accel;
    logic clk;
    logic rst;
    logic start;
    logic done;
    logic signed [31:0] a[0:7];
    logic signed [31:0] b[0:7];
    logic signed [31:0] a0,a1,a2,a3,a4,a5,a6,a7;
    logic signed [31:0] b0,b1,b2,b3,b4,b5,b6,b7;
    logic signed [63:0] result;

    // DUT
    dot_product_accel dut(
        .clk(clk), .rst(rst), .start(start), .done(done),
        .a0(a0), .a1(a1), .a2(a2), .a3(a3), .a4(a4), .a5(a5), .a6(a6), .a7(a7),
        .b0(b0), .b1(b1), .b2(b2), .b3(b3), .b4(b4), .b5(b5), .b6(b6), .b7(b7),
        .result(result)
    );

    // Clock 10ns -> 100MHz
    initial clk = 0;
    always #5 clk = ~clk;

    // Task de aplicação de vetores e checagem
    task run_case(input integer seed);
        integer i;
        longint signed sw_sum;
        integer s1, s2;
        begin
            // Gera dados determinísticos a partir da seed
            s1 = seed;
            s2 = seed + 32;
            for (i=0;i<8;i=i+1) begin
                a[i] = $signed($random(s1));
                b[i] = $signed($random(s2));
            end
            // Limita faixas para evitar overflow extremo nos testes
            for (i=0;i<8;i=i+1) begin
                a[i][31:16] = '0; // 16-bit efetivo
                b[i][31:16] = '0;
            end
            {a0,a1,a2,a3,a4,a5,a6,a7} = {a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]};
            {b0,b1,b2,b3,b4,b5,b6,b7} = {b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7]};

            // SW referência
            sw_sum = 0;
            for (i=0;i<8;i=i+1) begin
                sw_sum += longint'(a[i]) * longint'(b[i]);
            end

            // Pulso de start
            @(posedge clk);
            start <= 1'b1;
            @(posedge clk);
            start <= 1'b0;

            // Garante que vamos observar uma nova transição de done: 1->0->1
            // Aguarda done baixar (se já estava em 1 de operação anterior)
            while (done) @(posedge clk);
            // Aguarda done subir novamente ao fim do cálculo
            while (!done) @(posedge clk);

            // Checagem (exibe decimal e hexadecimal para facilitar análise)
            if (result !== sw_sum) begin
                $display("[ERRO] seed=%0d esperado=%0d (0x%016h) obtido=%0d (0x%016h)", seed, sw_sum, sw_sum, result, result);
                $fatal(1);
            end else begin
                $display("[OK] seed=%0d resultado=%0d (0x%016h)", seed, result, result);
            end
        end
    endtask

    // Controle por plusargs:
    // +numseeds=N     -> executa N seeds sequenciais (se 0, usa modo padrão)
    // +seedstart=S    -> primeira seed (padrão 1)
    // +vcd=0          -> desabilita geração de VCD (padrão 1/habilitado)
    integer numseeds;
    integer seedstart;
    integer vcd_en;

    initial begin
        // Defaults
        numseeds  = 0;
        seedstart = 1;
        vcd_en    = 1;
        void'($value$plusargs("numseeds=%d", numseeds));
        void'($value$plusargs("seedstart=%d", seedstart));
        void'($value$plusargs("vcd=%d", vcd_en));

        if (vcd_en != 0) begin
            $dumpfile("sim/dot_product_accel.vcd");
            $dumpvars(0, tb_dot_product_accel);
        end
        rst   = 1;
        start = 0;
        a0=0;a1=0;a2=0;a3=0;a4=0;a5=0;a6=0;a7=0;
        b0=0;b1=0;b2=0;b3=0;b4=0;b5=0;b6=0;b7=0;
        repeat(5) @(posedge clk);
        rst = 0;

        if (numseeds > 0) begin
            for (integer k = 0; k < numseeds; k = k + 1) begin
                run_case(seedstart + k);
            end
        end else begin
            // Modo padrão: três seeds determinísticas
            run_case(1);
            run_case(42);
            run_case(2025);
        end

        $display("Todos os testes passaram.");
        $finish;
    end

endmodule
