//===-- AVRISelLowering.cpp - AVR DAG Lowering Implementation -------------===//
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

#include "AVRISelLowering.h"

#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/ErrorHandling.h"

#include "AVR.h"
#include "AVRMachineFunctionInfo.h"
#include "AVRTargetMachine.h"

namespace llvm {


AVRTargetLowering::AVRTargetLowering(AVRTargetMachine &tm) :
  TargetLowering(tm)
{
  // Set up the register classes.
  addRegisterClass(MVT::i8, &AVR::GPR8RegClass);
  addRegisterClass(MVT::i16, &AVR::DREGSRegClass);

  // Compute derived properties from the register classes.
  computeRegisterProperties(tm.getSubtargetImpl()->getRegisterInfo());

  setBooleanContents(ZeroOrOneBooleanContent);
  setBooleanVectorContents(ZeroOrOneBooleanContent); // FIXME: Is this correct?
  setSchedulingPreference(Sched::RegPressure);
  setStackPointerRegisterToSaveRestore(AVR::SP);

  setOperationAction(ISD::GlobalAddress, MVT::i16, Custom);
  setOperationAction(ISD::BlockAddress,  MVT::i16, Custom);

  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i8, Expand);
  setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i16, Expand);

  for (MVT VT : MVT::integer_valuetypes()) {
    setLoadExtAction(ISD::EXTLOAD, VT, MVT::i8, Expand);
    setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i8, Expand);
    setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i8, Expand);
  }

  setTruncStoreAction(MVT::i16, MVT::i8, Expand);

  // sub (x, imm) gets canonicalized to add (x, -imm), so for illegal types
  // revert into a sub since we don't have an add with immediate instruction.
  setOperationAction(ISD::ADD, MVT::i32, Custom);
  setOperationAction(ISD::ADD, MVT::i64, Custom);

  // our shift instructions are only able to shift 1 bit at a time, so handle
  // this in a custom way.
  setOperationAction(ISD::SRA, MVT::i8, Custom);
  setOperationAction(ISD::SHL, MVT::i8, Custom);
  setOperationAction(ISD::SRL, MVT::i8, Custom);
  setOperationAction(ISD::SRA, MVT::i16, Custom);
  setOperationAction(ISD::SHL, MVT::i16, Custom);
  setOperationAction(ISD::SRL, MVT::i16, Custom);
  setOperationAction(ISD::SHL_PARTS, MVT::i16, Expand);
  setOperationAction(ISD::SRL_PARTS, MVT::i16, Expand);

  setOperationAction(ISD::BR_CC, MVT::i8, Custom);
  setOperationAction(ISD::BR_CC, MVT::i16, Custom);
  setOperationAction(ISD::BR_CC, MVT::i32, Custom);
  setOperationAction(ISD::BR_CC, MVT::i64, Custom);
  setOperationAction(ISD::BRCOND, MVT::Other, Expand);

  setOperationAction(ISD::SELECT_CC, MVT::i8, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i16, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i64, Custom);
  setOperationAction(ISD::SETCC, MVT::i8, Custom);
  setOperationAction(ISD::SETCC, MVT::i16, Custom);
  setOperationAction(ISD::SETCC, MVT::i32, Custom);
  setOperationAction(ISD::SETCC, MVT::i64, Custom);
  setOperationAction(ISD::SELECT, MVT::i8, Expand);
  setOperationAction(ISD::SELECT, MVT::i16, Expand);

  // add support for postincrement and predecrement load/stores.
  setIndexedLoadAction(ISD::POST_INC, MVT::i8, Legal);
  setIndexedLoadAction(ISD::POST_INC, MVT::i16, Legal);
  setIndexedLoadAction(ISD::PRE_DEC, MVT::i8, Legal);
  setIndexedLoadAction(ISD::PRE_DEC, MVT::i16, Legal);
  setIndexedStoreAction(ISD::POST_INC, MVT::i8, Legal);
  setIndexedStoreAction(ISD::POST_INC, MVT::i16, Legal);
  setIndexedStoreAction(ISD::PRE_DEC, MVT::i8, Legal);
  setIndexedStoreAction(ISD::PRE_DEC, MVT::i16, Legal);

  // :TODO: for now, we don't support jump tables
  setOperationAction(ISD::BR_JT, MVT::Other, Expand);

  setOperationAction(ISD::VASTART, MVT::Other, Custom);
  setOperationAction(ISD::VAEND, MVT::Other, Expand);
  setOperationAction(ISD::VAARG, MVT::Other, Expand);
  setOperationAction(ISD::VACOPY, MVT::Other, Expand);

  setOperationAction(ISD::UDIV, MVT::i8, Expand);
  setOperationAction(ISD::UDIV, MVT::i16, Expand);
  setOperationAction(ISD::UREM, MVT::i8, Expand);
  setOperationAction(ISD::UREM, MVT::i16, Expand);
  setOperationAction(ISD::SDIV, MVT::i8, Expand);
  setOperationAction(ISD::SDIV, MVT::i16, Expand);
  setOperationAction(ISD::SREM, MVT::i8, Expand);
  setOperationAction(ISD::SREM, MVT::i16, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i8, Expand);
  setOperationAction(ISD::UDIVREM, MVT::i16, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i8, Expand);
  setOperationAction(ISD::SDIVREM, MVT::i16, Expand);

  setOperationAction(ISD::SMUL_LOHI, MVT::i8, Expand);
  setOperationAction(ISD::SMUL_LOHI, MVT::i16, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i8, Expand);
  setOperationAction(ISD::UMUL_LOHI, MVT::i16, Expand);
  setOperationAction(ISD::MULHU, MVT::i8, Expand);
  setOperationAction(ISD::MULHU, MVT::i16, Expand);
  setOperationAction(ISD::MULHS, MVT::i8, Expand);
  setOperationAction(ISD::MULHS, MVT::i16, Expand);

  setMinFunctionAlignment(1);
}

const char *AVRTargetLowering::getTargetNodeName(unsigned Opcode) const
{
  switch (Opcode)
  {
  default:                                    return 0;
  case AVRISD::RET_FLAG:                      return "AVRISD::RET_FLAG";
  case AVRISD::RETI_FLAG:                     return "AVRISD::RETI_FLAG";
  case AVRISD::CALL:                          return "AVRISD::CALL";
  case AVRISD::Wrapper:                       return "AVRISD::Wrapper";
  case AVRISD::LSL:                           return "AVRISD::LSL";
  case AVRISD::LSR:                           return "AVRISD::LSR";
  case AVRISD::ROL:                           return "AVRISD::ROL";
  case AVRISD::ROR:                           return "AVRISD::ROR";
  case AVRISD::ASR:                           return "AVRISD::ASR";
  case AVRISD::LSLLOOP:                       return "AVRISD::LSLLOOP";
  case AVRISD::LSRLOOP:                       return "AVRISD::LSRLOOP";
  case AVRISD::ASRLOOP:                       return "AVRISD::ASRLOOP";
  case AVRISD::BRCOND:                        return "AVRISD::BRCOND";
  case AVRISD::CMP:                           return "AVRISD::CMP";
  case AVRISD::CMPC:                          return "AVRISD::CMPC";
  case AVRISD::TST:                           return "AVRISD::TST";
  case AVRISD::SELECT_CC:                     return "AVRISD::SELECT_CC";
  }
}

