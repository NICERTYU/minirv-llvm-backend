module tb_minirv;
    reg clk = 0;
    reg reset = 1;
    reg [31:0] arg0 = 0;
    reg [31:0] arg1 = 0;
    wire halted;
    wire illegal;
    wire [31:0] result;
    wire [31:0] pc;
    integer expected;
    integer cycles;
    integer i;
    reg [1023:0] hex_file;

    minirv_cpu dut (
        .clk(clk), .reset(reset), .arg0(arg0), .arg1(arg1),
        .halted(halted), .illegal(illegal), .result(result), .pc(pc)
    );
    always #5 clk = ~clk;

    initial begin
        for (i = 0; i < 256; i = i + 1) dut.imem[i] = 32'h00000013;
        for (i = 0; i < 1024; i = i + 1) dut.dmem[i] = 0;
        if (!$value$plusargs("hex=%s", hex_file)) $fatal(1, "missing hex file");
        $readmemh(hex_file, dut.imem);
        if (!$value$plusargs("arg0=%d", arg0)) $fatal(1, "missing arg0");
        if (!$value$plusargs("arg1=%d", arg1)) $fatal(1, "missing arg1");
        if (!$value$plusargs("expected=%d", expected)) $fatal(1, "missing expected");
        repeat (2) @(negedge clk);
        reset = 0;
        for (cycles = 0; cycles < 100; cycles = cycles + 1) begin
            @(negedge clk);
            if (halted) begin
                if (illegal) $fatal(1, "illegal instruction at pc=%h", pc);
                if ($signed(result) !== expected)
                    $fatal(1, "result=%0d expected=%0d", $signed(result), expected);
                $display("PASS result=%0d cycles=%0d", $signed(result), cycles + 1);
                $finish;
            end
        end
        $fatal(1, "timeout at pc=%h", pc);
    end
endmodule
