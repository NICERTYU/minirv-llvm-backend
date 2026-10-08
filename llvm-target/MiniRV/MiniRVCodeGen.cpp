#include "MiniRV.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Debug.h"

#define DEBUG_TYPE "minirv-codegen"

#define GET_REGINFO_TARGET_DESC
#include "MiniRVGenRegisterInfo.inc"
#define GET_INSTRINFO_CTOR_DTOR
#include "MiniRVGenInstrInfo.inc"
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "MiniRVGenSubtargetInfo.inc"

using namespace llvm;

MiniRVRegisterInfo::MiniRVRegisterInfo() : MiniRVGenRegisterInfo(MiniRV::X1) {}
const MCPhysReg *
MiniRVRegisterInfo::getCalleeSavedRegs(const MachineFunction *) const {
  return CSR_MiniRV_SaveList;
}
const uint32_t *
MiniRVRegisterInfo::getCallPreservedMask(const MachineFunction &,
                                         CallingConv::ID) const {
  return CSR_MiniRV_RegMask;
}
BitVector MiniRVRegisterInfo::getReservedRegs(const MachineFunction &) const {
  BitVector Reserved(getNumRegs());
  markSuperRegs(Reserved, MiniRV::X0);
  markSuperRegs(Reserved, MiniRV::X1);
  markSuperRegs(Reserved, MiniRV::X2);
  return Reserved;
}
bool MiniRVRegisterInfo::eliminateFrameIndex(MachineBasicBlock::iterator,
    int, unsigned, RegScavenger *) const {
  report_fatal_error("MiniRV: stack frames are not supported yet");
}
Register MiniRVRegisterInfo::getFrameRegister(const MachineFunction &) const {
  return MiniRV::X2;
}

MiniRVInstrInfo::MiniRVInstrInfo() : MiniRVGenInstrInfo(0, 0) {}
void MiniRVInstrInfo::copyPhysReg(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator I, const DebugLoc &DL, Register Dst,
    Register Src, bool Kill, bool, bool) const {
  BuildMI(MBB, I, DL, get(MiniRV::ADDI), Dst)
      .addReg(Src, getKillRegState(Kill)).addImm(0);
}
void MiniRVInstrInfo::storeRegToStackSlot(MachineBasicBlock &,
    MachineBasicBlock::iterator, Register, bool, int,
    const TargetRegisterClass *, const TargetRegisterInfo *, Register,
    MachineInstr::MIFlag) const {
  report_fatal_error("MiniRV: register spills are not supported yet");
}
void MiniRVInstrInfo::loadRegFromStackSlot(MachineBasicBlock &,
    MachineBasicBlock::iterator, Register, int,
    const TargetRegisterClass *, const TargetRegisterInfo *, Register,
    MachineInstr::MIFlag) const {
  report_fatal_error("MiniRV: register spills are not supported yet");
}

void MiniRVFrameLowering::emitPrologue(MachineFunction &MF,
                                       MachineBasicBlock &) const {
  if (MF.getFrameInfo().getStackSize())
    report_fatal_error("MiniRV: stack frames are not supported yet");
}
void MiniRVFrameLowering::emitEpilogue(MachineFunction &,
                                       MachineBasicBlock &) const {}

MiniRVTargetLowering::MiniRVTargetLowering(const TargetMachine &TM,
                                           const MiniRVSubtarget &STI)
    : TargetLowering(TM) {
  addRegisterClass(MVT::i32, &MiniRV::GPRRegClass);
  computeRegisterProperties(STI.getRegisterInfo());
  setStackPointerRegisterToSaveRestore(MiniRV::X2);
}

SDValue MiniRVTargetLowering::LowerFormalArguments(
    SDValue Chain, CallingConv::ID CC, bool IsVarArg,
    const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
    SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const {
  if (CC != CallingConv::C || IsVarArg || Ins.size() > 2)
    report_fatal_error("MiniRV: only C functions with up to two i32 args");
  static const MCPhysReg ArgRegs[] = {MiniRV::X10, MiniRV::X11};
  MachineRegisterInfo &MRI = DAG.getMachineFunction().getRegInfo();
  for (unsigned I = 0; I < Ins.size(); ++I) {
    if (Ins[I].VT != MVT::i32)
      report_fatal_error("MiniRV: only i32 arguments");
    Register VReg = MRI.createVirtualRegister(&MiniRV::GPRRegClass);
    MRI.addLiveIn(ArgRegs[I], VReg);
    SDValue Arg = DAG.getCopyFromReg(Chain, DL, VReg, MVT::i32);
    InVals.push_back(Arg);
    Chain = Arg.getValue(1);
  }
  return Chain;
}

SDValue MiniRVTargetLowering::LowerReturn(
    SDValue Chain, CallingConv::ID CC, bool IsVarArg,
    const SmallVectorImpl<ISD::OutputArg> &Outs,
    const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
    SelectionDAG &DAG) const {
  if (CC != CallingConv::C || IsVarArg || Outs.size() > 1 ||
      (!Outs.empty() && Outs[0].VT != MVT::i32))
    report_fatal_error("MiniRV: only i32 or void returns");
  SDValue Glue;
  SmallVector<SDValue, 3> RetOps;
  if (!OutVals.empty()) {
    Chain = DAG.getCopyToReg(Chain, DL, MiniRV::X10, OutVals[0], Glue);
    Glue = Chain.getValue(1);
  }
  RetOps.push_back(Chain);
  if (!OutVals.empty())
    RetOps.push_back(DAG.getRegister(MiniRV::X10, MVT::i32));
  if (Glue.getNode())
    RetOps.push_back(Glue);
  return DAG.getNode(MiniRVISD::RET_GLUE, DL, MVT::Other, RetOps);
}

MiniRVSubtarget::MiniRVSubtarget(const Triple &TT, StringRef CPU,
    StringRef FS, const TargetMachine &TM)
    : MiniRVGenSubtargetInfo(TT, CPU.empty() ? "generic" : CPU,
                              CPU.empty() ? "generic" : CPU, FS),
      TLInfo(TM, *this) {}

MiniRVTargetMachine::MiniRVTargetMachine(const Target &T, const Triple &TT,
    StringRef CPU, StringRef FS, const TargetOptions &Options,
    std::optional<Reloc::Model> RM, std::optional<CodeModel::Model> CM,
    CodeGenOptLevel OL, bool)
    : CodeGenTargetMachineImpl(
          T, "e-m:e-p:32:32-i64:64-n32-S128", TT, CPU, FS, Options,
          RM.value_or(Reloc::Static),
          getEffectiveCodeModel(CM, CodeModel::Small), OL),
      TLOF(std::make_unique<TargetLoweringObjectFileELF>()),
      Subtarget(TT, CPU, FS, *this) {
  initAsmInfo();
}

namespace {
class MiniRVPassConfig : public TargetPassConfig {
public:
  MiniRVPassConfig(MiniRVTargetMachine &TM, PassManagerBase &PM)
      : TargetPassConfig(TM, PM) {}
  bool addInstSelector() override {
    addPass(createMiniRVISelDag(getTM<MiniRVTargetMachine>(), getOptLevel()));
    return false;
  }
};
}
TargetPassConfig *MiniRVTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new MiniRVPassConfig(*this, PM);
}

extern "C" void LLVMInitializeMiniRVTarget() {
  RegisterTargetMachine<MiniRVTargetMachine> X(getTheMiniRVTarget());
  initializeMiniRVDAGToDAGISelLegacyPass(*PassRegistry::getPassRegistry());
}