SDValue AVRTargetLowering::LowerShifts(SDValue Op, SelectionDAG &DAG) const
{
  //:TODO: this function has to be completely rewritten to produce optimal
  // code, for now it's producing very long but correct code.
  unsigned Opc8;
  const SDNode *N = Op.getNode();
  EVT VT = Op.getValueType();
  SDLoc dl(N);

  // Expand non-constant shifts to loops.
  if (!isa<ConstantSDNode>(N->getOperand(1)))
  {
    switch (Op.getOpcode())
    {
    default:
      llvm_unreachable("Invalid shift opcode!");
    case ISD::SHL:
      return DAG.getNode(AVRISD::LSLLOOP, dl, VT, N->getOperand(0),
                         N->getOperand(1));
    case ISD::SRL:
      return DAG.getNode(AVRISD::LSRLOOP, dl, VT, N->getOperand(0),
                         N->getOperand(1));
    case ISD::SRA:
      return DAG.getNode(AVRISD::ASRLOOP, dl, VT, N->getOperand(0),
                         N->getOperand(1));
    }
  }

  uint64_t ShiftAmount = cast<ConstantSDNode>(N->getOperand(1))->getZExtValue();
  SDValue Victim = N->getOperand(0);

  switch (Op.getOpcode())
  {
  case ISD::SRA:
    Opc8 = AVRISD::ASR;
    break;
  case ISD::ROTL:
    Opc8 = AVRISD::ROL;
    break;
  case ISD::ROTR:
    Opc8 = AVRISD::ROR;
    break;
  case ISD::SRL:
    Opc8 = AVRISD::LSR;
    break;
  case ISD::SHL:
    Opc8 = AVRISD::LSL;
    break;
  default:
    llvm_unreachable("Invalid shift opcode");
  }

  while (ShiftAmount--)
  {
    Victim = DAG.getNode(Opc8, dl, VT, Victim);
  }

  return Victim;
}

SDValue
AVRTargetLowering::LowerGlobalAddress(SDValue Op, SelectionDAG &DAG) const
{
  const GlobalValue *GV = cast<GlobalAddressSDNode>(Op)->getGlobal();
  int64_t Offset = cast<GlobalAddressSDNode>(Op)->getOffset();

  // Create the TargetGlobalAddress node, folding in the constant offset.
  SDValue Result = DAG.getTargetGlobalAddress(GV, SDLoc(Op), getPointerTy(),
                                              Offset);
  return DAG.getNode(AVRISD::Wrapper, SDLoc(Op), getPointerTy(), Result);
}

SDValue
AVRTargetLowering::LowerBlockAddress(SDValue Op, SelectionDAG &DAG) const
{
  const BlockAddress *BA = cast<BlockAddressSDNode>(Op)->getBlockAddress();

  SDValue Result = DAG.getTargetBlockAddress(BA, getPointerTy());

  return DAG.getNode(AVRISD::Wrapper, SDLoc(Op), getPointerTy(), Result);
}

/// IntCCToAVRCC - Convert a DAG integer condition code to an AVR CC.
static AVRCC::CondCodes intCCToAVRCC(ISD::CondCode CC)
{
  switch (CC)
  {
  default:                      llvm_unreachable("Unknown condition code!");
  case ISD::SETEQ:              return AVRCC::COND_EQ;
  case ISD::SETNE:              return AVRCC::COND_NE;
  case ISD::SETGE:              return AVRCC::COND_GE;
  case ISD::SETLT:              return AVRCC::COND_LT;
  case ISD::SETUGE:             return AVRCC::COND_SH;
  case ISD::SETULT:             return AVRCC::COND_LO;
  }
}

/// Returns appropriate AVR CMP/CMPC nodes and corresponding condition code for
/// the given operands.
SDValue AVRTargetLowering::getAVRCmp(SDValue LHS, SDValue RHS, ISD::CondCode CC,
                                     SDValue &AVRcc, SelectionDAG &DAG,
                                     SDLoc DL) const
{
  SDValue Cmp;
  EVT VT = LHS.getValueType();
  bool UseTest = false;

  switch (CC)
  {
  default:
    break;
  case ISD::SETLE:
    {
      // Swap operands and reverse the branching condition.
      std::swap(LHS, RHS);
      CC = ISD::SETGE;
      break;
    }
  case ISD::SETGT:
    {
      if (const ConstantSDNode *C = dyn_cast<ConstantSDNode>(RHS))
      {
        switch (C->getSExtValue())
        {
        case -1:
          {
            // When doing lhs > -1 use a tst instruction on the top part of lhs
            // and use brpl instead of using a chain of cp/cpc.
            UseTest = true;
            AVRcc = DAG.getConstant(AVRCC::COND_PL, DL, MVT::i8);
            break;
          }
        case 0:
          {
            // Turn lhs > 0 into 0 < lhs since 0 can be materialized with
            // __zero_reg__ in lhs.
            RHS = LHS;
            LHS = DAG.getConstant(0, DL, VT);
            CC = ISD::SETLT;
            break;
          }
        default:
          {
            // Turn lhs < rhs with lhs constant into rhs >= lhs+1, this allows
            // us to  fold the constant into the cmp instruction.
            RHS = DAG.getConstant(C->getSExtValue() + 1, DL, VT);
            CC = ISD::SETGE;
            break;
          }
        }
        break;
      }
      // Swap operands and reverse the branching condition.
      std::swap(LHS, RHS);
      CC = ISD::SETLT;
      break;
    }
  case ISD::SETLT:
    {
      if (const ConstantSDNode *C = dyn_cast<ConstantSDNode>(RHS))
      {
        switch (C->getSExtValue())
        {
        case 1:
          {
            // Turn lhs < 1 into 0 >= lhs since 0 can be materialized with
            // __zero_reg__ in lhs.
            RHS = LHS;
            LHS = DAG.getConstant(0, DL, VT);
            CC = ISD::SETGE;
            break;
          }
        case 0:
          {
            // When doing lhs < 0 use a tst instruction on the top part of lhs
            // and use brmi instead of using a chain of cp/cpc.
            UseTest = true;
            AVRcc = DAG.getConstant(AVRCC::COND_MI, DL, MVT::i8);
            break;
          }
        }
      }
      break;
    }
  case ISD::SETULE:
    {
      // Swap operands and reverse the branching condition.
      std::swap(LHS, RHS);
      CC = ISD::SETUGE;
      break;
    }
  case ISD::SETUGT:
    {
      // Turn lhs < rhs with lhs constant into rhs >= lhs+1, this allows us to
      // fold the constant into the cmp instruction.
      if (const ConstantSDNode *C = dyn_cast<ConstantSDNode>(RHS))
      {
        RHS = DAG.getConstant(C->getSExtValue() + 1, DL, VT);
        CC = ISD::SETUGE;
        break;
      }
      // Swap operands and reverse the branching condition.
      std::swap(LHS, RHS);
      CC = ISD::SETULT;
      break;
    }
  }

  // Expand 32 and 64 bit comparisons with custom CMP and CMPC nodes instead of
  // using the default and/or/xor expansion code which is much longer.
  if (VT == MVT::i32)
  {
    SDValue LHSlo = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, LHS,
                                DAG.getIntPtrConstant(0, DL));
    SDValue LHShi = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, LHS,
                                DAG.getIntPtrConstant(1, DL));
    SDValue RHSlo = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, RHS,
                                DAG.getIntPtrConstant(0, DL));
    SDValue RHShi = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, RHS,
                                DAG.getIntPtrConstant(1, DL));

    if (UseTest)
    {
      // When using tst we only care about the highest part.
      SDValue Top = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i8, LHShi,
                                DAG.getIntPtrConstant(1, DL));
      Cmp = DAG.getNode(AVRISD::TST, DL, MVT::Glue, Top);
    }
    else
    {
      Cmp = DAG.getNode(AVRISD::CMP, DL, MVT::Glue, LHSlo, RHSlo);
      Cmp = DAG.getNode(AVRISD::CMPC, DL, MVT::Glue, LHShi, RHShi, Cmp);
    }
  }
  else if (VT == MVT::i64)
  {
    SDValue LHS_0 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, LHS,
                                DAG.getIntPtrConstant(0, DL));
    SDValue LHS_1 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, LHS,
                                DAG.getIntPtrConstant(1, DL));

    SDValue LHS0 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, LHS_0,
                               DAG.getIntPtrConstant(0, DL));
    SDValue LHS1 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, LHS_0,
                               DAG.getIntPtrConstant(1, DL));
    SDValue LHS2 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, LHS_1,
                               DAG.getIntPtrConstant(0, DL));
    SDValue LHS3 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, LHS_1,
                               DAG.getIntPtrConstant(1, DL));

    SDValue RHS_0 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, RHS,
                                DAG.getIntPtrConstant(0, DL));
    SDValue RHS_1 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i32, RHS,
                                DAG.getIntPtrConstant(1, DL));

    SDValue RHS0 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, RHS_0,
                               DAG.getIntPtrConstant(0, DL));
    SDValue RHS1 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, RHS_0,
                               DAG.getIntPtrConstant(1, DL));
    SDValue RHS2 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, RHS_1,
                               DAG.getIntPtrConstant(0, DL));
    SDValue RHS3 = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i16, RHS_1,
                               DAG.getIntPtrConstant(1, DL));

    if (UseTest)
    {
      // When using tst we only care about the highest part.
      SDValue Top = DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i8, LHS3,
                                DAG.getIntPtrConstant(1, DL));
      Cmp = DAG.getNode(AVRISD::TST, DL, MVT::Glue, Top);
    }
    else
    {
      Cmp = DAG.getNode(AVRISD::CMP, DL, MVT::Glue, LHS0, RHS0);
      Cmp = DAG.getNode(AVRISD::CMPC, DL, MVT::Glue, LHS1, RHS1, Cmp);
      Cmp = DAG.getNode(AVRISD::CMPC, DL, MVT::Glue, LHS2, RHS2, Cmp);
      Cmp = DAG.getNode(AVRISD::CMPC, DL, MVT::Glue, LHS3, RHS3, Cmp);
    }
  }
  else if (VT == MVT::i8 || VT == MVT::i16)
  {
    if (UseTest)
    {
      // When using tst we only care about the highest part.
      Cmp = DAG.getNode(AVRISD::TST, DL, MVT::Glue,
        (VT == MVT::i8) ? LHS : DAG.getNode(ISD::EXTRACT_ELEMENT, DL, MVT::i8,
                                            LHS, DAG.getIntPtrConstant(1, DL)));
    }
    else
    {
      Cmp = DAG.getNode(AVRISD::CMP, DL, MVT::Glue, LHS, RHS);
    }
  }
  else
  {
    llvm_unreachable("Invalid comparison size");
  }

  // When using a test instruction AVRcc is already set.
  if (!UseTest)
  {
    AVRcc = DAG.getConstant(intCCToAVRCC(CC), DL, MVT::i8);
  }

  return Cmp;
}

