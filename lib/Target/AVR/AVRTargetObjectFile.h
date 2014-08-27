//===-- AVRTargetObjectFile.h - AVR Object Info -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRTARGETOBJECTFILE_H__
#define __INCLUDE_AVRTARGETOBJECTFILE_H__

#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"

namespace llvm
{

class AVRTargetObjectFile : public TargetLoweringObjectFileELF
{
public:
  void Initialize(MCContext &ctx, const TargetMachine &TM);

  const MCSection *SelectSectionForGlobal(const GlobalValue *GV,
                                          SectionKind Kind,
                                          Mangler &Mang,
                                          const TargetMachine &TM) const;
private:
  const MCSection *ProgmemDataSection;
};

} // end namespace llvm

#endif //__INCLUDE_AVRTARGETOBJECTFILE_H__
