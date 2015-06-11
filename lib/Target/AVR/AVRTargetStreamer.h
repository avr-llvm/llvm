//===-- AVRTargetStreamer.h - AVR Target Streamer --------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AVR_AVRTARGETSTREAMER_H
#define LLVM_LIB_TARGET_AVR_AVRTARGETSTREAMER_H

#include "llvm/MC/MCELFStreamer.h"
#include "llvm/MC/MCStreamer.h"

namespace llvm {
class AVRTargetStreamer : public MCTargetStreamer {

public:
  explicit AVRTargetStreamer(MCStreamer &S);
};

// This part is for ascii assembly output
class AVRTargetAsmStreamer : public AVRTargetStreamer {

public:
  AVRTargetAsmStreamer(MCStreamer &);
};

// This part is for ELF object output
class AVRTargetELFStreamer : public AVRTargetStreamer {
public:
  explicit AVRTargetELFStreamer(MCStreamer &S);
  MCELFStreamer &getStreamer();
};
} // end namespace llvm

#endif