SDValue AVRTargetLowering::LowerBR_CC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue Chain = Op.getOperand(0);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
  SDValue LHS = Op.getOperand(2);
  SDValue RHS = Op.getOperand(3);
  SDValue Dest = Op.getOperand(4);
  SDLoc dl(Op);

  SDValue TargetCC;
  SDValue Cmp = getAVRCmp(LHS, RHS, CC, TargetCC, DAG, dl);

  return DAG.getNode(AVRISD::BRCOND, dl, MVT::Other, Chain, Dest, TargetCC,
                     Cmp);
}

SDValue AVRTargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue TrueV = Op.getOperand(2);
  SDValue FalseV = Op.getOperand(3);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
  SDLoc dl(Op);

  SDValue TargetCC;
  SDValue Cmp = getAVRCmp(LHS, RHS, CC, TargetCC, DAG, dl);

  SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
  SDValue Ops[] = { TrueV, FalseV, TargetCC, Cmp };

  return DAG.getNode(AVRISD::SELECT_CC, dl, VTs, Ops);
}

SDValue AVRTargetLowering::LowerSETCC(SDValue Op, SelectionDAG &DAG) const
{
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(2))->get();
  SDLoc DL(Op);

  SDValue TargetCC;
  SDValue Cmp = getAVRCmp(LHS, RHS, CC, TargetCC, DAG, DL);

  SDValue TrueV = DAG.getConstant(1, DL, Op.getValueType());
  SDValue FalseV = DAG.getConstant(0, DL, Op.getValueType());
  SDVTList VTs = DAG.getVTList(Op.getValueType(), MVT::Glue);
  SDValue Ops[] = { TrueV, FalseV, TargetCC, Cmp };

  return DAG.getNode(AVRISD::SELECT_CC, DL, VTs, Ops);
}

SDValue AVRTargetLowering::LowerVASTART(SDValue Op, SelectionDAG &DAG) const
{
  const MachineFunction &MF = DAG.getMachineFunction();
  const AVRMachineFunctionInfo *AFI = MF.getInfo<AVRMachineFunctionInfo>();
  const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
  SDLoc DL(Op);

  // Vastart just stores the address of the VarArgsFrameIndex slot into the
  // memory location argument.
  SDValue FI = DAG.getFrameIndex(AFI->getVarArgsFrameIndex(), getPointerTy());

  return DAG.getStore(Op.getOperand(0), DL, FI, Op.getOperand(1),
                      MachinePointerInfo(SV), false, false, 0);
}

SDValue AVRTargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const
{
  switch (Op.getOpcode())
  {
  default:
    llvm_unreachable("Don't know how to custom lower this!");
  case ISD::SHL:
  case ISD::SRA:
  case ISD::SRL:
  case ISD::ROTL:
  case ISD::ROTR:
    return LowerShifts(Op, DAG);
  case ISD::GlobalAddress:
    return LowerGlobalAddress(Op, DAG);
  case ISD::BlockAddress:
    return LowerBlockAddress(Op, DAG);
  case ISD::BR_CC:
    return LowerBR_CC(Op, DAG);
  case ISD::SELECT_CC:
    return LowerSELECT_CC(Op, DAG);
  case ISD::SETCC:
    return LowerSETCC(Op, DAG);
  case ISD::VASTART:
    return LowerVASTART(Op, DAG);
  }

  return SDValue();
}

/// ReplaceNodeResults - Replace a node with an illegal result type
/// with a new node built out of custom code.
void AVRTargetLowering::ReplaceNodeResults(SDNode *N,
                                           SmallVectorImpl<SDValue> &Results,
                                           SelectionDAG &DAG) const
{
  SDLoc DL(N);

  switch (N->getOpcode())
  {
  default:
    llvm_unreachable("Don't know how to custom expand this!");
  case ISD::ADD:
    {
      // Convert add (x, imm) into sub (x, -imm).
      if (const ConstantSDNode *C = dyn_cast<ConstantSDNode>(N->getOperand(1)))
      {
        SDValue Sub =
          DAG.getNode(ISD::SUB, DL, N->getValueType(0), N->getOperand(0),
                      DAG.getConstant(-C->getAPIntValue(),
                                      DL, C->getValueType(0)));
        Results.push_back(Sub);
        return;
      }
      break;
    }
  }
}

/// isLegalAddressingMode - Return true if the addressing mode represented
/// by AM is legal for this target, for a load/store of the specified type.
bool AVRTargetLowering::isLegalAddressingMode(const AddrMode &AM,
                                              Type *Ty,
                                              unsigned AS) const
{
  // FIXME: should we be using AS over Ty->getPointerAddressSpace()?
  int64_t Offs = AM.BaseOffs;

  // Allow absolute addresses.
  if (AM.BaseGV && !AM.HasBaseReg && AM.Scale == 0 && Offs == 0)
  {
    return true;
  }

  // Flash memory instructions only allow zero offsets.
  if (isa<PointerType>(Ty) && Ty->getPointerAddressSpace() == 1)
  {
    return false;
  }

  // Allow reg+<6bit> offset.
  if (Offs < 0) Offs = -Offs;
  if (AM.BaseGV == 0 && AM.HasBaseReg && AM.Scale == 0 && isUInt<6>(Offs))
  {
    return true;
  }

  return false;
}

