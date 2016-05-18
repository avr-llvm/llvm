//===-- LanaiTargetMachine.cpp - Define TargetMachine for Lanai ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Implements the info about Lanai target spec.
//
//===----------------------------------------------------------------------===//

#include "LanaiTargetMachine.h"

#include "Lanai.h"
#include "LanaiTargetObjectFile.h"
#include "LanaiTargetTransformInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"

using namespace llvm;

namespace llvm {
void initializeLanaiMemAluCombinerPass(PassRegistry &);
void initializeLanaiSetflagAluCombinerPass(PassRegistry &);
} // namespace llvm

extern "C" void LLVMInitializeLanaiTarget() {
  // Register the target.
  RegisterTargetMachine<LanaiTargetMachine> registered_target(TheLanaiTarget);
}

static std::string computeDataLayout(const Triple &TT) {
  // Data layout (keep in sync with clang/lib/Basic/Targets.cpp)
  return "E"        // Big endian
         "-m:e"     // ELF name manging
         "-p:32:32" // 32-bit pointers, 32 bit aligned
         "-i64:64"  // 64 bit integers, 64 bit aligned
         "-a:0:32"  // 32 bit alignment of objects of aggregate type
         "-n32"     // 32 bit native integer width
         "-S64";    // 64 bit natural stack alignment
}

LanaiTargetMachine::LanaiTargetMachine(const Target &TheTarget,
                                       const Triple &TargetTriple,
                                       StringRef Cpu, StringRef FeatureString,
                                       const TargetOptions &Options,
                                       Reloc::Model RelocationModel,
                                       CodeModel::Model CodeModel,
                                       CodeGenOpt::Level OptLevel)
    : LLVMTargetMachine(TheTarget, computeDataLayout(TargetTriple),
                        TargetTriple, Cpu, FeatureString, Options,
                        RelocationModel, CodeModel, OptLevel),
      Subtarget(TargetTriple, Cpu, FeatureString, *this, Options,
                RelocationModel, CodeModel, OptLevel),
      TLOF(new LanaiTargetObjectFile()) {
  initAsmInfo();
}

TargetIRAnalysis LanaiTargetMachine::getTargetIRAnalysis() {
  return TargetIRAnalysis([this](const Function &F) {
    return TargetTransformInfo(LanaiTTIImpl(this, F));
  });
}

namespace {
// Lanai Code Generator Pass Configuration Options.
class LanaiPassConfig : public TargetPassConfig {
public:
  LanaiPassConfig(LanaiTargetMachine *TM, PassManagerBase *PassManager)
      : TargetPassConfig(TM, *PassManager) {}

  LanaiTargetMachine &getLanaiTargetMachine() const {
    return getTM<LanaiTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreSched2() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *
LanaiTargetMachine::createPassConfig(PassManagerBase &PassManager) {
  return new LanaiPassConfig(this, &PassManager);
}

// Install an instruction selector pass.
bool LanaiPassConfig::addInstSelector() {
  addPass(createLanaiISelDag(getLanaiTargetMachine()));
  return false;
}

// Implemented by targets that want to run passes immediately before
// machine code is emitted.
void LanaiPassConfig::addPreEmitPass() {
  addPass(createLanaiDelaySlotFillerPass(getLanaiTargetMachine()));
}

// Run passes after prolog-epilog insertion and before the second instruction
// scheduling pass.
void LanaiPassConfig::addPreSched2() {
  addPass(createLanaiMemAluCombinerPass());
  addPass(createLanaiSetflagAluCombinerPass());
}
