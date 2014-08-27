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

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVRGenSubtargetInfo.inc"

using namespace llvm;

void AVRSubtarget::anchor() { }

AVRSubtarget::AVRSubtarget(const std::string &TT, const std::string &CPU,
                           const std::string &FS) :
  AVRGenSubtargetInfo(TT, CPU, FS),
  IsAsmOnly(false)
{
  //:FIXME: implement all subtarget stuff here
  //std::string CPU = "generic";

  // Parse features string.
  //ParseSubtargetFeatures(FS, CPU);
}
