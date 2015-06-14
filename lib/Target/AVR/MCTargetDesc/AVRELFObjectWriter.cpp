//===-- AVRELFObjectWriter.cpp - AVR ELF Writer ---------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AVRConfig.h"

#include <list>

#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

#include "MCTargetDesc/AVRFixupKinds.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"

namespace llvm {

class AVRELFObjectWriter : public MCELFObjectTargetWriter {
public:
  AVRELFObjectWriter(uint8_t OSABI);

  virtual ~AVRELFObjectWriter();

  unsigned GetRelocType(const MCValue &Target, const MCFixup &Fixup,
                        bool IsPCRel) const override;
  
  bool needsRelocateWithSymbol(const MCSymbol &Sym,
                               unsigned Type) const override;
};


AVRELFObjectWriter::AVRELFObjectWriter(uint8_t OSABI)
  : MCELFObjectTargetWriter(false, OSABI, ELF::EM_AVR,
                            true,
                            false) {}

AVRELFObjectWriter::~AVRELFObjectWriter() {}

unsigned AVRELFObjectWriter::GetRelocType(const MCValue &Target,
                                           const MCFixup &Fixup,
                                           bool IsPCRel) const {
  switch ((unsigned)Fixup.getKind()) {
    case FK_Data_1:                 return ELF::R_AVR_8;
    case FK_Data_2:                 return ELF::R_AVR_16;
    case FK_Data_4:                 return ELF::R_AVR_32;
    case AVR::fixup_32:             return ELF::R_AVR_32;
    case AVR::fixup_7_pcrel:        return ELF::R_AVR_7_PCREL;
    case AVR::fixup_13_pcrel:       return ELF::R_AVR_13_PCREL;
    case AVR::fixup_16:             return ELF::R_AVR_16;
    case AVR::fixup_16_pm:          return ELF::R_AVR_16_PM;
    case AVR::fixup_lo8_ldi:        return ELF::R_AVR_LO8_LDI;
    case AVR::fixup_hi8_ldi:        return ELF::R_AVR_HI8_LDI;
    case AVR::fixup_hh8_ldi:        return ELF::R_AVR_HH8_LDI;
    case AVR::fixup_lo8_ldi_neg:    return ELF::R_AVR_LO8_LDI_NEG;
    case AVR::fixup_hi8_ldi_neg:    return ELF::R_AVR_HI8_LDI_NEG;
    case AVR::fixup_hh8_ldi_neg:    return ELF::R_AVR_HH8_LDI_NEG;
    case AVR::fixup_lo8_ldi_pm:     return ELF::R_AVR_LO8_LDI_PM;
    case AVR::fixup_hi8_ldi_pm:     return ELF::R_AVR_HI8_LDI_PM;
    case AVR::fixup_hh8_ldi_pm:     return ELF::R_AVR_HH8_LDI_PM;
    case AVR::fixup_lo8_ldi_pm_neg: return ELF::R_AVR_LO8_LDI_PM_NEG;
    case AVR::fixup_hi8_ldi_pm_neg: return ELF::R_AVR_HI8_LDI_PM_NEG;
    case AVR::fixup_hh8_ldi_pm_neg: return ELF::R_AVR_HH8_LDI_PM_NEG;
    case AVR::fixup_call:           return ELF::R_AVR_CALL;
    case AVR::fixup_ldi:            return ELF::R_AVR_LDI;
    case AVR::fixup_6:              return ELF::R_AVR_6;
    case AVR::fixup_6_adiw:         return ELF::R_AVR_6_ADIW;
    case AVR::fixup_ms8_ldi:        return ELF::R_AVR_MS8_LDI;
    case AVR::fixup_ms8_ldi_neg:    return ELF::R_AVR_MS8_LDI_NEG;
    case AVR::fixup_lo8_ldi_gs:     return ELF::R_AVR_LO8_LDI_GS;
    case AVR::fixup_hi8_ldi_gs:     return ELF::R_AVR_HI8_LDI_GS;
    case AVR::fixup_8:              return ELF::R_AVR_8;
    case AVR::fixup_8_lo8:          return ELF::R_AVR_8_LO8;
    case AVR::fixup_8_hi8:          return ELF::R_AVR_8_HI8;
    case AVR::fixup_8_hlo8:         return ELF::R_AVR_8_HLO8;
    case AVR::fixup_sym_diff:       return ELF::R_AVR_SYM_DIFF;
    case AVR::fixup_16_ldst:        return ELF::R_AVR_16_LDST;
    case AVR::fixup_lds_sts_16:     return ELF::R_AVR_LDS_STS_16;
    case AVR::fixup_port6:          return ELF::R_AVR_PORT6;
    case AVR::fixup_port5:          return ELF::R_AVR_PORT5;
    default: llvm_unreachable("invalid fixup kind!"); return ELF::R_AVR_NONE;
  }
}

bool
AVRELFObjectWriter::needsRelocateWithSymbol(const MCSymbol &Sym,
                                            unsigned Type) const {
  return false;
}

MCObjectWriter *
createAVRELFObjectWriter(raw_pwrite_stream &OS, uint8_t OSABI) {
  MCELFObjectTargetWriter *MOTW = new AVRELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, true);
}

} // end of namespace llvm

