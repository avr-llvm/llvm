//===-- AVRAsmBackend.cpp - AVR Asm Backend  ------------------------------===//
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

#include "MCTargetDesc/AVRAsmBackend.h"

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

#include "MCTargetDesc/AVRFixupKinds.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"

namespace llvm {

inline unsigned adjustFixupRelCondbr(unsigned size,
                                     const MCFixup &Fixup,
                                     uint64_t Value,
                                     MCContext *Ctx = nullptr)
{
  // We now check if Value can fit in the specified size.
  // The value is rightshifted by one, giving us one extra bit of precision.
  if (!isIntN(size+1, Value) && Ctx != nullptr)
    Ctx->reportFatalError(Fixup.getLoc(), "out of range conditional branch target");
    
  Value = AVR::fixups::adjustRelativeBranchTarget(Value);
  
  return Value;
}

// TODO: On some targets, the program counter is 16-bits with 128KB progmem maximum.
//       On other targets, the counter is 22-bits with 8MB progmem maximum.
//       It might be a good idea to check whether the value fits into the size depending
//       on what the current target is, instead of the maximum - 22 bits.
inline unsigned adjustFixupCall(const MCFixup &Fixup,
                                uint64_t Value,
                                MCContext *Ctx = nullptr)
{
  // TODO: set this depending on device
  const auto CALL_TARGET_SIZE = 22;

  // We have one extra bit of precision because the value is rightshifted by one.
  if(!isIntN(CALL_TARGET_SIZE+1, Value) && Ctx != nullptr)
    Ctx->reportFatalError(Fixup.getLoc(), "out of range call target");

  Value = AVR::fixups::adjustBranchTarget(Value);
  return Value;
}

// Prepare value for the target space for it
inline unsigned adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext *Ctx = nullptr) {
  switch ((unsigned)Fixup.getKind()) {
    case AVR::fixup_7_pcrel:  return adjustFixupRelCondbr(7, Fixup, Value, Ctx);
    case AVR::fixup_13_pcrel: return adjustFixupRelCondbr(13, Fixup, Value, Ctx);
    case AVR::fixup_call:     return adjustFixupCall(Fixup, Value, Ctx);

    case AVR::fixup_lo8_ldi:
    case AVR::fixup_hi8_ldi:
    case AVR::fixup_hh8_ldi:
    case AVR::fixup_ms8_ldi:
    case AVR::fixup_lo8_ldi_pm:
    case AVR::fixup_hi8_ldi_pm:
    case AVR::fixup_hh8_ldi_pm:

    case FK_Data_2:
    case FK_GPRel_4:
    case FK_Data_4:
    case FK_Data_8:
      return Value;
    default: llvm_unreachable("unhandled fixup");
  }
}


MCObjectWriter *AVRAsmBackend::createObjectWriter(raw_pwrite_stream &OS) const {
  return createAVRELFObjectWriter(OS, MCELFObjectTargetWriter::getOSABI(OSType));
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
    // This table *must* be in same the order of fixup_* kinds in
    // AVRFixupKinds.h.
    //
    // name                    offset  bits  flags
    { "fixup_32",              0,      32,   0 },

    { "fixup_7_pcrel",         0,      7,    MCFixupKindInfo::FKF_IsPCRel },
    { "fixup_13_pcrel",        0,      13,   MCFixupKindInfo::FKF_IsPCRel },

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

    { "fixup_call",            0,      22,   0 },
    { "fixup_ldi",             0,      8,    0 },

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

    { "fixup_sym_diff",        0,      32,   0 },
    { "fixup_16_ldst",         0,      16,   0 },

    { "fixup_lds_sts_16",      0,      16,   0 },

    { "fixup_port6",           0,      6,    0 },
    { "fixup_port5",           0,      5,    0 },

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
  // If the count is not 2-byte aligned, we must be writing data into the text
  // section (otherwise we have unaligned instructions, and thus have far
  // bigger problems), so just write zeros instead.
  assert((Count % 2) == 0 && "NOP instructions must be 2 bytes");

  OW->WriteZeros(Count);
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
                                      bool &IsResolved)
{
  // At this point we'll ignore the value returned by adjustFixupValue as
  // we are only checking if the fixup can be applied correctly. We have
  // access to MCContext from here which allows us to report a fatal error
  // with *possibly* a source code location.
  adjustFixupValue(Fixup, Value, &Asm.getContext());
}

MCAsmBackend *
createAVRAsmBackend(const Target &T, const MCRegisterInfo &MRI,
                    const Triple &TT, StringRef CPU)
{
  return new AVRAsmBackend(T, TT.getOS());
}

} // end of namespace llvm

