//===-- AMDGPUInstrInfo.h - AMDGPU Instruction Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Contains the definition of a TargetInstrInfo class that is common
/// to all AMD GPUs.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AMDGPU_AMDGPUINSTRINFO_H
#define LLVM_LIB_TARGET_AMDGPU_AMDGPUINSTRINFO_H

#include "AMDGPURegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"

#define GET_INSTRINFO_HEADER
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_OPERAND_ENUM
#include "AMDGPUGenInstrInfo.inc"

#define OPCODE_IS_ZERO_INT AMDGPU::PRED_SETE_INT
#define OPCODE_IS_NOT_ZERO_INT AMDGPU::PRED_SETNE_INT
#define OPCODE_IS_ZERO AMDGPU::PRED_SETE
#define OPCODE_IS_NOT_ZERO AMDGPU::PRED_SETNE

namespace llvm {

class AMDGPUSubtarget;
class MachineFunction;
class MachineInstr;
class MachineInstrBuilder;

class AMDGPUInstrInfo : public AMDGPUGenInstrInfo {
private:
  const AMDGPURegisterInfo RI;
  virtual void anchor();
protected:
  const AMDGPUSubtarget &ST;
public:
  explicit AMDGPUInstrInfo(const AMDGPUSubtarget &st);

  virtual const AMDGPURegisterInfo &getRegisterInfo() const = 0;

public:
  /// \returns the smallest register index that will be accessed by an indirect
  /// read or write or -1 if indirect addressing is not used by this program.
  int getIndirectIndexBegin(const MachineFunction &MF) const;

  /// \returns the largest register index that will be accessed by an indirect
  /// read or write or -1 if indirect addressing is not used by this program.
  int getIndirectIndexEnd(const MachineFunction &MF) const;

  bool enableClusterLoads() const override;

  bool shouldScheduleLoadsNear(SDNode *Load1, SDNode *Load2,
                               int64_t Offset1, int64_t Offset2,
                               unsigned NumLoads) const override;

  /// \brief Return a target-specific opcode if Opcode is a pseudo instruction.
  /// Return -1 if the target-specific opcode for the pseudo instruction does
  /// not exist. If Opcode is not a pseudo instruction, this is identity.
  int pseudoToMCOpcode(int Opcode) const;

//===---------------------------------------------------------------------===//
// Pure virtual funtions to be implemented by sub-classes.
//===---------------------------------------------------------------------===//

  /// \returns The register class to be used for loading and storing values
  /// from an "Indirect Address" .
  virtual const TargetRegisterClass *getIndirectAddrRegClass() const {
    llvm_unreachable("getIndirectAddrRegClass() not implemented");
  }

  /// \brief Given a MIMG \p Opcode that writes all 4 channels, return the
  /// equivalent opcode that writes \p Channels Channels.
  int getMaskedMIMGOp(uint16_t Opcode, unsigned Channels) const;

};

namespace AMDGPU {
  LLVM_READONLY
  int16_t getNamedOperandIdx(uint16_t Opcode, uint16_t NamedIndex);
}  // End namespace AMDGPU

} // End llvm namespace

#define AMDGPU_FLAG_REGISTER_LOAD  (UINT64_C(1) << 63)
#define AMDGPU_FLAG_REGISTER_STORE (UINT64_C(1) << 62)

#endif
