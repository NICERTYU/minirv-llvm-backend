// RV32 register numbers and names match the standard RISC-V encoding.
let Namespace = "MiniRV" in {
  class RVReg<bits<5> Enc, string Name> : Register<Name> {
    let HWEncoding{4-0} = Enc;
  }
  foreach I = 0-31 in {
    def X#I : RVReg<I, "x"#I>, DwarfRegNum<[I]>;
  }
}

// Only caller-saved registers are allocatable in this first, leaf-only target.
def GPR : RegisterClass<"MiniRV", [i32], 32,
  (add (sequence "X%u", 5, 7), (sequence "X%u", 10, 17),
       (sequence "X%u", 28, 31))>;

def CSR_MiniRV : CalleeSavedRegs<(add X1, X2)>;
