#include "MiniRV.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

extern "C" void LLVMInitializeMiniRVTargetInfo();
extern "C" void LLVMInitializeMiniRVTarget();
extern "C" void LLVMInitializeMiniRVTargetMC();
extern "C" void LLVMInitializeMiniRVAsmPrinter();

static cl::opt<std::string> Input(cl::Positional, cl::Required,
                                  cl::desc("<input.ll>"));
static cl::opt<std::string> Output("o", cl::Required,
                                   cl::desc("output assembly file"));
static cl::opt<std::string> TripleOption("mtriple",
                                         cl::init("riscv32-unknown-elf"));
static cl::opt<bool> O0("O0", cl::init(false));

int main(int argc, char **argv) {
  InitLLVM Init(argc, argv);
  cl::ParseCommandLineOptions(argc, argv, "MiniRV LLVM code generator\n");
  LLVMInitializeMiniRVTargetInfo();
  LLVMInitializeMiniRVTarget();
  LLVMInitializeMiniRVTargetMC();
  LLVMInitializeMiniRVAsmPrinter();

  LLVMContext Context;
  SMDiagnostic Error;
  std::unique_ptr<Module> M = parseIRFile(Input, Error, Context);
  if (!M) {
    Error.print(argv[0], errs());
    return 1;
  }
  Triple TT(TripleOption);
  std::string ErrorText;
  const Target *T = TargetRegistry::lookupTarget("minirv", TT, ErrorText);
  if (!T) {
    errs() << ErrorText << '\n';
    return 1;
  }
  TargetOptions Options;
  std::unique_ptr<TargetMachine> TM(T->createTargetMachine(
      TT, "generic", "", Options, Reloc::Static, CodeModel::Small,
      CodeGenOptLevel::None));
  M->setTargetTriple(TT);
  M->setDataLayout(TM->createDataLayout());
  std::error_code EC;
  raw_fd_ostream Out(Output, EC, sys::fs::OF_None);
  if (EC) {
    errs() << EC.message() << '\n';
    return 1;
  }
  legacy::PassManager PM;
  if (TM->addPassesToEmitFile(PM, Out, nullptr,
                              CodeGenFileType::AssemblyFile)) {
    errs() << "MiniRV cannot emit assembly\n";
    return 1;
  }
  PM.run(*M);
  Out.flush();
  return 0;
}
