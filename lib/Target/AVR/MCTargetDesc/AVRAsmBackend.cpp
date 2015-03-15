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

#include "MCTargetDesc/AVRFixupKinds.h"
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

// FIXME: we are using this function for all relative fixups.
// Is this what we should be doing?
// Also, this will fail with 32 bit instructions.
static unsigned adjustFixupRelCondbr(unsigned size,
                                  const MCFixup &Fixup, uint64_t Value,
                                  MCContext *Ctx = nullptr)
{
  // Take the size of the current instruction away.
  Value -= 2;

  // We now check if Value can be encoded as a 7-bit signed immediate.
  //if (!isIntN(size, Value) && Ctx)
  //  Ctx->FatalError(Fixup.getLoc(), "out of range brcond fixup");
    
  Value >>= 2;
  
  return Value;
}

// Prepare value for the target space for it
static unsigned adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext *Ctx = nullptr) {

  unsigned Kind = Fixup.getKind();

  // Add/subtract and shift
  switch (Kind) {
  default:
    return 0;
  case FK_Data_2:
  case FK_GPRel_4:
  case FK_Data_4:
  case FK_Data_8:
    break;
  case AVR::fixup_7_pcrel:
  {
    Value = adjustFixupRelCondbr(7, Fixup, Value, Ctx);
    break;
  }
  case AVR::fixup_13_pcrel:
  {
    Value = adjustFixupRelCondbr(13, Fixup, Value, Ctx);
    break;
  }
  }

  return Value;
}

MCObjectWriter *AVRAsmBackend::createObjectWriter(raw_ostream &OS) const {
  return createAVRELFObjectWriter(OS,
    MCELFObjectTargetWriter::getOSABI(OSType),
    IsLittle);
}

/// ApplyFixup - Apply the \p Value for given \p Fixup into the provided
/// data fragment, at the offset specified by the fixup and following the
/// fixup kind as appropriate.
void AVRAsmBackend::applyFixup(const MCFixup &Fixup, char *Data,
                                unsigned DataSize, uint64_t Value,
                                bool IsPCRel) const {
  
  unsigned NumBytes = 1;//getFixupKindNumBytes(Fixup.getKind());
  if (!Value)
    return; // Doesn't change encoding.
  MCFixupKindInfo Info = getFixupKindInfo(Fixup.getKind());
  // Apply any target-specific value adjustments.
  Value = adjustFixupValue(Fixup, Value);

  // Shift the value into position.
  Value <<= Info.TargetOffset;

  unsigned Offset = Fixup.getOffset();
  assert(Offset + NumBytes <= DataSize && "Invalid fixup offset!");

  // For each byte of the fragment that the fixup touches, mask in the
  // bits from the fixup value.
  for (unsigned i = 0; i != NumBytes; ++i)
    Data[Offset + i] |= uint8_t((Value >> (i * 8)) & 0xff);
}

const MCFixupKindInfo &AVRAsmBackend::
getFixupKindInfo(MCFixupKind Kind) const {
  const static MCFixupKindInfo Infos[AVR::NumTargetFixupKinds] = {
    // FIXME: all of the fixups are untested.
    // Make sure every one works correctly. Many are probably broken.
    //
    // This table *must* be in same the order of fixup_* kinds in
    // AVRFixupKinds.h.
    //
    // name                    offset  bits  flags
    { "fixup_32",              0,      32,   0 },

    { "fixup_7_pcrel",         0,      7,    MCFixupKindInfo::FKF_IsPCRel },
    // FIXME: change `bits` to 13
    { "fixup_13_pcrel",        0,      12,   MCFixupKindInfo::FKF_IsPCRel },

    { "fixup_16",              0,      16,   0 },
    { "fixup_16_pm",           0,      16,   0 },

    { "fixup_lo8_ldi",         0,      8,    0 },
    { "fixup_hi8_ldi",         0,      8,    0 },
    { "fixup_hh8_ldi",         0,      8,    0 },

    { "fixup_lo8_ldi_neg",     0,      8,    0 },
    { "fixup_hi8_ldi_neg",     0,      8,    0 },
    { "fixup_hh8_ldi_neg",     0,      8,    0 },
		
    { "fixup_lo8_ldi_pm",      0,      8,    0 },
    { "fixup_hi8_ldi_pm",      0,      8,    0 },
    { "fixup_hh8_ldi_pm",      0,      8,    0 },
    
    { "fixup_lo8_ldi_pm_neg",  0,      8,    0 },
    { "fixup_hi8_ldi_pm_neg",  0,      8,    0 },
    { "fixup_hh8_ldi_pm_neg",  0,      8,    0 },

    { "fixup_call",            0,      0xff, 0 },
    { "fixup_ldi",             0,      0xff, 0 },

    { "fixup_6",               0,      6,    0 },
    { "fixup_6_adiw",          0,      6,    0 },

    { "fixup_ms8_ldi",         0,      8,    0 },
    { "fixup_ms8_ldi_neg",     0,      8,    0 },

    { "fixup_lo8_ldi_gs",      0,      8,    0 },
    { "fixup_hi8_ldi_gs",      0,      8,    0 },

    { "fixup_8",               0,      8,    0 },
    { "fixup_8_lo8",           0,      8,    0 },
    { "fixup_8_hi8",           0,      8,    0 },
    { "fixup_8_hlo8",          0,      8,    0 },

    { "fixup_sym_diff",        0,      0xff, 0 },
    { "fixup_16_ldst",         0,      16,   0 },

    { "fixup_lds_sts_16",      0,      16,   0 },

    { "fixup_port6",           0,      0xff, 0 },
    { "fixup_port5",           0,      0xff, 0 },

  };

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
          "Invalid kind!");

  return Infos[Kind - FirstTargetFixupKind];
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
  // FIXME: I believe we are writing 4 null bytes for each NOP instead of 2.
  
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

/// processFixupValue - Target hook to process the literal value of a fixup
/// if necessary.
void AVRAsmBackend::processFixupValue(const MCAssembler &Asm,
                                      const MCAsmLayout &Layout,
                                      const MCFixup &Fixup,
                                      const MCFragment *DF,
                                      const MCValue &Target,
                                      uint64_t &Value,
                                      bool &IsResolved) {
  IsResolved = false;
  
  // At this point we'll ignore the value returned by adjustFixupValue as
  // we are only checking if the fixup can be applied correctly. We have
  // access to MCContext from here which allows us to report a fatal error
  // with *possibly* a source code location.
  Value = adjustFixupValue(Fixup, Value, &Asm.getContext());
}

MCAsmBackend *llvm::createAVRAsmBackendEL(const Target &T,
                                             const MCRegisterInfo &MRI,
                                             StringRef TT,
                                             StringRef CPU) {
  return new AVRAsmBackend(T, Triple(TT).getOS(), true);
}

MCAsmBackend *llvm::createAVRAsmBackendEB(const Target &T,
                                             const MCRegisterInfo &MRI,
                                             StringRef TT,
                                             StringRef CPU) {
  return new AVRAsmBackend(T, Triple(TT).getOS(), false);
}
