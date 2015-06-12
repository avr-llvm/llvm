//===-- AVRTargetObjectFile.cpp - AVR Object Files ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AVRTargetObjectFile.h"

#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/ELF.h"

using namespace llvm;

void AVRTargetObjectFile::Initialize(MCContext &ctx, const TargetMachine &TM)
{
  TargetLoweringObjectFileELF::Initialize(ctx, TM);

  ProgmemDataSection =
    getContext().getELFSection(".progmem.data", ELF::SHT_PROGBITS,
                               ELF::SHF_ALLOC);
}

MCSection *
AVRTargetObjectFile::SelectSectionForGlobal(const GlobalValue *GV,
                                            SectionKind Kind, Mangler &Mang,
                                            const TargetMachine &TM) const
{
  // Global values in flash memory are placed in the progmem.data section.
  if (GV->getType()->getAddressSpace() == 1)
  {
    // Do not override custom user sections.
    if (!GV->hasSection())
    {
      return ProgmemDataSection;
    }
  }

  // Otherwise, we work the same way as ELF.
  return TargetLoweringObjectFileELF::SelectSectionForGlobal(GV, Kind, Mang,
                                                             TM);
}
