include "llvm/Target/Target.td"

include "MiniRVRegisterInfo.td"
include "MiniRVInstrInfo.td"

def MiniRVInstrInfo : InstrInfo;
def : Processor<"generic", NoItineraries, []>;

def MiniRVAsmWriter : AsmWriter {
  let AsmWriterClassName = "InstPrinter";
}

def MiniRV : Target {
  let InstructionSet = MiniRVInstrInfo;
  let AssemblyWriters = [MiniRVAsmWriter];
}
