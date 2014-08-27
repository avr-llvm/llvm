//===-- AVRFrameLowering.h - Define frame lowering for AVR ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRFRAMELOWERING_H__
#define __INCLUDE_AVRFRAMELOWERING_H__

#include "llvm/Target/TargetFrameLowering.h"

namespace llvm
{

class AVRFrameLowering : public TargetFrameLowering
{
public:
  explicit AVRFrameLowering();
public: // TargetFrameLowering
  void emitPrologue(MachineFunction &MF) const;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const;
  bool hasFP(const MachineFunction &MF) const;
  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const;
  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const std::vector<CalleeSavedInfo> &CSI,
                                   const TargetRegisterInfo *TRI) const;
  bool hasReservedCallFrame(const MachineFunction &MF) const;
  bool canSimplifyCallFramePseudos(const MachineFunction &MF) const;
  void processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                            RegScavenger *RS = NULL) const;
  void eliminateCallFramePseudoInstr(MachineFunction &MF,
                                     MachineBasicBlock &MBB,
                                     MachineBasicBlock::iterator MI) const;
};

} // end namespace llvm

#endif //__INCLUDE_AVRFRAMELOWERING_H__