/// getPreIndexedAddressParts - returns true by value, base pointer and
/// offset pointer and addressing mode by reference if the node's address
/// can be legally represented as pre-indexed load / store address.
bool AVRTargetLowering::getPreIndexedAddressParts(SDNode *N, SDValue &Base,
                                                  SDValue &Offset,
                                                  ISD::MemIndexedMode &AM,
                                                  SelectionDAG &DAG) const
{
  EVT VT;
  const SDNode *Op;
  SDLoc DL(N);

  if (const LoadSDNode *LD = dyn_cast<LoadSDNode>(N))
  {
    VT = LD->getMemoryVT();
    Op = LD->getBasePtr().getNode();
    if (LD->getExtensionType() != ISD::NON_EXTLOAD) return false;
    if (cast<PointerType>(LD->getMemOperand()->getValue()->getType())->getAddressSpace() == 1)
    {
      return false;
    }
  }
  else if (const StoreSDNode *ST = dyn_cast<StoreSDNode>(N))
  {
    VT = ST->getMemoryVT();
    Op = ST->getBasePtr().getNode();
    if (cast<PointerType>(ST->getMemOperand()->getValue()->getType())->getAddressSpace() == 1)
    {
      return false;
    }
  }
  else
  {
    return false;
  }

  if (VT != MVT::i8 && VT != MVT::i16)
  {
    return false;
  }

  if (Op->getOpcode() != ISD::ADD && Op->getOpcode() != ISD::SUB)
  {
    return false;
  }

  if (const ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(Op->getOperand(1)))
  {
    int RHSC = RHS->getSExtValue();
    if (Op->getOpcode() == ISD::SUB) RHSC = -RHSC;
    if ((VT == MVT::i16 && RHSC != -2) || (VT == MVT::i8 && RHSC != -1))
    {
      return false;
    }

    Base = Op->getOperand(0);
    Offset = DAG.getConstant(RHSC, DL, MVT::i8);
    AM = ISD::PRE_DEC;

    return true;
  }

  return false;
}

/// getPostIndexedAddressParts - returns true by value, base pointer and
/// offset pointer and addressing mode by reference if this node can be
/// combined with a load / store to form a post-indexed load / store.
bool AVRTargetLowering::getPostIndexedAddressParts(SDNode *N, SDNode *Op,
                                                   SDValue &Base,
                                                   SDValue &Offset,
                                                   ISD::MemIndexedMode &AM,
                                                   SelectionDAG &DAG) const
{
  EVT VT;
  SDLoc DL(N);

  if (const LoadSDNode *LD = dyn_cast<LoadSDNode>(N))
  {
    VT = LD->getMemoryVT();
    if (LD->getExtensionType() != ISD::NON_EXTLOAD) return false;
  }
  else if (const StoreSDNode *ST = dyn_cast<StoreSDNode>(N))
  {
    VT = ST->getMemoryVT();
    if (cast<PointerType>(ST->getMemOperand()->getValue()->getType())->getAddressSpace() == 1)
    {
      return false;
    }
  }
  else
  {
    return false;
  }

  if (VT != MVT::i8 && VT != MVT::i16)
  {
    return false;
  }

  if (Op->getOpcode() != ISD::ADD && Op->getOpcode() != ISD::SUB)
  {
    return false;
  }

  if (const ConstantSDNode *RHS = dyn_cast<ConstantSDNode>(Op->getOperand(1)))
  {
    int RHSC = RHS->getSExtValue();
    if (Op->getOpcode() == ISD::SUB) RHSC = -RHSC;
    if ((VT == MVT::i16 && RHSC != 2) || (VT == MVT::i8 && RHSC != 1))
    {
      return false;
    }

    Base = Op->getOperand(0);
    Offset = DAG.getConstant(RHSC, DL, MVT::i8);
    AM = ISD::POST_INC;

    return true;
  }

  return false;
}

/// isOffsetFoldingLegal - Return true if folding a constant offset
/// with the given GlobalAddress is legal.  It is frequently not legal in
/// PIC relocation models.
bool
AVRTargetLowering::isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const
{
  //:TODO: consider folding other operators like or,and,xor,...
  return true;
}

//===----------------------------------------------------------------------===//
//             Formal Arguments Calling Convention Implementation
//===----------------------------------------------------------------------===//

#include "AVRGenCallingConv.inc"

/// For each argument in a function store the number of pieces it is composed
/// of.
static void parseFunctionArgs(const Function *F, const DataLayout *TD,
                              SmallVectorImpl<unsigned> &Out)
{
  for (Function::const_arg_iterator I = F->arg_begin(), E = F->arg_end();
       I != E; ++I)
  {
    unsigned Bytes = TD->getTypeSizeInBits(I->getType()) / 8;
    Out.push_back(((Bytes == 1) || (Bytes == 2)) ? 1 : Bytes / 2);
  }
}

/// For external symbols there is no function prototype information so we
/// have to rely directly on argument sizes.
static void parseExternFuncCallArgs(const SmallVectorImpl<ISD::OutputArg> &In,
                                    SmallVectorImpl<unsigned> &Out)
{
  for (unsigned i = 0, e = In.size(); i != e; )
  {
    unsigned Size = 0;
    unsigned Offset = 0;
    while ((i != e) && (In[i].PartOffset == Offset))
    {
      Offset += In[i].VT.getStoreSize();
      ++i;
      ++Size;
    }
    Out.push_back(Size);
  }
}

/// Analyze incoming and outgoing function arguments. We need custom C++ code
/// to handle special constraints in the ABI like reversing the order of the
/// pieces of splitted arguments. In addition, all pieces of a certain argument
/// have to be passed either using registers or the stack but never mixing both.
static void analyzeArguments(const Function *F, const DataLayout *TD,
                             const SmallVectorImpl<ISD::OutputArg> *Outs,
                             const SmallVectorImpl<ISD::InputArg> *Ins,
                             SmallVectorImpl<CCValAssign> &ArgLocs,
                             CCState &CCInfo, bool IsCall, bool IsVarArg)
{
  static const MCPhysReg RegList8[] =
  {
    AVR::R24, AVR::R22, AVR::R20, AVR::R18, AVR::R16, AVR::R14, AVR::R12,
    AVR::R10, AVR::R8
  };
  static const MCPhysReg RegList16[] =
  {
    AVR::R25R24, AVR::R23R22, AVR::R21R20, AVR::R19R18, AVR::R17R16,
    AVR::R15R14, AVR::R13R12, AVR::R11R10, AVR::R9R8
  };

  if (IsVarArg)
  {
    // Variadic functions do not need all the analisys below.
    if (IsCall)
    {
      CCInfo.AnalyzeCallOperands(*Outs, CC_AVR_Vararg);
    }
    else
    {
      CCInfo.AnalyzeFormalArguments(*Ins, CC_AVR_Vararg);
    }
    return;
  }

  // Fill in the Args array which will contain original argument sizes.
  SmallVector<unsigned, 8> Args;
  if (IsCall && !F)
  {
    parseExternFuncCallArgs(*Outs, Args);
  }
  else
  {
    parseFunctionArgs(F, TD, Args);
  }

  unsigned RegsLeft = array_lengthof(RegList8), ValNo = 0;
  // Variadic functions always use the stack.
  bool UsesStack = false;
  for (unsigned i = 0, pos = 0, e = Args.size(); i != e; ++i)
  {
    unsigned Size = Args[i];
    MVT LocVT = (IsCall) ? (*Outs)[pos].VT : (*Ins)[pos].VT;

    // If we have plenty of regs to pass the whole argument do it.
    if (!UsesStack && (Size <= RegsLeft))
    {
      const MCPhysReg *RegList = (LocVT == MVT::i16) ? RegList16 : RegList8;

      for (unsigned j = 0; j != Size; ++j)
      {
        unsigned Reg = CCInfo.AllocateReg(ArrayRef<MCPhysReg>(RegList, array_lengthof(RegList8)));
        CCInfo.addLoc(CCValAssign::getReg(ValNo++, LocVT, Reg, LocVT,
                                          CCValAssign::Full));
        --RegsLeft;
      }

      // Reverse the order of the pieces to agree with the "big endian" format
      // required in the calling convention ABI.
      std::reverse(ArgLocs.begin() + pos, ArgLocs.begin() + pos + Size);
    }
    else
    {
      // Pass the rest of arguments using the stack.
      UsesStack = true;
      for (unsigned j = 0; j != Size; ++j)
      {
        unsigned Offset = CCInfo.AllocateStack(TD->getTypeAllocSize(EVT(LocVT)
                              .getTypeForEVT(CCInfo.getContext())),
                            TD->getABITypeAlignment(EVT(LocVT)
                              .getTypeForEVT(CCInfo.getContext())));
        CCInfo.addLoc(CCValAssign::getMem(ValNo++, LocVT, Offset, LocVT,
                                          CCValAssign::Full));
      }
    }
    pos += Size;
  }
}

