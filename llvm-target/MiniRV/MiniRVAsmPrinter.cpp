#include "MiniRV.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class MiniRVAsmPrinter : public AsmPrinter {
public:
  static char ID;
  MiniRVAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer), ID) {}
  StringRef getPassName() const override { return "MiniRV assembly printer"; }
  void emitInstruction(const MachineInstr *MI) override {
    MCInst Inst;
    Inst.setOpcode(MI->getOpcode());
    for (const MachineOperand &Op : MI->operands()) {
      if (Op.isReg() && Op.isImplicit())
        continue;
      if (Op.isReg())
        Inst.addOperand(MCOperand::createReg(Op.getReg()));
      else if (Op.isImm())
        Inst.addOperand(MCOperand::createImm(Op.getImm()));
      else
        report_fatal_error("MiniRV: unsupported machine operand");
    }
    EmitToStreamer(*OutStreamer, Inst);
  }
};
}

char MiniRVAsmPrinter::ID = 0;
extern "C" void LLVMInitializeMiniRVAsmPrinter() {
  RegisterAsmPrinter<MiniRVAsmPrinter> X(getTheMiniRVTarget());
}
