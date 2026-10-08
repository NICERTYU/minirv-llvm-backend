#ifndef MINIRV_TARGET_H
#define MINIRV_TARGET_H

#include "llvm/CodeGen/CodeGenTargetMachineImpl.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetRegisterInfo.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/SelectionDAGTargetInfo.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>
#include <optional>

#define GET_REGINFO_ENUM
#include "MiniRVGenRegisterInfo.inc"
#define GET_INSTRINFO_ENUM
#include "MiniRVGenInstrInfo.inc"
#define GET_SUBTARGETINFO_ENUM
#include "MiniRVGenSubtargetInfo.inc"

#define GET_REGINFO_HEADER
#include "MiniRVGenRegisterInfo.inc"
#define GET_INSTRINFO_HEADER
#include "MiniRVGenInstrInfo.inc"
#define GET_SUBTARGETINFO_HEADER
#include "MiniRVGenSubtargetInfo.inc"

namespace llvm {

class MiniRVSubtarget;

namespace MiniRVISD {
enum NodeType { RET_GLUE = ISD::BUILTIN_OP_END };
}

class MiniRVRegisterInfo : public MiniRVGenRegisterInfo {
public:
  MiniRVRegisterInfo();
  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF) const override;
  const uint32_t *getCallPreservedMask(const MachineFunction &MF,
                                       CallingConv::ID CC) const override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  bool eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override;
  Register getFrameRegister(const MachineFunction &MF) const override;
};

class MiniRVInstrInfo : public MiniRVGenInstrInfo {
  MiniRVRegisterInfo RI;
public:
  MiniRVInstrInfo();
  const MiniRVRegisterInfo &getRegisterInfo() const { return RI; }
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator I,
                   const DebugLoc &DL, Register DestReg, Register SrcReg,
                   bool KillSrc, bool RenamableDest = false,
                   bool RenamableSrc = false) const override;
  void storeRegToStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register SrcReg,
      bool IsKill, int FrameIndex, const TargetRegisterClass *RC,
      const TargetRegisterInfo *TRI, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
  void loadRegFromStackSlot(
      MachineBasicBlock &MBB, MachineBasicBlock::iterator I, Register DestReg,
      int FrameIndex, const TargetRegisterClass *RC,
      const TargetRegisterInfo *TRI, Register VReg,
      MachineInstr::MIFlag Flags = MachineInstr::NoFlags) const override;
};

class MiniRVFrameLowering : public TargetFrameLowering {
public:
  MiniRVFrameLowering()
      : TargetFrameLowering(StackGrowsDown, Align(16), 0, Align(16)) {}
  void emitPrologue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;
protected:
  bool hasFPImpl(const MachineFunction &MF) const override { return false; }
};

class MiniRVTargetLowering : public TargetLowering {
public:
  MiniRVTargetLowering(const TargetMachine &TM, const MiniRVSubtarget &STI);
  SDValue LowerFormalArguments(
      SDValue Chain, CallingConv::ID CC, bool IsVarArg,
      const SmallVectorImpl<ISD::InputArg> &Ins, const SDLoc &DL,
      SelectionDAG &DAG, SmallVectorImpl<SDValue> &InVals) const override;
  SDValue LowerReturn(
      SDValue Chain, CallingConv::ID CC, bool IsVarArg,
      const SmallVectorImpl<ISD::OutputArg> &Outs,
      const SmallVectorImpl<SDValue> &OutVals, const SDLoc &DL,
      SelectionDAG &DAG) const override;
};

class MiniRVSubtarget : public MiniRVGenSubtargetInfo {
  MiniRVInstrInfo InstrInfo;
  MiniRVFrameLowering FrameLowering;
  MiniRVTargetLowering TLInfo;
  SelectionDAGTargetInfo DAGInfo;
public:
  MiniRVSubtarget(const Triple &TT, StringRef CPU, StringRef FS,
                  const TargetMachine &TM);
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);
  const MiniRVInstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const MiniRVRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }
  const MiniRVFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const MiniRVTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const SelectionDAGTargetInfo *getSelectionDAGInfo() const override {
    return &DAGInfo;
  }
};

class MiniRVTargetMachine : public CodeGenTargetMachineImpl {
  std::unique_ptr<TargetLoweringObjectFile> TLOF;
  MiniRVSubtarget Subtarget;
public:
  MiniRVTargetMachine(const Target &T, const Triple &TT, StringRef CPU,
                      StringRef FS, const TargetOptions &Options,
                      std::optional<Reloc::Model> RM,
                      std::optional<CodeModel::Model> CM,
                      CodeGenOptLevel OL, bool JIT);
  const MiniRVSubtarget *getSubtargetImpl(const Function &F) const override {
    return &Subtarget;
  }
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;
  TargetLoweringObjectFile *getObjFileLowering() const override {
    return TLOF.get();
  }
};

Target &getTheMiniRVTarget();
FunctionPass *createMiniRVISelDag(MiniRVTargetMachine &TM,
                                   CodeGenOptLevel OptLevel);
void initializeMiniRVDAGToDAGISelLegacyPass(PassRegistry &);
}
#endif
