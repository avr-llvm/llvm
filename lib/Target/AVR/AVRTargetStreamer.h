//===-- AVRTargetStreamer.h - AVR Target Streamer ----------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR_SPARCTARGETSTREAMER_H
#define LLVM_LIB_TARGET_AVR_SPARCTARGETSTREAMER_H

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCStreamer.h"

namespace llvm {
class AVRTargetStreamer : public MCTargetStreamer {
  virtual void anchor();

public:
  AVRTargetStreamer(MCStreamer &S);
};

// This part is for ascii assembly output
class AVRTargetAsmStreamer : public AVRTargetStreamer {
  formatted_raw_ostream &OS;

public:
  AVRTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
};

// This part is for ELF object output
class AVRTargetELFStreamer : public AVRTargetStreamer {
public:
  AVRTargetELFStreamer(MCStreamer &S);
  MCELFStreamer &getStreamer();
};
} // end namespace llvm

#endif
