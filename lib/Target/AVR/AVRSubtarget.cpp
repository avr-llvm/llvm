//===-- AVRSubtarget.cpp - AVR Subtarget Information ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AVR specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/TargetRegistry.h"

#include "AVR.h"
#include "AVRSubtarget.h"
#include "AVRTargetMachine.h"

#define DEBUG_TYPE "avr-subtarget"

#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AVRGenSubtargetInfo.inc"

using namespace llvm;


AVRSubtarget::AVRSubtarget(const std::string &TT, const std::string &CPU,
                           const std::string &FS, AVRTargetMachine &TM) :
  AVRGenSubtargetInfo(TT, CPU, FS),
  InstrInfo(),
  FrameLowering(),
  TLInfo(TM),
  TSInfo(*TM.getDataLayout()),
  
  // Subtarget features
  m_hasSRAM(),
  m_hasJMPCALL(),
  m_hasIJMPCALL(),
  m_hasEIJMPCALL(),
  m_hasADDSUBIW(),
  m_hasSmallStack(),
  m_hasMOVW(),
  m_hasLPM(),
  m_hasLPMX(),
  m_hasELPM(),
  m_hasELPMX(),
  m_hasSPM(),
  m_hasSPMX(),
  m_hasDES(),
  m_supportsRMW(),
  m_supportsMultiplication(),
  m_hasBREAK(),
  m_hasTinyEncoding(),
  m_FeatureSetDummy()
{
  // Parse features string.
  ParseSubtargetFeatures(CPU, FS);
}

