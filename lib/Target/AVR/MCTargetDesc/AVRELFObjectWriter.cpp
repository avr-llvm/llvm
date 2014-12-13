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
    case AVR::fixup_7_pcrel:
      Type = ELF::R_AVR_7_PCREL;
      break;
    case AVR::fixup_12_pcrel:
      Type = ELF::R_AVR_13_PCREL;
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
