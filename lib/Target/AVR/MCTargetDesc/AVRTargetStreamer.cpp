//===-- AVRTargetStreamer.cpp - AVR Target Streamer Methods ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides AVR specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "AVRTargetStreamer.h"

#include "llvm/Support/FormattedStream.h"

#include "InstPrinter/AVRInstPrinter.h"

namespace llvm {

// pin vtable to this file
AVRTargetStreamer::AVRTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

AVRTargetAsmStreamer::AVRTargetAsmStreamer(MCStreamer &S)
  : AVRTargetStreamer(S) {}

AVRTargetELFStreamer::AVRTargetELFStreamer(MCStreamer &S,
                                           const MCSubtargetInfo &STI)
  : AVRTargetStreamer(S), STI(STI) {

  MCAssembler &MCA = getStreamer().getAssembler();
}

MCELFStreamer &AVRTargetELFStreamer::getStreamer() {
  return static_cast<MCELFStreamer&>(Streamer);
}
} // end namespace llvm
