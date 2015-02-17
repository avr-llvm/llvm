//===-- AVRELFObjectWriter.cpp - AVR ELF Writer -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/AVRFixupKinds.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include <list>

using namespace llvm;

namespace {
  class AVRELFObjectWriter : public MCELFObjectTargetWriter {
  public:
    AVRELFObjectWriter(uint8_t OSABI);

    virtual ~AVRELFObjectWriter();

    unsigned GetRelocType(const MCValue &Target, const MCFixup &Fixup,
                          bool IsPCRel) const override;
    
    bool needsRelocateWithSymbol(const MCSymbolData &SD,
                                 unsigned Type) const override;
  };
}

AVRELFObjectWriter::AVRELFObjectWriter(uint8_t OSABI)
  : MCELFObjectTargetWriter(false, OSABI, ELF::EM_AVR,
                            /*HasRelocationAddend*/ false,
                            false) {}

AVRELFObjectWriter::~AVRELFObjectWriter() {}

unsigned AVRELFObjectWriter::GetRelocType(const MCValue &Target,
                                           const MCFixup &Fixup,
                                           bool IsPCRel) const {
  // determine the type of the relocation
  unsigned Type = (unsigned)ELF::R_AVR_NONE;
  unsigned Kind = (unsigned)Fixup.getKind();

  switch (Kind) {
    default:
      llvm_unreachable("invalid fixup kind!");
      break;
    case FK_Data_1:
      Type = ELF::R_AVR_8;
      break;
    case FK_Data_2:
      Type = ELF::R_AVR_16;
      break;
    case FK_Data_4:
      Type = ELF::R_AVR_32;
      break;
    case AVR::fixup_32:
      Type = ELF::R_AVR_32;
      break;
    case AVR::fixup_7_pcrel:
      Type = ELF::R_AVR_7_PCREL;
      break;
    case AVR::fixup_13_pcrel:
      Type = ELF::R_AVR_13_PCREL;
      break;
    case AVR::fixup_16:
      Type = ELF::R_AVR_16;
      break;
    case AVR::fixup_16_pm:
      Type = ELF::R_AVR_16_PM;
      break;
    case AVR::fixup_lo8_ldi:
      Type = ELF::R_AVR_LO8_LDI;
      break;
    case AVR::fixup_hi8_ldi:
      Type = ELF::R_AVR_HI8_LDI;
      break;
    case AVR::fixup_hh8_ldi:
      Type = ELF::R_AVR_HH8_LDI;
      break;
    case AVR::fixup_lo8_ldi_neg:
      Type = ELF::R_AVR_LO8_LDI_NEG;
      break;
    case AVR::fixup_hi8_ldi_neg:
      Type = ELF::R_AVR_HI8_LDI_NEG;
      break;
    case AVR::fixup_hh8_ldi_neg:
      Type = ELF::R_AVR_HH8_LDI_NEG;
      break;
    case AVR::fixup_lo8_ldi_pm:
      Type = ELF::R_AVR_LO8_LDI_PM;
      break;
    case AVR::fixup_hi8_ldi_pm:
      Type = ELF::R_AVR_HI8_LDI_PM;
      break;
    case AVR::fixup_hh8_ldi_pm:
      Type = ELF::R_AVR_HH8_LDI_PM;
      break;
    case AVR::fixup_lo8_ldi_pm_neg:
      Type = ELF::R_AVR_LO8_LDI_PM_NEG;
      break;
    case AVR::fixup_hi8_ldi_pm_neg:
      Type = ELF::R_AVR_HI8_LDI_PM_NEG;
      break;
    case AVR::fixup_hh8_ldi_pm_neg:
      Type = ELF::R_AVR_HH8_LDI_PM_NEG;
      break;
    case AVR::fixup_call:
      Type = ELF::R_AVR_CALL;
      break;
    case AVR::fixup_ldi:
      Type = ELF::R_AVR_LDI;
      break;
    case AVR::fixup_6:
      Type = ELF::R_AVR_6;
      break;
    case AVR::fixup_6_adiw:
      Type = ELF::R_AVR_6_ADIW;
      break;
    case AVR::fixup_ms8_ldi:
      Type = ELF::R_AVR_MS8_LDI;
      break;
    case AVR::fixup_ms8_ldi_neg:
      Type = ELF::R_AVR_MS8_LDI_NEG;
      break;
    case AVR::fixup_lo8_ldi_gs:
      Type = ELF::R_AVR_LO8_LDI_GS;
      break;
    case AVR::fixup_hi8_ldi_gs:
      Type = ELF::R_AVR_HI8_LDI_GS;
      break;
    case AVR::fixup_8:
      Type = ELF::R_AVR_8;
      break;
    case AVR::fixup_8_lo8:
      Type = ELF::R_AVR_8_LO8;
      break;
    case AVR::fixup_8_hi8:
      Type = ELF::R_AVR_8_HI8;
      break;
    case AVR::fixup_8_hlo8:
      Type = ELF::R_AVR_8_HLO8;
      break;
    case AVR::fixup_sym_diff:
      Type = ELF::R_AVR_SYM_DIFF;
      break;
    case AVR::fixup_16_ldst:
      Type = ELF::R_AVR_16_LDST;
      break;
    case AVR::fixup_lds_sts_16:
      Type = ELF::R_AVR_LDS_STS_16;
      break;
    case AVR::fixup_port6:
      Type = ELF::R_AVR_PORT6;
      break;
    case AVR::fixup_port5:
      Type = ELF::R_AVR_PORT5;
      break;
  }
  return Type;
}

bool
AVRELFObjectWriter::needsRelocateWithSymbol(const MCSymbolData &SD,
                                            unsigned Type) const {
  switch (Type) {
  default:
    return true;
  }
}

MCObjectWriter *llvm::createAVRELFObjectWriter(raw_ostream &OS,
                                                uint8_t OSABI,
                                                bool IsLittleEndian) {
  MCELFObjectTargetWriter *MOTW = new AVRELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, IsLittleEndian);
}
