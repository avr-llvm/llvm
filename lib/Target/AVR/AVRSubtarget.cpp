//===-- AVRSubtarget.cpp - AVR Subtarget Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AVR specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "AVRSubtarget.h"
#include "AVR.h"
#include "llvm/Support/TargetRegistry.h"

#define DEBUG_TYPE "avr-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVRGenSubtargetInfo.inc"

using namespace llvm;

void AVRSubtarget::anchor() { }

AVRSubtarget::AVRSubtarget(const std::string &TT, const std::string &CPU,
                           const std::string &FS, AVRTargetMachine &TM) :
  AVRGenSubtargetInfo(TT, CPU, FS),
  IsAsmOnly(false),
  DL("e-p:16:8:8-i8:8:8-i16:8:8-i32:8:8-i64:8:8-f32:8:8-f64:8:8-n8"),
  InstrInfo(),
  FrameLowering(),
  TLInfo(TM),
  TSInfo(TM)
{
  //:FIXME: implement all subtarget stuff here
  //std::string CPU = "generic";

  // Parse features string.
  //ParseSubtargetFeatures(FS, CPU);
}

