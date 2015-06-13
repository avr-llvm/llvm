//===-- AVRISelLowering.h - AVR DAG Lowering Interface ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that AVR uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_AVR_ISEL_LOWERING_H
# define LLVM_AVR_ISEL_LOWERING_H

# include "llvm/Target/TargetLowering.h"

namespace llvm {

namespace AVRISD {

//! AVR Specific DAG Nodes
enum NodeType {
  //! Start the numbering where the builtin ops leave off.
  FIRST_NUMBER = ISD::BUILTIN_OP_END,
  //! Return from subroutine.
  RET_FLAG,
  //! Return from ISR.
  RETI_FLAG,
  //! These operations represent an abstract call
  //! instruction, which includes a bunch of information.
  CALL,
  //! A wrapper node for TargetConstantPool,
  //! TargetExternalSymbol, and TargetGlobalAddress.
  Wrapper,
  //! Bit shifting and rotation.
  LSL,
  LSR,
  ASR,
  ROR,
  ROL,
  //! Non-constant shifts.
  LSLLOOP,
  LSRLOOP,
  ASRLOOP,
  //! AVR conditional branches. Operand 0 is the chain operand, operand 1
  //! is the block to branch if condition is true, operand 2 is the
  //! condition code, and operand 3 is the flag operand produced by a CMP
  //! or TEST instruction.
  BRCOND,
  //! Compare instruction.
  CMP,
  //! Compare with carry instruction.
  CMPC,
  //! Test for zero or minus instruction.
  TST,
  //! Operand 0 and operand 1 are selection variable, operand 2
  //! is condition code and operand 3 is flag operand.
  SELECT_CC
};

} // end of namespace AVRISD

class AVRTargetMachine;

class AVRTargetLowering : public TargetLowering
{
public:
  explicit AVRTargetLowering(AVRTargetMachine &TM);
public:
  MVT getScalarShiftAmountTy(EVT LHSTy) const { return MVT::i8; }
  const char *getTargetNodeName(unsigned Opcode) const override;

  /*!
   *  Provide custom lowering hooks for some operations.
   */
  SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

  /*!
   * Replace the results of node with an illegal result
   * type with new values built out of custom code.
   */
  void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue> &Results,
                          SelectionDAG &DAG) const override;

  /*!
   * Returns true if the addressing mode represented
   * by AM is legal for this target, for a load/store
   * of the specified type.
   */
  bool isLegalAddressingMode(const AddrMode &AM, Type *Ty,
                             unsigned AS) const override;

  /*!
   * Returns true by value, base pointer and
   * offset pointer and addressing mode by reference if the node's address
   * can be legally represented as pre-indexed load / store address.
   */
  bool getPreIndexedAddressParts(SDNode *N, SDValue &Base, SDValue &Offset,
                                 ISD::MemIndexedMode &AM,
                                 SelectionDAG &DAG) const override;

  /*!
   * Returns true by value, base pointer and
   * offset pointer and addressing mode by reference if this node can be
   * combined with a load / store to form a post-indexed load / store.
   */
  bool getPostIndexedAddressParts(SDNode *N, SDNode *Op, SDValue &Base,
                                  SDValue &Offset, ISD::MemIndexedMode &AM,
                                  SelectionDAG &DAG) const override;

  /*!
   * Return true if folding a constant offset
   * with the given GlobalAddress is legal.  It is frequently not legal in
   * PIC relocation models.
   */
  bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const override;

  MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr *MI,
                                                 MachineBasicBlock *MBB) const override;

  /// Inline Asm support.
  /// Implementation of TargetLowering hooks.
  ConstraintType getConstraintType(const std::string &Constraint) const override;

  ConstraintWeight
  getSingleConstraintMatchWeight(AsmOperandInfo &info,
                                 const char *constraint) const override;

  std::pair<unsigned, const TargetRegisterClass *>
  getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                               const std::string &Constraint,
                               MVT VT) const override;

  void LowerAsmOperandForConstraint(SDValue Op, std::string &Constraint,
                                    std::vector<SDValue> &Ops,
                                    SelectionDAG &DAG) const override;

private:
  SDValue getAVRCmp(SDValue LHS, SDValue RHS, ISD::CondCode CC, SDValue &AVRcc,
                    SelectionDAG &DAG, SDLoc dl) const;
  SDValue LowerShifts(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBR_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerINLINEASM(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSETCC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerVASTART(SDValue Op, SelectionDAG &DAG) const;

  SDValue LowerReturn(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                      const SmallVectorImpl<ISD::OutputArg> &Outs,
                      const SmallVectorImpl<SDValue> &OutVals, SDLoc dl,
                      SelectionDAG &DAG) const;
  SDValue LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::InputArg> &Ins,
                               SDLoc dl, SelectionDAG &DAG,
                               SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerCall(TargetLowering::CallLoweringInfo &CLI,
                    SmallVectorImpl<SDValue> &InVals) const;
  SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                          CallingConv::ID CallConv, bool isVarArg,
                          const SmallVectorImpl<ISD::InputArg> &Ins,
                          SDLoc dl, SelectionDAG &DAG,
                          SmallVectorImpl<SDValue> &InVals) const;
  MachineBasicBlock *EmitShiftInstr(MachineInstr *MI,
                                    MachineBasicBlock *BB) const;
};

} // end namespace llvm

#endif // LLVM_AVR_ISEL_LOWERING_H
