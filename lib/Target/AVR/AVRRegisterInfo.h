//===-- AVRRegisterInfo.h - AVR Register Information Impl -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the AVR implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRREGISTERINFO_H__
#define __INCLUDE_AVRREGISTERINFO_H__

#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "AVRGenRegisterInfo.inc"

namespace llvm
{

class AVRRegisterInfo : public AVRGenRegisterInfo
{
public:
  AVRRegisterInfo();
public: // TargetRegisterInfo
  /// Code Generation virtual methods...
  const uint16_t *getCalleeSavedRegs(const MachineFunction *MF = 0) const;
  const uint32_t *getCallPreservedMask(CallingConv::ID CC) const;
  BitVector getReservedRegs(const MachineFunction &MF) const;

  const TargetRegisterClass *
  getLargestLegalSuperClass(const TargetRegisterClass *RC) const;

  /// Stack Frame Processing Methods
  void eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = NULL) const;

  /// Debug information queries.
  unsigned getFrameRegister(const MachineFunction &MF) const;

  /// Returns a TargetRegisterClass used for pointer values.
  const TargetRegisterClass *
  getPointerRegClass(const MachineFunction &MF, unsigned Kind = 0) const;
};

} // end namespace llvm

#endif //__INCLUDE_AVRREGISTERINFO_H__
