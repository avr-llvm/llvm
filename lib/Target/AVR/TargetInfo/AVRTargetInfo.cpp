//===-- AVRTargetInfo.cpp - AVR Target Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Module.h"
#include "llvm/Support/TargetRegistry.h"

#include "AVR.h"

using namespace llvm;

Target llvm::TheAVRTarget;

extern "C" void LLVMInitializeAVRTargetInfo()
{
  RegisterTarget<Triple::avr> X(TheAVRTarget, "avr",
                                "Atmel AVR Microcontroller [experimental]");
}
