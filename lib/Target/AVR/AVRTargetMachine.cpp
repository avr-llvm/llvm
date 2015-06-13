//===-- AVRTargetMachine.cpp - Define TargetMachine for AVR ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the AVR specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#include "AVRTargetMachine.h"

#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetRegistry.h"

#include "AVRTargetObjectFile.h"
#include "AVR.h"

namespace llvm {

namespace
{
    // The default CPU to choose if an empty string is passed.
    const char* DefaultCPU = "avr2";
}

AVRTargetMachine::AVRTargetMachine(const Target &T, StringRef TT, StringRef CPU,
                                   StringRef FS, const TargetOptions &Options,
                                   Reloc::Model RM, CodeModel::Model CM,
                                   CodeGenOpt::Level OL) :
  LLVMTargetMachine(T, "e-p:16:8:8-i8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-n8",
                    TT, CPU.empty() ? DefaultCPU : CPU, FS, Options, RM, CM, OL),
  SubTarget(TT, CPU, FS, *this)
{
  this->TLOF = make_unique<AVRTargetObjectFile>();
  initAsmInfo();
}

namespace
{
/// AVR Code Generator Pass Configuration Options.
class AVRPassConfig : public TargetPassConfig
{
public:
  AVRPassConfig(AVRTargetMachine *TM, PassManagerBase &PM) :
    TargetPassConfig(TM, PM) {}

  AVRTargetMachine &getAVRTargetMachine() const
  {
    return getTM<AVRTargetMachine>();
  }

  bool addInstSelector() override;
  void addPreSched2() override;
  void addPreRegAlloc() override;
  void addPreEmitPass() override;
};
} // namespace

TargetPassConfig *AVRTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new AVRPassConfig(this, PM);
}

extern "C" void LLVMInitializeAVRTarget() {
  // Register the target.
  RegisterTargetMachine<AVRTargetMachine> X(TheAVRTarget);
}

const AVRSubtarget *AVRTargetMachine::getSubtargetImpl() const
{
  return &SubTarget;
}

const AVRSubtarget *AVRTargetMachine::getSubtargetImpl(const Function&) const
{
  return &SubTarget;
}

//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

bool AVRPassConfig::addInstSelector()
{
  // Install an instruction selector.
  addPass(createAVRISelDag(getAVRTargetMachine(), getOptLevel()));
  // Create the frame analyzer pass used by the PEI pass.
  addPass(createAVRFrameAnalyzerPass());

  return false;
}

void AVRPassConfig::addPreRegAlloc()
{
  // Create the dynalloc SP save/restore pass to handle variable sized allocas.
  addPass(createAVRDynAllocaSRPass());
}

void AVRPassConfig::addPreSched2()
{
  addPass(createAVRExpandPseudoPass());
}

void AVRPassConfig::addPreEmitPass()
{
  // Must run branch selection immediately preceding the asm printer.
  addPass(createAVRBranchSelectionPass());
}

} // end of namespace llvm