SDValue AVRTargetLowering::
LowerFormalArguments(SDValue Chain, CallingConv::ID CallConv, bool isVarArg,
                     const SmallVectorImpl<ISD::InputArg> &Ins, SDLoc dl,
                     SelectionDAG &DAG,
                     SmallVectorImpl<SDValue> &InVals) const
{
  MachineFunction &MF = DAG.getMachineFunction();
  MachineFrameInfo *MFI = MF.getFrameInfo();

  // Assign locations to all of the incoming arguments.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 ArgLocs, *DAG.getContext());

  analyzeArguments(MF.getFunction(), getDataLayout(), 0, &Ins, ArgLocs, CCInfo, false,
                   isVarArg);

  SDValue ArgValue;
  for (unsigned i = 0, e = ArgLocs.size(); i != e; ++i)
  {
    CCValAssign &VA = ArgLocs[i];

    // Arguments stored on registers.
    if (VA.isRegLoc())
    {
      EVT RegVT = VA.getLocVT();
      const TargetRegisterClass *RC;

      if (RegVT == MVT::i8)
      {
        RC = &AVR::GPR8RegClass;
      }
      else if (RegVT == MVT::i16)
      {
        RC = &AVR::DREGSRegClass;
      }
      else
      {
        llvm_unreachable("Unknown argument type!");
      }

      unsigned Reg = MF.addLiveIn(VA.getLocReg(), RC);
      ArgValue = DAG.getCopyFromReg(Chain, dl, Reg, RegVT);

      // :NOTE: Clang should not promote any i8 into i16 but for safety the
      // following code will handle zexts or sexts generated by other
      // front ends. Otherwise:
      // If this is an 8 bit value, it is really passed promoted
      // to 16 bits. Insert an assert[sz]ext to capture this, then
      // truncate to the right size.
      switch (VA.getLocInfo())
      {
      default:
        llvm_unreachable("Unknown loc info!");
      case CCValAssign::Full:
        break;
      case CCValAssign::BCvt:
        ArgValue = DAG.getNode(ISD::BITCAST, dl, VA.getValVT(), ArgValue);
        break;
      case CCValAssign::SExt:
        ArgValue = DAG.getNode(ISD::AssertSext, dl, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);
        break;
      case CCValAssign::ZExt:
        ArgValue = DAG.getNode(ISD::AssertZext, dl, RegVT, ArgValue,
                               DAG.getValueType(VA.getValVT()));
        ArgValue = DAG.getNode(ISD::TRUNCATE, dl, VA.getValVT(), ArgValue);
        break;
      }

      InVals.push_back(ArgValue);
    }
    else
    {
      // Sanity check.
      assert(VA.isMemLoc());

      EVT LocVT = VA.getLocVT();

      // Create the frame index object for this incoming parameter.
      int FI = MFI->CreateFixedObject(LocVT.getSizeInBits() / 8,
                                      VA.getLocMemOffset(), true);

      // Create the SelectionDAG nodes corresponding to a load
      // from this parameter.
      SDValue FIN = DAG.getFrameIndex(FI, getPointerTy());
      InVals.push_back(DAG.getLoad(LocVT, dl, Chain, FIN,
                                   MachinePointerInfo::getFixedStack(FI), false,
                                   false, false, 0));
    }
  }

  // If the function takes variable number of arguments, make a frame index for
  // the start of the first vararg value... for expansion of llvm.va_start.
  if (isVarArg)
  {
    unsigned StackSize = CCInfo.getNextStackOffset();
    AVRMachineFunctionInfo *AFI = MF.getInfo<AVRMachineFunctionInfo>();

    AFI->setVarArgsFrameIndex(MFI->CreateFixedObject(2, StackSize, true));
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
//                  Call Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue
AVRTargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                             SmallVectorImpl<SDValue> &InVals) const
{
  SelectionDAG &DAG = CLI.DAG;
  SDLoc &DL = CLI.DL;
  SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
  SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
  SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
  SDValue Chain = CLI.Chain;
  SDValue Callee = CLI.Callee;
  bool &isTailCall = CLI.IsTailCall;
  CallingConv::ID CallConv = CLI.CallConv;
  bool isVarArg = CLI.IsVarArg;

  // AVR does not yet support tail call optimization.
  isTailCall = false;

  // Analyze operands of the call, assigning locations to each operand.
  SmallVector<CCValAssign, 16> ArgLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 ArgLocs, *DAG.getContext());

  // If the callee is a GlobalAddress/ExternalSymbol node (quite common, every
  // direct call is) turn it into a TargetGlobalAddress/TargetExternalSymbol
  // node so that legalize doesn't hack it.
  const Function *F = 0;
  if (const GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
  {
    const GlobalValue *GV = G->getGlobal();

    F = cast<Function>(GV);
    Callee = DAG.getTargetGlobalAddress(GV, DL, getPointerTy());
  }
  else if (const ExternalSymbolSDNode *ES =
             dyn_cast<ExternalSymbolSDNode>(Callee))
  {
    Callee = DAG.getTargetExternalSymbol(ES->getSymbol(), getPointerTy());
  }

  analyzeArguments(F, getDataLayout(), &Outs, 0, ArgLocs, CCInfo, true, isVarArg);

  // Get a count of how many bytes are to be pushed on the stack.
  unsigned NumBytes = CCInfo.getNextStackOffset();

  Chain = DAG.getCALLSEQ_START(Chain, DAG.getIntPtrConstant(NumBytes, DL, true),
                               DL);

  SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;

  // First, walk the register assignments, inserting copies.
  unsigned AI, AE;
  bool HasStkArgs = false;
  for (AI = 0, AE = ArgLocs.size(); AI != AE; ++AI)
  {
    CCValAssign &VA = ArgLocs[AI];
    EVT RegVT = VA.getLocVT();
    SDValue Arg = OutVals[AI];

    // Promote the value if needed. With Clang this should not happen.
    switch (VA.getLocInfo())
    {
    default:
      llvm_unreachable("Unknown loc info!");
    case CCValAssign::Full:
      break;
    case CCValAssign::SExt:
      Arg = DAG.getNode(ISD::SIGN_EXTEND, DL, RegVT, Arg);
      break;
    case CCValAssign::ZExt:
      Arg = DAG.getNode(ISD::ZERO_EXTEND, DL, RegVT, Arg);
      break;
    case CCValAssign::AExt:
      Arg = DAG.getNode(ISD::ANY_EXTEND, DL, RegVT, Arg);
      break;
    case CCValAssign::BCvt:
      Arg = DAG.getNode(ISD::BITCAST, DL, RegVT, Arg);
      break;
    }

    // Stop when we encounter a stack argument, we need to process them
    // in reverse order in the loop below.
    if (VA.isMemLoc())
    {
      HasStkArgs = true;
      break;
    }

    // Arguments that can be passed on registers must be kept in the RegsToPass
    // vector.
    RegsToPass.push_back(std::make_pair(VA.getLocReg(), Arg));
  }

  // Second, stack arguments have to walked in reverse order by inserting
  // chained stores, this ensures their order is not changed by the scheduler
  // and that the push instruction sequence generated is correct, otherwise they
  // can be freely intermixed.
  if (HasStkArgs)
  {
    for (AE = AI, AI = ArgLocs.size(); AI != AE; --AI)
    {
      unsigned Loc = AI - 1;
      CCValAssign &VA = ArgLocs[Loc];
      SDValue Arg = OutVals[Loc];

      assert(VA.isMemLoc());

      // SP points to one stack slot further so add one to adjust it.
      SDValue PtrOff = DAG.getNode(ISD::ADD, DL, getPointerTy(),
                                   DAG.getRegister(AVR::SP, getPointerTy()),
                                   DAG.getIntPtrConstant(VA.getLocMemOffset()
                                                         + 1, DL));

      Chain = DAG.getStore(Chain, DL, Arg, PtrOff,
                           MachinePointerInfo::getStack(VA.getLocMemOffset()),
                           false, false, 0);
    }
  }

  // Build a sequence of copy-to-reg nodes chained together with token chain and
  // flag operands which copy the outgoing args into registers.  The InFlag in
  // necessary since all emited instructions must be stuck together.
  SDValue InFlag;
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
  {
    Chain = DAG.getCopyToReg(Chain, DL, RegsToPass[i].first,
                             RegsToPass[i].second, InFlag);
    InFlag = Chain.getValue(1);
  }

  // Returns a chain & a flag for retval copy to use.
  SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
  SmallVector<SDValue, 8> Ops;
  Ops.push_back(Chain);
  Ops.push_back(Callee);

  // Add argument registers to the end of the list so that they are known live
  // into the call.
  for (unsigned i = 0, e = RegsToPass.size(); i != e; ++i)
  {
    Ops.push_back(DAG.getRegister(RegsToPass[i].first,
                                  RegsToPass[i].second.getValueType()));
  }

  // Add a register mask operand representing the call-preserved registers.
  const AVRTargetMachine& TM = (const AVRTargetMachine&)getTargetMachine();
  const TargetRegisterInfo *TRI = TM.getSubtargetImpl()->getRegisterInfo();
  const uint32_t *Mask = TRI->getCallPreservedMask(DAG.getMachineFunction(), CallConv);
  assert(Mask && "Missing call preserved mask for calling convention");
  Ops.push_back(DAG.getRegisterMask(Mask));

  if (InFlag.getNode())
  {
    Ops.push_back(InFlag);
  }

  Chain = DAG.getNode(AVRISD::CALL, DL, NodeTys, Ops);
  InFlag = Chain.getValue(1);

  // Create the CALLSEQ_END node.
  Chain = DAG.getCALLSEQ_END(Chain, DAG.getIntPtrConstant(NumBytes, DL, true),
                             DAG.getIntPtrConstant(0, DL, true), InFlag, DL);

  if (!Ins.empty())
  {
    InFlag = Chain.getValue(1);
  }

  // Handle result values, copying them out of physregs into vregs that we
  // return.
  return LowerCallResult(Chain, InFlag, CallConv, isVarArg, Ins, DL, DAG,
                         InVals);
}

/// LowerCallResult - Lower the result values of a call into the
/// appropriate copies out of appropriate physical registers.
///
SDValue
AVRTargetLowering::LowerCallResult(SDValue Chain, SDValue InFlag,
                                   CallingConv::ID CallConv, bool isVarArg,
                                   const SmallVectorImpl<ISD::InputArg> &Ins,
                                   SDLoc dl, SelectionDAG &DAG,
                                   SmallVectorImpl<SDValue> &InVals) const
{
  // Assign locations to each value returned by this call.
  SmallVector<CCValAssign, 16> RVLocs;
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 RVLocs, *DAG.getContext());

  CCInfo.AnalyzeCallResult(Ins, RetCC_AVR);

  // Reverse splitted return values to get the "big endian" format required
  // to agree with the calling convention ABI.
  if (RVLocs.size() > 1)
  {
    std::reverse(RVLocs.begin(), RVLocs.end());
  }

  // Copy all of the result registers out of their specified physreg.
  for (unsigned i = 0, e = RVLocs.size(); i != e; ++i)
  {
    Chain = DAG.getCopyFromReg(Chain, dl, RVLocs[i].getLocReg(),
                               RVLocs[i].getValVT(), InFlag).getValue(1);
    InFlag = Chain.getValue(2);
    InVals.push_back(Chain.getValue(0));
  }

  return Chain;
}

