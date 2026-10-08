// Machine instructions are deliberately RV32I-compatible so GNU as can
// assemble the output and the Verilog CPU can execute it.
class RVInst<dag Outs, dag Ins, string Asm, list<dag> Pats>
    : Instruction {
  let Namespace = "MiniRV";
  let OutOperandList = Outs;
  let InOperandList = Ins;
  let AsmString = Asm;
  let Pattern = Pats;
  let Size = 4;
}

def simm12 : Operand<i32>, ImmLeaf<i32, [{ return isInt<12>(Imm); }]>;
def shamt  : Operand<i32>, ImmLeaf<i32, [{ return isUInt<5>(Imm); }]>;

def ADD : RVInst<(outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                 "add $rd, $rs1, $rs2",
                 [(set GPR:$rd, (add GPR:$rs1, GPR:$rs2))]>;
def SUB : RVInst<(outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                 "sub $rd, $rs1, $rs2",
                 [(set GPR:$rd, (sub GPR:$rs1, GPR:$rs2))]>;
def ADDI : RVInst<(outs GPR:$rd), (ins GPR:$rs1, simm12:$imm),
                  "addi $rd, $rs1, $imm",
                  [(set GPR:$rd, (add GPR:$rs1, simm12:$imm))]>;
def SLLI : RVInst<(outs GPR:$rd), (ins GPR:$rs1, shamt:$imm),
                  "slli $rd, $rs1, $imm",
                  [(set GPR:$rd, (shl GPR:$rs1, shamt:$imm))]>;
def AND : RVInst<(outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                 "and $rd, $rs1, $rs2",
                 [(set GPR:$rd, (and GPR:$rs1, GPR:$rs2))]>;
def OR : RVInst<(outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                "or $rd, $rs1, $rs2",
                [(set GPR:$rd, (or GPR:$rs1, GPR:$rs2))]>;
def XOR : RVInst<(outs GPR:$rd), (ins GPR:$rs1, GPR:$rs2),
                 "xor $rd, $rs1, $rs2",
                 [(set GPR:$rd, (xor GPR:$rs1, GPR:$rs2))]>;

def : Pat<(i32 simm12:$imm), (ADDI X0, simm12:$imm)>;

def retglue : SDNode<"MiniRVISD::RET_GLUE", SDTNone,
                     [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
let isReturn = 1, isTerminator = 1, isBarrier = 1,
    Uses = [X1] in
def RET : RVInst<(outs), (ins), "ret", [(retglue)]>;
