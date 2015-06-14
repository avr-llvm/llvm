//===-- AVRTargetInfo.cpp - AVR Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AVRConfig.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"

#include "AVR.h"

namespace llvm { Target TheAVRTarget; }

extern "C"
void
LLVMInitializeAVRTargetInfo() {
  llvm::RegisterTarget<llvm::Triple::avr> X(llvm::TheAVRTarget, "avr",
                                "Atmel AVR Microcontroller [experimental]");
}