//===----------------------------------------------------------------------===//
//               Return Value Calling Convention Implementation
//===----------------------------------------------------------------------===//

SDValue
AVRTargetLowering::LowerReturn(SDValue Chain, CallingConv::ID CallConv,
                               bool isVarArg,
                               const SmallVectorImpl<ISD::OutputArg> &Outs,
                               const SmallVectorImpl<SDValue> &OutVals,
                               SDLoc dl, SelectionDAG &DAG) const
{
  // CCValAssign - represent the assignment of the return value to locations.
  SmallVector<CCValAssign, 16> RVLocs;

  // CCState - Info about the registers and stack slot.
  CCState CCInfo(CallConv, isVarArg, DAG.getMachineFunction(),
                 RVLocs, *DAG.getContext());

  // Analize return values.
  CCInfo.AnalyzeReturn(Outs, RetCC_AVR);

  // If this is the first return lowered for this function, add the regs to
  // the liveout set for the function.
  MachineFunction &MF = DAG.getMachineFunction();
  unsigned e = RVLocs.size();

  // Reverse splitted return values to get the "big endian" format required
  // to agree with the calling convention ABI.
  if (e > 1)
  {
    std::reverse(RVLocs.begin(), RVLocs.end());
  }

  SDValue Flag;
  SmallVector<SDValue, 4> RetOps(1, Chain);
  // Copy the result values into the output registers.
  for (unsigned i = 0; i != e; ++i)
  {
    CCValAssign &VA = RVLocs[i];
    assert(VA.isRegLoc() && "Can only return in registers!");

    Chain = DAG.getCopyToReg(Chain, dl, VA.getLocReg(), OutVals[i], Flag);

    // Guarantee that all emitted copies are stuck together with flags.
    Flag = Chain.getValue(1);
    RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
  }

  // Don't emit the ret/reti instruction when the naked attribute is present in
  // the function being compiled.
  if (MF.getFunction()->getAttributes().
      hasAttribute(AttributeSet::FunctionIndex, Attribute::Naked))
  {
    return Chain;
  }

  unsigned RetOpc = (CallConv == CallingConv::AVR_INTR
                     || CallConv == CallingConv::AVR_SIGNAL) ?
                    AVRISD::RETI_FLAG : AVRISD::RET_FLAG;

  RetOps[0] = Chain;  // Update chain.

  if (Flag.getNode())
  {
    RetOps.push_back(Flag);
  }

  return DAG.getNode(RetOpc, dl, MVT::Other, RetOps);
}

//===----------------------------------------------------------------------===//
//  Other Lowering Code
//===----------------------------------------------------------------------===//

