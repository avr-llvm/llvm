//===-- AVRAsmBackend.cpp - Mips Asm Backend  ----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AVRAsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#include "MCTargetDesc/AVRAsmBackend.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Prepare value for the target space for it
static unsigned adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext *Ctx = nullptr) {

  unsigned Kind = Fixup.getKind();

  // Add/subtract and shift
  switch (Kind) {
  default:
    return 0;
  }

  return Value;
}

MCObjectWriter *AVRAsmBackend::createObjectWriter(raw_ostream &OS) const {
  return NULL;
  // TODO[avr-obj]: implement.
  //return createAVRELFObjectWriter(OS,
  //  MCELFObjectTargetWriter::getOSABI(OSType));
}

/// ApplyFixup - Apply the \p Value for given \p Fixup into the provided
/// data fragment, at the offset specified by the fixup and following the
/// fixup kind as appropriate.
void AVRAsmBackend::applyFixup(const MCFixup &Fixup, char *Data,
                                unsigned DataSize, uint64_t Value,
                                bool IsPCRel) const {
  MCFixupKind Kind = Fixup.getKind();
  Value = adjustFixupValue(Fixup, Value);

  if (!Value)
    return; // Doesn't change encoding.

  // Used to point to big endian bytes
  unsigned FullSize;

  switch ((unsigned)Kind) {
  case FK_Data_2:
    FullSize = 2;
    break;
  case FK_Data_8:
    FullSize = 8;
    break;
  case FK_Data_4:
  default:
    FullSize = 4;
    break;
  }
  
  // I'm not sure how to modify this function so put this here.
  llvm_unreachable("this is an AVR-LLVM bug");
}

/// WriteNopData - Write an (optimal) nop sequence of Count bytes
/// to the given output. If the target cannot generate such a sequence,
/// it should return an error.
///
/// \return - True on success.
bool AVRAsmBackend::writeNopData(uint64_t Count, MCObjectWriter *OW) const {
  // Check for a less than instruction size number of bytes
  // FIXME: 16 bit instructions are not handled yet here.
  // We shouldn't be using a hard coded number for instruction size.

  // If the count is not 4-byte aligned, we must be writing data into the text
  // section (otherwise we have unaligned instructions, and thus have far
  // bigger problems), so just write zeros instead.
  for (uint64_t i = 0, e = Count % 4; i != e; ++i)
    OW->Write8(0);

  uint64_t NumNops = Count / 4;
  for (uint64_t i = 0; i != NumNops; ++i)
    OW->Write32(0);
  return true;
}

MCAsmBackend *llvm::createAVRAsmBackend(const Target &T,
                                             const MCRegisterInfo &MRI,
                                             StringRef TT,
                                             StringRef CPU) {
  return new AVRAsmBackend(T);
}
