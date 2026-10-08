// Single-cycle educational RV32I subset. Each clock retires one instruction.
// Instruction and data memories are separate to keep the datapath visible.
module minirv_cpu (
    input  wire        clk,
    input  wire        reset,
    input  wire [31:0] arg0,
    input  wire [31:0] arg1,
    output reg         halted,
    output reg         illegal,
    output reg  [31:0] result,
    output reg  [31:0] pc
);
    localparam [31:0] HALT_PC = 32'hffff_fffc;
    reg [31:0] imem [0:255];
    reg [31:0] dmem [0:1023];
    reg [31:0] regs [0:31];
    integer i;

    wire [31:0] inst = imem[pc[9:2]];
    wire [6:0] opcode = inst[6:0];
    wire [4:0] rd = inst[11:7];
    wire [2:0] funct3 = inst[14:12];
    wire [4:0] rs1 = inst[19:15];
    wire [4:0] rs2 = inst[24:20];
    wire [6:0] funct7 = inst[31:25];
    wire [31:0] a = regs[rs1];
    wire [31:0] b = regs[rs2];
    wire [31:0] imm_i = {{20{inst[31]}}, inst[31:20]};
    wire [31:0] imm_s = {{20{inst[31]}}, inst[31:25], inst[11:7]};
    wire [31:0] imm_b = {{19{inst[31]}}, inst[31], inst[7],
                         inst[30:25], inst[11:8], 1'b0};
    wire [31:0] imm_u = {inst[31:12], 12'b0};
    wire [31:0] imm_j = {{11{inst[31]}}, inst[31], inst[19:12],
                         inst[20], inst[30:21], 1'b0};
    reg [31:0] next_pc;
    reg [31:0] write_value;
    reg [31:0] address;
    reg write_reg;
    reg write_mem;
    reg bad_instruction;

    // Decode and combinational datapath.
    always @* begin
        next_pc = pc + 4;
        write_value = 0;
        address = 0;
        write_reg = 0;
        write_mem = 0;
        bad_instruction = 0;
        case (opcode)
            7'b0110011: begin // R type
                write_reg = 1;
                case ({funct7, funct3})
                    {7'b0000000, 3'b000}: write_value = a + b;
                    {7'b0100000, 3'b000}: write_value = a - b;
                    {7'b0000000, 3'b111}: write_value = a & b;
                    {7'b0000000, 3'b110}: write_value = a | b;
                    {7'b0000000, 3'b100}: write_value = a ^ b;
                    {7'b0000000, 3'b001}: write_value = a << b[4:0];
                    {7'b0000000, 3'b101}: write_value = a >> b[4:0];
                    {7'b0100000, 3'b101}: write_value = $signed(a) >>> b[4:0];
                    default: bad_instruction = 1;
                endcase
            end
            7'b0010011: begin // I type arithmetic
                write_reg = 1;
                case (funct3)
                    3'b000: write_value = a + imm_i;
                    3'b111: write_value = a & imm_i;
                    3'b110: write_value = a | imm_i;
                    3'b100: write_value = a ^ imm_i;
                    3'b001: begin
                        if (funct7 == 0) write_value = a << inst[24:20];
                        else bad_instruction = 1;
                    end
                    3'b101: begin
                        if (funct7 == 0) write_value = a >> inst[24:20];
                        else if (funct7 == 7'b0100000)
                            write_value = $signed(a) >>> inst[24:20];
                        else bad_instruction = 1;
                    end
                    default: bad_instruction = 1;
                endcase
            end
            7'b0000011: begin // lw
                if (funct3 == 3'b010) begin
                    address = a + imm_i;
                    write_reg = 1;
                    write_value = dmem[address[11:2]];
                end else bad_instruction = 1;
            end
            7'b0100011: begin // sw
                if (funct3 == 3'b010) begin
                    address = a + imm_s;
                    write_mem = 1;
                end else bad_instruction = 1;
            end
            7'b1100011: begin // conditional branches
                case (funct3)
                    3'b000: if (a == b) next_pc = pc + imm_b;
                    3'b001: if (a != b) next_pc = pc + imm_b;
                    3'b100: if ($signed(a) < $signed(b)) next_pc = pc + imm_b;
                    3'b101: if ($signed(a) >= $signed(b)) next_pc = pc + imm_b;
                    default: bad_instruction = 1;
                endcase
            end
            7'b1101111: begin // jal
                write_reg = 1;
                write_value = pc + 4;
                next_pc = pc + imm_j;
            end
            7'b1100111: begin // jalr, including ret
                if (funct3 == 0) begin
                    write_reg = 1;
                    write_value = pc + 4;
                    next_pc = (a + imm_i) & 32'hffff_fffe;
                end else bad_instruction = 1;
            end
            7'b0110111: begin write_reg = 1; write_value = imm_u; end // lui
            7'b0010111: begin write_reg = 1; write_value = pc + imm_u; end // auipc
            default: bad_instruction = 1;
        endcase
    end

    // Architectural state advances on the clock edge.
    always @(posedge clk) begin
        if (reset) begin
            pc <= 0;
            halted <= 0;
            illegal <= 0;
            result <= 0;
            for (i = 0; i < 32; i = i + 1) regs[i] <= 0;
            regs[1] <= HALT_PC; // return from the function ends simulation
            regs[2] <= 32'h00001000; // 16-byte aligned stack top
            regs[10] <= arg0;
            regs[11] <= arg1;
        end else if (!halted) begin
            if (pc == HALT_PC) begin
                halted <= 1;
                result <= regs[10];
            end else if (pc[1:0] != 0 || bad_instruction) begin
                halted <= 1;
                illegal <= 1;
            end else begin
                pc <= next_pc;
                if (write_reg && rd != 0) regs[rd] <= write_value;
                if (write_mem) dmem[address[11:2]] <= b;
            end
        end
    end
endmodule
