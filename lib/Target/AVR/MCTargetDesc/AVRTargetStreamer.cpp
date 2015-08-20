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

#include "llvm/Support/ELF.h"
#include "llvm/Support/FormattedStream.h"

#include "InstPrinter/AVRInstPrinter.h"
#include "AVRSubtarget.h"

namespace llvm {

// pin vtable to this file
AVRTargetStreamer::AVRTargetStreamer(MCStreamer &S) : MCTargetStreamer(S) {}

AVRTargetAsmStreamer::AVRTargetAsmStreamer(MCStreamer &S)
  : AVRTargetStreamer(S) {}

AVRTargetELFStreamer::AVRTargetELFStreamer(MCStreamer &S,
                                           const MCSubtargetInfo &STI)
                                          : AVRTargetStreamer(S), STI(STI) {

  MCAssembler &MCA = getStreamer().getAssembler();

  const FeatureBitset &Features = STI.getFeatureBits();

  // Set e_flags
  {
    unsigned EFlags = MCA.getELFHeaderEFlags();

    // Set architecture
    if      (Features[AVR::ELFArchAVR1])    EFlags |= ELF::EF_AVR_ARCH_AVR1;
    else if (Features[AVR::ELFArchAVR2])    EFlags |= ELF::EF_AVR_ARCH_AVR2;
    else if (Features[AVR::ELFArchAVR25])   EFlags |= ELF::EF_AVR_ARCH_AVR25;
    else if (Features[AVR::ELFArchAVR3])    EFlags |= ELF::EF_AVR_ARCH_AVR3;
    else if (Features[AVR::ELFArchAVR31])   EFlags |= ELF::EF_AVR_ARCH_AVR31;
    else if (Features[AVR::ELFArchAVR35])   EFlags |= ELF::EF_AVR_ARCH_AVR35;
    else if (Features[AVR::ELFArchAVR4])    EFlags |= ELF::EF_AVR_ARCH_AVR4;
    else if (Features[AVR::ELFArchAVR5])    EFlags |= ELF::EF_AVR_ARCH_AVR5;
    else if (Features[AVR::ELFArchAVR51])   EFlags |= ELF::EF_AVR_ARCH_AVR51;
    else if (Features[AVR::ELFArchAVR6])    EFlags |= ELF::EF_AVR_ARCH_AVR6;
    else if (Features[AVR::ELFArchAVRTiny]) EFlags |= ELF::EF_AVR_ARCH_AVRTINY;
    else if (Features[AVR::ELFArchXMEGA1])  EFlags |= ELF::EF_AVR_ARCH_XMEGA1;
    else if (Features[AVR::ELFArchXMEGA2])  EFlags |= ELF::EF_AVR_ARCH_XMEGA2;
    else if (Features[AVR::ELFArchXMEGA3])  EFlags |= ELF::EF_AVR_ARCH_XMEGA3;
    else if (Features[AVR::ELFArchXMEGA4])  EFlags |= ELF::EF_AVR_ARCH_XMEGA4;
    else if (Features[AVR::ELFArchXMEGA5])  EFlags |= ELF::EF_AVR_ARCH_XMEGA5;
    else if (Features[AVR::ELFArchXMEGA6])  EFlags |= ELF::EF_AVR_ARCH_XMEGA6;
    else if (Features[AVR::ELFArchXMEGA7])  EFlags |= ELF::EF_AVR_ARCH_XMEGA7;

    MCA.setELFHeaderEFlags(EFlags);
  }
}
} // end namespace llvm