MachineBasicBlock*
AVRTargetLowering::EmitShiftInstr(MachineInstr *MI, MachineBasicBlock *BB) const
{
  unsigned Opc;
  const TargetRegisterClass *RC;
  MachineFunction *F = BB->getParent();
  MachineRegisterInfo &RI = F->getRegInfo();
  const AVRTargetMachine& TM = (const AVRTargetMachine&)getTargetMachine();
  const TargetInstrInfo &TII = *TM.getSubtargetImpl()->getInstrInfo();
  DebugLoc dl = MI->getDebugLoc();

  switch (MI->getOpcode())
  {
  default:
    llvm_unreachable("Invalid shift opcode!");
  case AVR::Lsl8:
    Opc = AVR::LSLRd;
    RC = &AVR::GPR8RegClass;
    break;
  case AVR::Lsl16:
    Opc = AVR::LSLWRd;
    RC = &AVR::DREGSRegClass;
    break;
  case AVR::Asr8:
    Opc = AVR::ASRRd;
    RC = &AVR::GPR8RegClass;
    break;
  case AVR::Asr16:
    Opc = AVR::ASRWRd;
    RC = &AVR::DREGSRegClass;
    break;
  case AVR::Lsr8:
    Opc = AVR::LSRRd;
    RC = &AVR::GPR8RegClass;
    break;
  case AVR::Lsr16:
    Opc = AVR::LSRWRd;
    RC = &AVR::DREGSRegClass;
    break;
  }

  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator I = BB;
  ++I;

  // Create loop block.
  MachineBasicBlock *LoopBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *RemBB  = F->CreateMachineBasicBlock(LLVM_BB);

  F->insert(I, LoopBB);
  F->insert(I, RemBB);

  // Update machine-CFG edges by transferring all successors of the current
  // block to the block containing instructions after shift.
  RemBB->splice(RemBB->begin(), BB,
                std::next(MachineBasicBlock::iterator(MI)),
                BB->end());
  RemBB->transferSuccessorsAndUpdatePHIs(BB);

  // Add adges BB => LoopBB => RemBB, BB => RemBB, LoopBB => LoopBB.
  BB->addSuccessor(LoopBB);
  BB->addSuccessor(RemBB);
  LoopBB->addSuccessor(RemBB);
  LoopBB->addSuccessor(LoopBB);

  unsigned ShiftAmtReg = RI.createVirtualRegister(&AVR::LD8RegClass);
  unsigned ShiftAmtReg2 = RI.createVirtualRegister(&AVR::LD8RegClass);
  unsigned ShiftReg = RI.createVirtualRegister(RC);
  unsigned ShiftReg2 = RI.createVirtualRegister(RC);
  unsigned ShiftAmtSrcReg = MI->getOperand(2).getReg();
  unsigned SrcReg = MI->getOperand(1).getReg();
  unsigned DstReg = MI->getOperand(0).getReg();

  // BB:
  // cp 0, N
  // breq RemBB
  BuildMI(BB, dl, TII.get(AVR::CPRdRr))
    .addReg(ShiftAmtSrcReg).addReg(AVR::R0);
  BuildMI(BB, dl, TII.get(AVR::BREQk))
    .addMBB(RemBB);

  // LoopBB:
  // ShiftReg = phi [%SrcReg, BB], [%ShiftReg2, LoopBB]
  // ShiftAmt = phi [%N, BB],      [%ShiftAmt2, LoopBB]
  // ShiftReg2 = shift ShiftReg
  // ShiftAmt2 = ShiftAmt - 1;
  BuildMI(LoopBB, dl, TII.get(AVR::PHI), ShiftReg)
    .addReg(SrcReg).addMBB(BB)
    .addReg(ShiftReg2).addMBB(LoopBB);
  BuildMI(LoopBB, dl, TII.get(AVR::PHI), ShiftAmtReg)
    .addReg(ShiftAmtSrcReg).addMBB(BB)
    .addReg(ShiftAmtReg2).addMBB(LoopBB);
  BuildMI(LoopBB, dl, TII.get(Opc), ShiftReg2)
    .addReg(ShiftReg);
  BuildMI(LoopBB, dl, TII.get(AVR::SUBIRdK), ShiftAmtReg2)
    .addReg(ShiftAmtReg).addImm(1);
  BuildMI(LoopBB, dl, TII.get(AVR::BRNEk))
    .addMBB(LoopBB);

  // RemBB:
  // DestReg = phi [%SrcReg, BB], [%ShiftReg, LoopBB]
  BuildMI(*RemBB, RemBB->begin(), dl, TII.get(AVR::PHI), DstReg)
    .addReg(SrcReg).addMBB(BB)
    .addReg(ShiftReg2).addMBB(LoopBB);

  MI->eraseFromParent();   // The pseudo instruction is gone now.
  return RemBB;
}

MachineBasicBlock *
AVRTargetLowering::EmitInstrWithCustomInserter(MachineInstr *MI,
                                               MachineBasicBlock *BB) const
{
  int Opc = MI->getOpcode();

  // Pseudo shift instructions with a non constant shift amount are expanded
  // into a loop.
  switch (Opc)
  {
  case AVR::Lsl8:
  case AVR::Lsl16:
  case AVR::Lsr8:
  case AVR::Lsr16:
  case AVR::Asr8:
  case AVR::Asr16:
    return EmitShiftInstr(MI, BB);
  }

  assert((Opc == AVR::Select16 || Opc == AVR::Select8)
         && "Unexpected instr type to insert");

  const AVRInstrInfo &TII = (const AVRInstrInfo&) *MI->getParent()->getParent()->getSubtarget().getInstrInfo();
  DebugLoc dl = MI->getDebugLoc();

  // To "insert" a SELECT instruction, we actually have to insert the diamond
  // control-flow pattern.  The incoming instruction knows the destination vreg
  // to set, the condition code register to branch on, the true/false values to
  // select between, and a branch opcode to use.
  const BasicBlock *LLVM_BB = BB->getBasicBlock();
  MachineFunction::iterator I = BB;
  ++I;

  //  thisMBB:
  //  ...
  //   TrueVal = ...
  //   cmpTY ccX, r1, r2
  //   jCC copy1MBB
  //   fallthrough --> copy0MBB
  MachineBasicBlock *thisMBB = BB;
  MachineFunction *F = BB->getParent();
  MachineBasicBlock *copy0MBB = F->CreateMachineBasicBlock(LLVM_BB);
  MachineBasicBlock *copy1MBB = F->CreateMachineBasicBlock(LLVM_BB);
  AVRCC::CondCodes CC = (AVRCC::CondCodes)MI->getOperand(3).getImm();
  F->insert(I, copy0MBB);
  F->insert(I, copy1MBB);
  // Update machine-CFG edges by transferring all successors of the current
  // block to the new block which will contain the Phi node for the select.
  copy1MBB->splice(copy1MBB->begin(), BB,
                   std::next(MachineBasicBlock::iterator(MI)),
                   BB->end());
  copy1MBB->transferSuccessorsAndUpdatePHIs(BB);
  // Next, add the true and fallthrough blocks as its successors.
  BB->addSuccessor(copy0MBB);
  BB->addSuccessor(copy1MBB);

  BuildMI(BB, dl, TII.getBrCond(CC)).addMBB(copy1MBB);

  //  copy0MBB:
  //   %FalseValue = ...
  //   # fallthrough to copy1MBB
  BB = copy0MBB;

  // Update machine-CFG edges.
  BB->addSuccessor(copy1MBB);

  //  copy1MBB:
  //   %Result = phi [ %FalseValue, copy0MBB ], [ %TrueValue, thisMBB ]
  //  ...
  BB = copy1MBB;
  BuildMI(*BB, BB->begin(), dl, TII.get(AVR::PHI),
          MI->getOperand(0).getReg())
    .addReg(MI->getOperand(2).getReg()).addMBB(copy0MBB)
    .addReg(MI->getOperand(1).getReg()).addMBB(thisMBB);

  MI->eraseFromParent();   // The pseudo instruction is gone now.
  return BB;
}

//===----------------------------------------------------------------------===//
//  Inline Asm Support
//===----------------------------------------------------------------------===//

AVRTargetLowering::ConstraintType
AVRTargetLowering::getConstraintType(const std::string &Constraint) const
{
  if (Constraint.size() == 1)
  {
    switch (Constraint[0])
    {
    case 'a':
    case 'b':
    case 'd':
    case 'l':
    case 'e':
    case 'q':
    case 'r':
    case 'w':
      return C_RegisterClass;
    case 't':
    case 'x':
    case 'y':
    case 'z':
      return C_Register;
    case 'Q':
      return C_Memory;
    case 'G':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'R':
      return C_Other;
    default:
      break;
    }
  }

  return TargetLowering::getConstraintType(Constraint);
}

