//===-- AVRInstrInfo.h - AVR Instruction Information ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the AVR implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRINSTRINFO_H__
#define __INCLUDE_AVRINSTRINFO_H__

#include "AVRRegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#include "AVRGenInstrInfo.inc"

namespace llvm
{

namespace AVRCC
{
  // AVR specific condition code. These correspond to AVR_*_COND in
  // AVRInstrInfo.td. They must be kept in synch.
  enum CondCodes
  {
    COND_EQ,            // equal
    COND_NE,            // not equal
    COND_GE,            // greater than or equal
    COND_LT,            // less than
    COND_SH,            // unsigned same or higher
    COND_LO,            // unsigned lower
    COND_MI,            // minus
    COND_PL,            // plus
    COND_INVALID
  };
}

namespace AVRII
{
  /// Target Operand Flag enum.
  enum TOF
  {
    //===------------------------------------------------------------------===//
    // AVR Specific MachineOperand flags.
    MO_NO_FLAG,

    /// MO_LO - On a symbol operand, this represents the lo part.
    MO_LO = (1 << 1),

    /// MO_HI - On a symbol operand, this represents the hi part.
    MO_HI = (1 << 2),

    /// MO_NEG - On a symbol operand, this represents it has to be negated.
    MO_NEG = (1 << 3)
  };
}

class AVRInstrInfo : public AVRGenInstrInfo
{
public:
  explicit AVRInstrInfo();
public:
  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const AVRRegisterInfo &getRegisterInfo() const { return RI; }
  const MCInstrDesc &getBrCond(AVRCC::CondCodes CC) const;
  AVRCC::CondCodes getCondFromBranchOpc(unsigned Opc) const;
  AVRCC::CondCodes getOppositeCondition(AVRCC::CondCodes CC) const;
  unsigned GetInstSizeInBytes(const MachineInstr *MI) const;
public: // TargetInstrInfo
  void copyPhysReg(MachineBasicBlock &MBB, MachineBasicBlock::iterator MI,
                   DebugLoc DL, unsigned DestReg, unsigned SrcReg,
                   bool KillSrc) const;
  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, unsigned SrcReg,
                           bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const;
  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI, unsigned DestReg,
                            int FrameIndex, const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const;
  unsigned isLoadFromStackSlot(const MachineInstr *MI, int &FrameIndex) const;
  unsigned isStoreToStackSlot(const MachineInstr *MI, int &FrameIndex) const;

  // Branch analysis.
  bool AnalyzeBranch(MachineBasicBlock &MBB, MachineBasicBlock *&TBB,
                     MachineBasicBlock *&FBB,
                     SmallVectorImpl<MachineOperand> &Cond,
                     bool AllowModify = false) const;
  unsigned InsertBranch(MachineBasicBlock &MBB, MachineBasicBlock *TBB,
                        MachineBasicBlock *FBB,
                        const SmallVectorImpl<MachineOperand> &Cond,
                        DebugLoc DL) const;
  unsigned RemoveBranch(MachineBasicBlock &MBB) const;
  bool ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const;
private:
  const AVRRegisterInfo RI;
};

} // end namespace llvm

#endif //__INCLUDE_AVRINSTRINFO_H__
