#include "MiniRV.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/InitializePasses.h"

using namespace llvm;

#define DEBUG_TYPE "minirv-isel"

namespace {
class MiniRVDAGToDAGISel : public SelectionDAGISel {
public:
  MiniRVDAGToDAGISel(MiniRVTargetMachine &TM, CodeGenOptLevel OL)
      : SelectionDAGISel(TM, OL) {}
  void Select(SDNode *N) override {
    if (N->isMachineOpcode())
      return;
    SelectCode(N);
  }
#include "MiniRVGenDAGISel.inc"
};

class MiniRVDAGToDAGISelLegacy : public SelectionDAGISelLegacy {
public:
  static char ID;
  MiniRVDAGToDAGISelLegacy(MiniRVTargetMachine &TM, CodeGenOptLevel OL)
      : SelectionDAGISelLegacy(
            ID, std::make_unique<MiniRVDAGToDAGISel>(TM, OL)) {}
};
}

char MiniRVDAGToDAGISelLegacy::ID;
INITIALIZE_PASS(MiniRVDAGToDAGISelLegacy, DEBUG_TYPE,
                "MiniRV DAG instruction selection", false, false)

FunctionPass *llvm::createMiniRVISelDag(MiniRVTargetMachine &TM,
                                        CodeGenOptLevel OL) {
  return new MiniRVDAGToDAGISelLegacy(TM, OL);
}