AVRTargetLowering::ConstraintWeight
AVRTargetLowering::getSingleConstraintMatchWeight(AsmOperandInfo &info,
                                                  const char *constraint) const
{
  ConstraintWeight weight = CW_Invalid;
  Value *CallOperandVal = info.CallOperandVal;

  // If we don't have a value, we can't do a match,
  // but allow it at the lowest weight.
  // (this behaviour has been copied from the ARM backend)
  if (CallOperandVal == NULL)
  {
    return CW_Default;
  }

  // Look at the constraint type.
  switch (*constraint)
  {
  default:
    weight = TargetLowering::getSingleConstraintMatchWeight(info, constraint);
    break;
  case 'd':
  case 'r':
  case 'l':
    weight = CW_Register;
    break;
  case 'a':
  case 'b':
  case 'e':
  case 'q':
  case 't':
  case 'w':
  case 'x':
  case 'y':
  case 'z':
    weight = CW_SpecificReg;
    break;
  case 'G':
    if (const ConstantFP *C = dyn_cast<ConstantFP>(CallOperandVal))
    {
      if (C->isZero())
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'I':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if (isUInt<6>(C->getZExtValue()))
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'J':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if ((C->getSExtValue() >= -63) && (C->getSExtValue() <= 0))
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'K':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if (C->getZExtValue() == 2)
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'L':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if (C->getZExtValue() == 0)
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'M':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if (isUInt<8>(C->getZExtValue()))
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'N':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if (C->getSExtValue() == -1)
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'O':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if ((C->getZExtValue() == 8) || (C->getZExtValue() == 16)
          || (C->getZExtValue() == 24))
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'P':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if (C->getZExtValue() == 1)
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'R':
    if (const ConstantInt *C = dyn_cast<ConstantInt>(CallOperandVal))
    {
      if ((C->getSExtValue() >= -6) && (C->getSExtValue() <= 5))
      {
        weight = CW_Constant;
      }
    }
    break;
  case 'Q':
    weight = CW_Memory;
    break;
  }

  return weight;
}

std::pair<unsigned, const TargetRegisterClass *>
AVRTargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                                const std::string &Constraint,
                                                MVT VT) const
{
  auto STI = static_cast<const AVRTargetMachine&>(this->getTargetMachine()).getSubtargetImpl();

  // We only support i8 and i16.
  //
  //:FIXME: remove this assert for now since it gets sometimes executed
  //assert((VT == MVT::i16 || VT == MVT::i8) && "Wrong operand type.");

  if (Constraint.size() == 1)
  {
    switch (Constraint[0])
    {
    case 'a': // Simple upper registers r16..r23.
      return std::make_pair(0U, &AVR::LD8loRegClass);
    case 'b': // Base pointer registers: y, z.
      return std::make_pair(0U, &AVR::PTRDISPREGSRegClass);
    case 'd': // Upper registers r16..r31.
      return std::make_pair(0U, &AVR::LD8RegClass);
    case 'l': // Lower registers r0..r15.
      return std::make_pair(0U, &AVR::GPR8loRegClass);
    case 'e': // Pointer register pairs: x, y, z.
      return std::make_pair(0U, &AVR::PTRREGSRegClass);
    case 'q': // Stack pointer register: SPH:SPL.
      return std::make_pair(0U, &AVR::GPRSPRegClass);
    case 'r': // Any register: r0..r31.
      if (VT == MVT::i8)
      {
        return std::make_pair(0U, &AVR::GPR8RegClass);
      }
      else
      {
        return std::make_pair(0U, &AVR::DREGSRegClass);
      }
    case 't': // Temporary register: r0.
      return std::make_pair(unsigned(AVR::R0), &AVR::GPR8RegClass);
    case 'w': // Special upper register pairs: r24, r26, r28, r30.
      return std::make_pair(0U, &AVR::IWREGSRegClass);
    case 'x': // Pointer register pair X: r27:r26.
      return std::make_pair(unsigned(AVR::R27R26), &AVR::PTRREGSRegClass);
    case 'y': // Pointer register pair Y: r29:r28.
      return std::make_pair(unsigned(AVR::R29R28), &AVR::PTRREGSRegClass);
    case 'z': // Pointer register pair Z: r31:r30.
      return std::make_pair(unsigned(AVR::R31R30), &AVR::PTRREGSRegClass);
    default:
      break;
    }
  }

  return TargetLowering::getRegForInlineAsmConstraint(STI->getRegisterInfo(), Constraint, VT);
}

void AVRTargetLowering::LowerAsmOperandForConstraint(SDValue Op,
                                                     std::string &Constraint,
                                                     std::vector<SDValue> &Ops,
                                                     SelectionDAG &DAG) const
{
  SDValue Result(0, 0);
  SDLoc DL(Op);
  EVT Ty = Op.getValueType();

  // Currently only support length 1 constraints.
  if (Constraint.length() != 1)
  {
    return;
  }

  char ConstraintLetter = Constraint[0];
  switch (ConstraintLetter)
  {
  default:
    break;
  // Deal with integers first:
  case 'I':
  case 'J':
  case 'K':
  case 'L':
  case 'M':
  case 'N':
  case 'O':
  case 'P':
  case 'R':
    {
      const ConstantSDNode *C = dyn_cast<ConstantSDNode>(Op);
      if (!C)
      {
        return;
      }

      int64_t CVal64 = C->getSExtValue();
      uint64_t CUVal64 = C->getZExtValue();
      switch (ConstraintLetter)
      {
      case 'I': // 0..63
        if (!isUInt<6>(CUVal64)) return;
        Result = DAG.getTargetConstant(CUVal64, DL, Ty);
        break;
      case 'J': // -63..0
        if (CVal64 < -63 || CVal64 > 0) return;
        Result = DAG.getTargetConstant(CVal64, DL, Ty);
        break;
      case 'K': // 2
        if (CUVal64 != 2) return;
        Result = DAG.getTargetConstant(CUVal64, DL, Ty);
        break;
      case 'L': // 0
        if (CUVal64 != 0) return;
        Result = DAG.getTargetConstant(CUVal64, DL, Ty);
        break;
      case 'M': // 0..255
        if (!isUInt<8>(CUVal64)) return;
        // i8 type may be printed as a negative number,
        // e.g. 254 would be printed as -2,
        // so we force it to i16 at least.
        if (Ty.getSimpleVT() == MVT::i8)
        {
          Ty = MVT::i16;
        }
        Result = DAG.getTargetConstant(CUVal64, DL, Ty);
        break;
      case 'N': // -1
        if (CVal64 != -1) return;
        Result = DAG.getTargetConstant(CVal64, DL, Ty);
        break;
      case 'O': // 8, 16, 24
        if (CUVal64 != 8 && CUVal64 != 16 && CUVal64 != 24) return;
        Result = DAG.getTargetConstant(CUVal64, DL, Ty);
        break;
      case 'P': // 1
        if (CUVal64 != 1) return;
        Result = DAG.getTargetConstant(CUVal64, DL, Ty);
        break;
      case 'R': // -6..5
        if (CVal64 < -6 || CVal64 > 5) return;
        Result = DAG.getTargetConstant(CVal64, DL, Ty);
        break;
      }

      break;
    }
  case 'G':
    const ConstantFPSDNode *FC = dyn_cast<ConstantFPSDNode>(Op);
    if (!FC || !FC->isZero())
      return;
    // Soften float to i8 0
    Result = DAG.getTargetConstant(0, DL, MVT::i8);
    break;
  }

  if (Result.getNode())
  {
    Ops.push_back(Result);
    return;
  }

  return TargetLowering::LowerAsmOperandForConstraint(Op, Constraint, Ops, DAG);
}

} // end of namespace llvm
