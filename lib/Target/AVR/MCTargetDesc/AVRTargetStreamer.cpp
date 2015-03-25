//===-- AVRTargetStreamer.cpp - AVR Target Streamer Methods -----------===//
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
#include "InstPrinter/AVRInstPrinter.h"
#include "llvm/Support/FormattedStream.h"

using namespace llvm;

// pin vtable to this file
AVRTargetStreamer::AVRTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

void AVRTargetStreamer::anchor() {}

AVRTargetAsmStreamer::AVRTargetAsmStreamer(MCStreamer &S,
                                               formatted_raw_ostream &OS)
    : AVRTargetStreamer(S), OS(OS) {}

AVRTargetELFStreamer::AVRTargetELFStreamer(MCStreamer &S)
    : AVRTargetStreamer(S) {}

MCELFStreamer &AVRTargetELFStreamer::getStreamer() {
  return static_cast<MCELFStreamer &>(Streamer);
}
