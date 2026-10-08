#include "MiniRV.h"
#include "llvm/MC/MCAsmInfoELF.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"

#define GET_INSTRINFO_MC_DESC
#include "MiniRVGenInstrInfo.inc"
#define GET_REGINFO_MC_DESC
#include "MiniRVGenRegisterInfo.inc"
#define GET_SUBTARGETINFO_MC_DESC
#include "MiniRVGenSubtargetInfo.inc"

using namespace llvm;

Target &llvm::getTheMiniRVTarget() {
  static Target TheTarget;
  return TheTarget;
}

extern "C" void LLVMInitializeMiniRVTargetInfo() {
  RegisterTarget<Triple::riscv32> X(
      getTheMiniRVTarget(), "minirv", "Minimal RV32I teaching target", "MiniRV");
}

namespace {
class MiniRVMCAsmInfo : public MCAsmInfoELF {
public:
  MiniRVMCAsmInfo(const Triple &, const MCTargetOptions &) {
    CodePointerSize = 4;
    CalleeSaveStackSlotSize = 4;
  }
};

class MiniRVInstPrinter : public MCInstPrinter {
public:
  MiniRVInstPrinter(const MCAsmInfo &MAI, const MCInstrInfo &MII,
                    const MCRegisterInfo &MRI)
      : MCInstPrinter(MAI, MII, MRI) {}
  void printInst(const MCInst *MI, uint64_t Address, StringRef Annot,
                 const MCSubtargetInfo &STI, raw_ostream &O) override;
  void printOperand(const MCInst *MI, unsigned OpNo, raw_ostream &O);
  std::pair<const char *, uint64_t>
  getMnemonic(const MCInst &MI) const override;
  void printInstruction(const MCInst *MI, uint64_t Address, raw_ostream &O);
  bool printAliasInstr(const MCInst *MI, uint64_t Address, raw_ostream &O);
  void printCustomAliasOperand(const MCInst *, uint64_t, unsigned, unsigned,
                               raw_ostream &);
  static const char *getRegisterName(MCRegister Reg);
  void printRegName(raw_ostream &OS, MCRegister Reg) override {
    OS << getRegisterName(Reg);
  }
};
}

#include "MiniRVGenAsmWriter.inc"

void MiniRVInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O) {
  const MCOperand &Op = MI->getOperand(OpNo);
  if (Op.isReg())
    O << getRegisterName(Op.getReg());
  else if (Op.isImm())
    O << Op.getImm();
  else
    report_fatal_error("MiniRV: unsupported MC operand");
}

void MiniRVInstPrinter::printInst(const MCInst *MI, uint64_t Address,
    StringRef Annot, const MCSubtargetInfo &, raw_ostream &O) {
  printInstruction(MI, Address, O);
  printAnnotation(O, Annot);
}

void MiniRVInstPrinter::printCustomAliasOperand(
    const MCInst *, uint64_t, unsigned, unsigned, raw_ostream &) {
  report_fatal_error("MiniRV: no custom aliases");
}

static MCInstrInfo *createMiniRVMCInstrInfo() {
  auto *Info = new MCInstrInfo();
  InitMiniRVMCInstrInfo(Info);
  return Info;
}
static MCRegisterInfo *createMiniRVMCRegisterInfo(const Triple &) {
  auto *Info = new MCRegisterInfo();
  InitMiniRVMCRegisterInfo(Info, MiniRV::X1);
  return Info;
}
static MCSubtargetInfo *createMiniRVMCSubtargetInfo(
    const Triple &TT, StringRef CPU, StringRef FS) {
  if (CPU.empty())
    CPU = "generic";
  return createMiniRVMCSubtargetInfoImpl(TT, CPU, CPU, FS);
}
static MCInstPrinter *createMiniRVMCInstPrinter(
    const Triple &, unsigned, const MCAsmInfo &MAI,
    const MCInstrInfo &MII, const MCRegisterInfo &MRI) {
  return new MiniRVInstPrinter(MAI, MII, MRI);
}

extern "C" void LLVMInitializeMiniRVTargetMC() {
  Target &T = getTheMiniRVTarget();
  RegisterMCAsmInfo<MiniRVMCAsmInfo> AsmInfo(T);
  TargetRegistry::RegisterMCInstrInfo(T, createMiniRVMCInstrInfo);
  TargetRegistry::RegisterMCRegInfo(T, createMiniRVMCRegisterInfo);
  TargetRegistry::RegisterMCSubtargetInfo(T, createMiniRVMCSubtargetInfo);
  TargetRegistry::RegisterMCInstPrinter(T, createMiniRVMCInstPrinter);
}
