//===-- AVRTargetStreamer.h - AVR Target Streamer --------------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_AVR_TARGET_STREAMER_H
# define LLVM_AVR_TARGET_STREAMER_H

# include "AVRConfig.h"

# include "llvm/MC/MCELFStreamer.h"
# include "llvm/MC/MCStreamer.h"

namespace llvm {

/**
 * A generic AVR target output stream.
 */
class AVRTargetStreamer : public MCTargetStreamer {
  public:
    explicit AVRTargetStreamer(MCStreamer & S);
};

/**
 * A target streamer for textual AVR assembly code.
 */
class AVRTargetAsmStreamer : public AVRTargetStreamer {
  public:
    explicit AVRTargetAsmStreamer(MCStreamer & S);
};

/**
 * A target streamer for an AVR ELF object file.
 */
class AVRTargetELFStreamer : public AVRTargetStreamer {
  public:
    explicit AVRTargetELFStreamer(MCStreamer &S, const MCSubtargetInfo &STI);

    /// Gets the ELF streamer.
    MCELFStreamer &getStreamer();

  private:
    const MCSubtargetInfo &STI;
};

} // end namespace llvm

#endif // LLVM_AVR_TARGET_STREAMER_H
