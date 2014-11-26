//===-- AVRInstrInfo.td - AVR Instruction defs -------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file describes the AVR instructions in TableGen format.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Instruction format superclass
//===----------------------------------------------------------------------===//

include "AVRInstrFormats.td"

//===----------------------------------------------------------------------===//
// AVR Type Profiles
//===----------------------------------------------------------------------===//

def SDT_AVRCallSeqStart : SDCallSeqStart<[SDTCisVT<0, i16>]>;
def SDT_AVRCallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i16>, SDTCisVT<1, i16>]>;
def SDT_AVRCall : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_AVRWrapper : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def SDT_AVRBrcond : SDTypeProfile<0, 2,
                                  [SDTCisVT<0, OtherVT>, SDTCisVT<1, i8>]>;
def SDT_AVRCmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_AVRTst : SDTypeProfile<0, 1, [SDTCisInt<0>]>;
def SDT_AVRSelectCC : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>,
                                    SDTCisSameAs<1, 2>, SDTCisVT<3, i8>]>;

//===----------------------------------------------------------------------===//
// AVR Specific Node Definitions
//===----------------------------------------------------------------------===//

def AVRretflag : SDNode<"AVRISD::RET_FLAG", SDTNone,
                        [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def AVRretiflag : SDNode<"AVRISD::RETI_FLAG", SDTNone,
                         [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;

def AVRcallseq_start : SDNode<"ISD::CALLSEQ_START", SDT_AVRCallSeqStart,
                              [SDNPHasChain, SDNPOutGlue]>;
def AVRcallseq_end : SDNode<"ISD::CALLSEQ_END", SDT_AVRCallSeqEnd,
                            [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;

def AVRcall : SDNode<"AVRISD::CALL", SDT_AVRCall,
                     [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;

def AVRWrapper : SDNode<"AVRISD::Wrapper", SDT_AVRWrapper>;

def AVRbrcond : SDNode<"AVRISD::BRCOND", SDT_AVRBrcond,
                       [SDNPHasChain, SDNPInGlue]>;
def AVRcmp : SDNode<"AVRISD::CMP", SDT_AVRCmp, [SDNPOutGlue]>;
def AVRcmpc : SDNode<"AVRISD::CMPC", SDT_AVRCmp, [SDNPInGlue, SDNPOutGlue]>;
def AVRtst : SDNode<"AVRISD::TST", SDT_AVRTst, [SDNPOutGlue]>;
def AVRselectcc: SDNode<"AVRISD::SELECT_CC", SDT_AVRSelectCC, [SDNPInGlue]>;

// shift nodes
def AVRlsl : SDNode<"AVRISD::LSL", SDTIntUnaryOp>;
def AVRlsr : SDNode<"AVRISD::LSR", SDTIntUnaryOp>;
def AVRrol : SDNode<"AVRISD::ROL", SDTIntUnaryOp>;
def AVRror : SDNode<"AVRISD::ROR", SDTIntUnaryOp>;
def AVRasr : SDNode<"AVRISD::ASR", SDTIntUnaryOp>;

// pseudo shift nodes for non-constant shift amounts
def AVRlslLoop : SDNode<"AVRISD::LSLLOOP", SDTIntShiftOp>;
def AVRlsrLoop : SDNode<"AVRISD::LSRLOOP", SDTIntShiftOp>;
def AVRasrLoop : SDNode<"AVRISD::ASRLOOP", SDTIntShiftOp>;

//===----------------------------------------------------------------------===//
// AVR Operands, Complex Patterns and Transformations Definitions.
//===----------------------------------------------------------------------===//

def imm8_neg_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), MVT::i8);
}]>;

def imm16_neg_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), MVT::i16);
}]>;

def imm0_63_neg : PatLeaf<(imm),
[{
  int64_t val = -N->getSExtValue();
  return val >= 0 && val < 64;
}], imm16_neg_XFORM>;

def uimm6 : PatLeaf<(imm), [{ return isUInt<6>(N->getZExtValue()); }]>;

def ioaddr_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(uint8_t(N->getZExtValue()) - 0x20, MVT::i8);
}]>;

def iobitpos8_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(Log2_32(uint8_t(N->getZExtValue())),
                                   MVT::i8);
}]>;

def iobitposn8_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(Log2_32(uint8_t(~N->getZExtValue())),
                                   MVT::i8);
}]>;

def ioaddr8 : PatLeaf<(imm),
[{
  uint64_t val = N->getZExtValue();
  return val >= 0x20 && val < 0x60;
}], ioaddr_XFORM>;

def lowioaddr8 : PatLeaf<(imm),
[{
  uint64_t val = N->getZExtValue();
  return val >= 0x20 && val < 0x40;
}], ioaddr_XFORM>;

def ioaddr16 : PatLeaf<(imm),
[{
  uint64_t val = N->getZExtValue();
  return val >= 0x20 && val < 0x5f;
}], ioaddr_XFORM>;

def iobitpos8 : PatLeaf<(imm),
[{
  return isPowerOf2_32(uint8_t(N->getZExtValue()));
}], iobitpos8_XFORM>;

def iobitposn8 : PatLeaf<(imm),
[{
  return isPowerOf2_32(uint8_t(~N->getZExtValue()));
}], iobitposn8_XFORM>;

// Address operand for reg+imm
def memri : Operand<iPTR>
{
  let PrintMethod = "printMemriOperand";
  let MIOperandInfo = (ops PTRDISPREGS, i8imm);
}

// Address operand for SP+imm used by STD{W}SPQRr
def memspi : Operand<iPTR>
{
  let MIOperandInfo = (ops GPRSP, i16imm);
}

// Branch targets have OtherVT type and print as pc-relative values.
def brtarget : Operand<OtherVT>
{
  let PrintMethod = "print_pcrel_imm";
}

// Addressing mode pattern reg+imm6
def addr : ComplexPattern<iPTR, 2, "SelectAddr", [], [SDNPWantRoot]>;

// AVR specific condition code. These correspond to AVR_*_COND in
// AVRInstrInfo.td. They must be kept in synch.
def AVR_COND_EQ : PatLeaf<(i8 0)>;
def AVR_COND_NE : PatLeaf<(i8 1)>;
def AVR_COND_GE : PatLeaf<(i8 2)>;
def AVR_COND_LT : PatLeaf<(i8 3)>;
def AVR_COND_SH : PatLeaf<(i8 4)>;
def AVR_COND_LO : PatLeaf<(i8 5)>;
def AVR_COND_MI : PatLeaf<(i8 6)>;
def AVR_COND_PL : PatLeaf<(i8 7)>;

// :TODO: TEMPORARY INSTRUCTION LISTING, leave it here until we're done with it
// Arithmetic instructions
/*
  ADD RD, RR
  ADC RD, RR
  ADIW RD, K

  SUB RD, RR
  SUBI RD, K
  SBC RD, RR
  SBCI RD, K
  SBIW RD, K

  INC RD
  DEC RD

  MUL RD, RR
  MULS RD, RR
  MULSU RD, RR
  FMUL RD, RR
  FMULS RD, RR
  FMULSU RD, RR

  DES??
*/

/*
def ADDrr       0000 11rd dddd rrrr ok
def ADCrr       0001 11rd dddd rrrr ok
def ADIWri      1001 0110 KKdd KKKK ok

def SUBrr       0001 10rd dddd rrrr ok
def SUBIri      0101 KKKK dddd KKKK ok
def SBCrr       0000 10rd dddd rrrr ok
def SBCIri      0100 KKKK dddd KKKK ok
def SBIWri      1001 0111 KKdd KKKK ok

def INCr        1001 010d dddd 0011 ok
def DECr        1001 010d dddd 1010 ok

def MULrr       1001 11rd dddd rrrr ok
def MULSrr      0000 0010 dddd rrrr ok
def MULSUrr     0000 0011 0ddd 0rrr ok
def FMULrr      0000 0011 0ddd 1rrr ok
def FMULSrr     0000 0011 1ddd 0rrr ok
def FMULSUrr    0000 0011 1ddd 1rrr ok
*/
//def DES :TODO:

// Logic instructions
/*
  AND RD, RR
  ANDI RD, K

  OR RD, RR
  ORI RD, K

  EOR RD, RR

  COM RD
  NEG RD

  SBR RD, K
  CBR RD, K

  TST RD

  CLR RD

  SER RD
*/
/*
def ANDrr           0010 00rd dddd rrrr ok
def ANDIri          0111 KKKK dddd KKKK ok
def ORrr            0010 10rd dddd rrrr ok
def ORIri           0110 KKKK dddd KKKK ok
def EORrr           0010 01rd dddd rrrr ok
def COMr            1001 010d dddd 0000 ok
def NEGr            1001 010d dddd 0001 ok
def SBRri           0110 KKKK dddd KKKK ok
def CBRri           SAME AS   ANDI      ok
def TSTr            0010 00dd dddd dddd (SAME AS AND RD, RD) ok
def CLRr            0010 01dd dddd dddd (SAME AS EOR RD, RD) ok
def SERr            1110 1111 dddd 1111 (SAME AS LDI RD, 0xFF) ok

// Branch instructions
def RJMPi           1100 kkkk kkkk kkkk ok  //:TODO: use a for addr instead of i?
def IJMP            1001 0100 0000 1001 ok
def JMPi            1001 010k kkkk 110k kkkk kkkk kkkk kkkk

def RCALLi          1101 kkkk kkkk kkkk ok
def ICALL           1001 0101 0000 1001 ok
def EICALL          1001 0101 0001 1001 ok
def CALLi           1001 010k kkkk 111k kkkk kkkk kkkk kkkk
def RET             1001 0101 0000 1000 ok
def RETI            1001 0101 0001 1000 ok

def CPSErr          0001 00rd dddd rrrr ok
def CPrr            0001 01rd dddd rrrr ok
def CPCrr           0000 01rd dddd rrrr ok
def CPIri           0011 kkkk dddd kkkk ok

def SBRCrb          1111 110r rrrr 0bbb
def SBRSrb          1111 111r rrrr 0bbb
def SBICab          1001 1001 AAAA Abbb
def SBISab          1001 1011 AAAA Abbb

def BRBSsi          1111 00kk kkkk ksss ok
def BRBCsi          1111 01kk kkkk ksss ok

def BREQi           1111 00kk kkkk k001 ok
def BRNEi           1111 01kk kkkk k001 ok
def BRCSi           1111 00kk kkkk k000 ok  (SAME AS BRLO)
def BRCCi           1111 01kk kkkk k000 ok  (SAME AS BRSH)
def BRSHi           1111 01kk kkkk k000 ok
def BRLOi           1111 00kk kkkk k000 ok
def BRMIi           1111 00kk kkkk k010 ok
def BRPLi           1111 01kk kkkk k010 ok
def BRGEi           1111 01kk kkkk k100 ok
def BRLTi           1111 00kk kkkk k100 ok
def BRHSi           1111 00kk kkkk k101 ok
def BRHCi           1111 01kk kkkk k101 ok
def BRTSi           1111 00kk kkkk k110 ok
def BRTCi           1111 01kk kkkk k110 ok
def BRVSi           1111 00kk kkkk k011 ok
def BRVCi           1111 01kk kkkk k011 ok
def BRIEi           1111 00kk kkkk k111 ok
def BRIDi           1111 01kk kkkk k111 ok

// data transfer instructions
def MOVrr           0010 11rd dddd rrrr ok
def MOVWrr          0000 0001 dddd rrrr

def LDIri           1110 kkkk dddd kkkk ok
def LDSri           1001 000d dddd 0000 kkkk kkkk kkkk kkkk

def LDrX            1001 000d dddd 1100
def LDrXpi          1001 000d dddd 1101     //postinc
def LDrXpd          1001 000d dddd 1110//predec

def LDrY            1000 000d dddd 1000
def LDrYpi          1001 000d dddd 1001
def LDrYpd          1001 000d dddd 1010

def LDDrYq          10q0 qq0d dddd 1qqq

def LDrZ            1000 000d dddd 0000
def LDrZpi          1001 000d dddd 0001
def LDrZpd          1001 000d dddd 0010

def LDDrZq          10q0 qq0d dddd 0qqq

def STSir           1001 001d dddd 0000 kkkk kkkk kkkk kkkk
def STXr            1001 001d dddd 1100     // watch out: r cant be X
def STXpir          1001 001d dddd 1101
def STXpdr          1001 001d dddd 1110

def STYr            1000 001d dddd 1000     // watch out: r cant Y
def STYpir          1001 001d dddd 1001
def STYpdr          1001 001d dddd 1010
def STDYqr          10q0 qq1d dddd 1qqq

def STZr            1000 001d dddd 0000
def STZpir          1001 001d dddd 0001
def STZpdr          1001 001d dddd 0010
def STDqr           10q0 qq1d dddd 0qqq

def LPM             1001 0101 1100 1000     //(R0)<-(Z)
def LPMrZ           1001 000d dddd 0100
def LPMrZpi         1001 000d dddd 0101

def ELPM            1001 0101 1101 1000
def ELPMrZ          1001 000d dddd 0110
def ELPMrZpi        1001 000d dddd 0111

def SPM             //:TODO:
def SPMZpi

def INra            1011 0AAd dddd AAAA ok
def OUTar           1011 1AAd dddd AAAA ok

def PUSHr           1001 001d dddd 1111 ok
def POPr            1001 000d dddd 1111 ok

//bit and bit-test instructions
def LSLr            0000 11dd dddd dddd   ok  (SAME AS ADD rd, rd)
def LSRr            1001 010d dddd 0110   ok
def ROLr            0001 11dd dddd dddd   ok  (SAME AS ADC rd, rd)
def RORr            1001 010d dddd 0111   ok

def ASRr            1001 010d dddd 0101  ok

def SWAPr           1001 010d dddd 0010 ok

def SBIab           1001 1010 AAAA Abbb
def CBIab           1001 1000 AAAA Abbb

def BSTrb           1111 101d dddd 0bbb
def BLDrb           1111 100d dddd 0bbb

def BSETs           1001 0100 0sss 1000
def BCLRs           1001 0100 1sss 1000 // probably all the todo below comes from here
//:TODO: all SREG instr manipulation (SEC, CLC, SEN, etc...)

// MCU control instructions
def NOP             0000 0000 0000 0000
*/
//__________________________________________________________________________________________
//__________________________________________________________________________________________
//__________________________________________________________________________________________
//__________________________________________________________________________________________
//__________________________________________________________________________________________
//__________________________________________________________________________________________
//__________________________________________________________________________________________
//__________________________________________________________________________________________

//===----------------------------------------------------------------------===//
// AVR Instruction list
//===----------------------------------------------------------------------===//

// ADJCALLSTACKDOWN/UP implicitly use/def SP because they may be expanded into
// a stack adjustment and the codegen must know that they may modify the stack
// pointer before prolog-epilog rewriting occurs.
// Pessimistically assume ADJCALLSTACKDOWN / ADJCALLSTACKUP will become
// sub / add which can clobber SREG.
let Defs = [SP, SREG],
Uses = [SP] in
{
def ADJCALLSTACKDOWN : Pseudo<(outs),
                              (ins i16imm:$amt),
                              "#ADJCALLSTACKDOWN",
                              [(AVRcallseq_start timm:$amt)]>;

// R31R30 is used to update SP, since it is a scratch reg and this instruction
// is placed after the function call then R31R30 should be always free.
//let Defs = [R31R30],
//Uses = [R31R30] in
//:TODO: if we enable this, the pseudo is killed because it looks dead
def ADJCALLSTACKUP : Pseudo<(outs),
                            (ins i16imm:$amt1, i16imm:$amt2),
                            "#ADJCALLSTACKUP",
                            [(AVRcallseq_end timm:$amt1, timm:$amt2)]>;
}

// Arithmetic

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ADDRdRr : FRdRr<0b0000,
                    0b11,
                    (outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$src2),
                    "add\t$dst, $src2",
                    [(set GPR8:$dst, (add GPR8:$src, GPR8:$src2)),
                     (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ADDWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "addw\t$dst, $src2",
                      [(set DREGS:$dst, (add DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def ADCRdRr : FRdRr<0b0001,
                    0b11,
                    (outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$src2),
                    "adc\t$dst, $src2",
                    [(set GPR8:$dst, (adde GPR8:$src, GPR8:$src2)),
                     (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def ADCWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "adcw\t$dst, $src2",
                      [(set DREGS:$dst, (adde DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ADIWRdK : FWRdK<0b0,
                    (outs IWREGS:$dst),
                    (ins IWREGS:$src, i16imm:$src2),
                    "adiw\t$dst, $src2",
                    [(set IWREGS:$dst, (add IWREGS:$src, uimm6:$src2)),
                     (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def SUBRdRr : FRdRr<0b0001,
                    0b10,
                    (outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$src2),
                    "sub\t$dst, $src2",
                    [(set GPR8:$dst, (sub GPR8:$src, GPR8:$src2)),
                     (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def SUBWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "subw\t$dst, $src2",
                      [(set DREGS:$dst, (sub DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def SUBIRdK : FRdK<0b0101,
                   (outs LD8:$dst),
                   (ins LD8:$src, i8imm:$src2),
                   "subi\t$dst, $src2",
                   [(set LD8:$dst, (sub LD8:$src, imm:$src2)),
                    (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def SUBIWRdK : Pseudo<(outs DLDREGS:$dst),
                      (ins DLDREGS:$src, i16imm:$src2),
                      "subiw\t$dst, $src2",
                      [(set DLDREGS:$dst, (sub DLDREGS:$src, imm:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def SBCRdRr : FRdRr<0b0000,
                    0b10,
                    (outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$src2),
                    "sbc\t$dst, $src2",
                    [(set GPR8:$dst, (sube GPR8:$src, GPR8:$src2)),
                     (implicit SREG)]>;

let Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def SBCWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "sbcw\t$dst, $src2",
                      [(set DREGS:$dst, (sube DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def SBCIRdK : FRdK<0b0100,
                   (outs LD8:$dst),
                   (ins LD8:$src, i8imm:$src2),
                   "sbci\t$dst, $src2",
                   [(set LD8:$dst, (sube LD8:$src, imm:$src2)),
                    (implicit SREG)]>;

let Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def SBCIWRdK : Pseudo<(outs DLDREGS:$dst),
                      (ins DLDREGS:$src, i16imm:$src2),
                      "sbciw\t$dst, $src2",
                      [(set DLDREGS:$dst, (sube DLDREGS:$src, imm:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def SBIWRdK : FWRdK<0b1,
                    (outs IWREGS:$dst),
                    (ins IWREGS:$src, i16imm:$src2),
                    "sbiw\t$dst, $src2",
                    [(set IWREGS:$dst, (sub IWREGS:$src, uimm6:$src2)),
                     (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def INCRd : FRd<0b1001,
                0b0100011,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "inc\t$dst",
                [(set GPR8:$dst, (add GPR8:$src, 1)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def DECRd : FRd<0b1001,
                0b0101010,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "dec\t$dst",
                [(set GPR8:$dst, (add GPR8:$src, -1)), (implicit SREG)]>;

// This pseudo is used as a marker to insert a "clr r1" after the operation.
let isCommutable = 1,
Defs = [R1, R0, SREG] in
def MULRdRrP : Pseudo<(outs GPR8:$dst),
                      (ins GPR8:$src, GPR8:$src2),
                      "mulp\t$dst, $src, $src2",
                      [(set GPR8:$dst, (mul GPR8:$src, GPR8:$src2)),
                       (implicit SREG)]>;

let isCommutable = 1,
Defs = [R1, R0, SREG],
neverHasSideEffects = 1 in
def MULRdRr : Mu1RdRr<(outs GPR8:$dst),
                      (ins GPR8:$src, GPR8:$src2),
                      "mul\t$src, $src2",
                      []>;

// The earlyclobber flag is required because the pseudo expansion needs $dst
// and $src to have different registers.
let isCommutable = 1,
Constraints = "@earlyclobber $dst",
Defs = [R1, R0, SREG] in
def MULWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "mulw\t$dst, $src, $src2",
                      [(set DREGS:$dst, (mul DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

// This instruction produces a 16bit result from two 8bit operands.
let isCommutable = 1,
Defs = [R1R0, SREG],
neverHasSideEffects = 1 in
def MUL16RdRr : Mu1RdRr<(outs DREGS:$dst),
                        (ins GPR8:$src, GPR8:$src2),
                        "mul\t$src, $src2",
                        []>;

/*
def MULSrr      0000 0010 dddd rrrr ok
def MULSUrr     0000 0011 0ddd 0rrr ok
def FMULrr      0000 0011 0ddd 1rrr ok
def FMULSrr     0000 0011 1ddd 0rrr ok
def FMULSUrr    0000 0011 1ddd 1rrr ok
*/
//def DES :TODO:

//_______________________________________
// logic instructions

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ANDRdRr : FRdRr<0b0010,
                    0b00,
                    (outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$src2),
                    "and\t$dst, $src2",
                    [(set GPR8:$dst, (and GPR8:$src, GPR8:$src2)),
                     (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ANDWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "andw\t$dst, $src2",
                      [(set DREGS:$dst, (and DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ANDIRdK : FRdK<0b0111,
                   (outs LD8:$dst),
                   (ins LD8:$src, i8imm:$src2),
                   "andi\t$dst, $src2",
                   [(set LD8:$dst, (and LD8:$src, imm:$src2)),
                    (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ANDIWRdK : Pseudo<(outs DLDREGS:$dst),
                      (ins DLDREGS:$src, i16imm:$src2),
                      "andiw\t$dst, $src2",
                      [(set DLDREGS:$dst, (and DLDREGS:$src, imm:$src2)),
                       (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ORRdRr : FRdRr<0b0010,
                   0b10,
                   (outs GPR8:$dst),
                   (ins GPR8:$src, GPR8:$src2),
                   "or\t$dst, $src2",
                   [(set GPR8:$dst, (or GPR8:$src, GPR8:$src2)),
                    (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ORWRdRr : Pseudo<(outs DREGS:$dst),
                     (ins DREGS:$src, DREGS:$src2),
                     "orw\t$dst, $src2",
                     [(set DREGS:$dst, (or DREGS:$src, DREGS:$src2)),
                      (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ORIRdK : FRdK<0b0110,
                  (outs LD8:$dst),
                  (ins LD8:$src, i8imm:$src2),
                  "ori\t$dst, $src2",
                  [(set LD8:$dst, (or LD8:$src, imm:$src2)),
                   (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ORIWRdK : Pseudo<(outs DLDREGS:$dst),
                     (ins DLDREGS:$src, i16imm:$src2),
                     "oriw\t$dst, $src2",
                     [(set DLDREGS:$dst, (or DLDREGS:$src, imm:$src2)),
                      (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def EORRdRr : FRdRr<0b0010,
                    0b01,
                    (outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$src2),
                    "eor\t$dst, $src2",
                    [(set GPR8:$dst, (xor GPR8:$src, GPR8:$src2)),
                     (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def EORWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "eorw\t$dst, $src2",
                      [(set DREGS:$dst, (xor DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def COMRd : FRd<0b1001,
                0b0100000,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "com\t$dst",
                [(set GPR8:$dst, (not GPR8:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def COMWRd : Pseudo<(outs DREGS:$dst),
                    (ins DREGS:$src),
                    "comw\t$dst",
                    [(set DREGS:$dst, (not DREGS:$src)), (implicit SREG)]>;

//:TODO: optimize NEG for wider types
let Constraints = "$src = $dst",
Defs = [SREG] in
def NEGRd : FRd<0b1001,
                0b0100001,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "neg\t$dst",
                [(set GPR8:$dst, (ineg GPR8:$src)), (implicit SREG)]>;

/*
def SBRri           0110 KKKK dddd KKKK ok
def CBRri           SAME AS   ANDI      ok
*/

let Defs = [SREG] in
def TSTRd : Pseudo<(outs),
                   (ins GPR8:$src),
                   "tst\t$src",
                   [(AVRtst GPR8:$src), (implicit SREG)]>;

/*
def CLRr            0010 01dd dddd dddd (SAME AS EOR RD, RD) ok
def SERr            1110 1111 dddd 1111 (SAME AS LDI RD, 0xFF) ok
*/

// Branch instructions

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def RJMPk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "rjmp\t$target",
                   [(br bb:$target)]>;

let isBranch = 1,
isTerminator = 1,
isBarrier = 1,
isIndirectBranch = 1,
Uses = [R31R30] in
def IJMP : FBR<0b1001010000001001,
               (outs),
               (ins),
               "ijmp",
               []>;

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def JMPk : F32BRk<0b110,
                  (outs),
                  (ins brtarget:$target),
                  "jmp\t$target",
                  []>;
/*
def RCALLi          1101 kkkk kkkk kkkk ok
*/

let isCall = 1 in
  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP, R31R30] in
  {
    def ICALL : FBR<0b1001010100001001,
                    (outs),
                    (ins variable_ops),
                    "icall",
                    []>;
  }

/*
def EICALL          1001 0101 0001 1001 ok
*/

let isCall = 1 in
  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP] in
  {
    def CALLk : F32BRk<0b111,
                       (outs),
      //:TODO: the imm field can be either 16 or 22 bits in devices with more
      // than 64k of ROM, fix it once we support the largests devices.
                       (ins i16imm:$dst),
                       "call\t$dst",
                       [(AVRcall imm:$dst)]>;
  }

let isTerminator = 1,
isReturn = 1,
isBarrier = 1 in
def RET : FBR<0b1001010100001000,
              (outs),
              (ins),
              "ret",
              [(AVRretflag)]>;

let isTerminator = 1,
isReturn = 1,
isBarrier = 1 in
def RETI : FBR<0b1001010100011000,
               (outs),
               (ins),
               "reti",
               [(AVRretiflag)]>;
/*
def CPSErr          0001 00rd dddd rrrr ok
*/
let Defs = [SREG] in
def CPRdRr : Pseudo<(outs),
                    (ins GPR8:$src, GPR8:$src2),
                    "cp\t$src, $src2",
                    [(AVRcmp GPR8:$src, GPR8:$src2), (implicit SREG)]>;

let Defs = [SREG] in
def CPWRdRr : Pseudo<(outs),
                     (ins DREGS:$src, DREGS:$src2),
                     "cpw\t$src, $src2",
                     [(AVRcmp DREGS:$src, DREGS:$src2), (implicit SREG)]>;

let Defs = [SREG],
Uses = [SREG] in
def CPCRdRr : Pseudo<(outs),
                     (ins GPR8:$src, GPR8:$src2),
                     "cpc\t$src, $src2",
                     [(AVRcmpc GPR8:$src, GPR8:$src2), (implicit SREG)]>;

let Defs = [SREG],
Uses = [SREG] in
def CPCWRdRr : Pseudo<(outs),
                      (ins DREGS:$src, DREGS:$src2),
                      "cpcw\t$src, $src2",
                      [(AVRcmpc DREGS:$src, DREGS:$src2), (implicit SREG)]>;

/*
def CPIri           0011 kkkk dddd kkkk ok

def SBRCrb          1111 110r rrrr 0bbb
def SBRSrb          1111 111r rrrr 0bbb
def SBICab          1001 1001 AAAA Abbb
def SBISab          1001 1011 AAAA Abbb

def BRBSsi          1111 00kk kkkk ksss ok
def BRBCsi          1111 01kk kkkk ksss ok
*/

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BREQk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "breq\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_EQ)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRNEk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brne\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_NE)]>;

/*
def BRCSi           1111 00kk kkkk k000 ok  (SAME AS BRLO)
def BRCCi           1111 01kk kkkk k000 ok  (SAME AS BRSH)
*/
let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRSHk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brsh\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_SH)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRLOk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brlo\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_LO)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRMIk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brmi\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_MI)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRPLk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brpl\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_PL)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRGEk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brge\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_GE)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRLTk : Pseudo<(outs),
                   (ins brtarget:$target),
                   "brlt\t$target",
                   [(AVRbrcond bb:$target, AVR_COND_LT)]>;
/*
def BRHSi           1111 00kk kkkk k101 ok
def BRHCi           1111 01kk kkkk k101 ok
def BRTSi           1111 00kk kkkk k110 ok
def BRTCi           1111 01kk kkkk k110 ok
def BRVSi           1111 00kk kkkk k011 ok
def BRVCi           1111 01kk kkkk k011 ok
def BRIEi           1111 00kk kkkk k111 ok
def BRIDi           1111 01kk kkkk k111 ok
*/
// data transfer instructions

let neverHasSideEffects = 1 in
def MOVRdRr : FRdRr<0b0010,
                    0b11,
                    (outs GPR8:$dst),
                    (ins GPR8:$src),
                    "mov\t$dst, $src",
                    []>;

let neverHasSideEffects = 1 in
def MOVWRdRr : FMOVWRdRr<(outs DREGS:$dst),
                         (ins DREGS:$src),
                         "movw\t$dst, $src",
                         []>;

let isReMaterializable = 1,
isAsCheapAsAMove = 1 in
def LDIRdK : FRdK<0b1110,
                  (outs LD8:$dst),
                  (ins i8imm:$src),
                  "ldi\t$dst, $src",
                  [(set LD8:$dst, imm:$src)]>;

let isReMaterializable = 1,
isAsCheapAsAMove = 1 in
def LDIWRdK : Pseudo<(outs DLDREGS:$dst),
                     (ins i16imm:$src),
                     "ldiw\t$dst, $src",
                     [(set DLDREGS:$dst, imm:$src)]>;

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def LDSRdK : F32DM<0b0,
                   (outs GPR8:$dst),
                   (ins i16imm:$src),
                   "lds\t$dst, $src",
                   [(set GPR8:$dst, (load imm:$src))]>;

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def LDSWRdK : Pseudo<(outs DREGS:$dst),
                     (ins i16imm:$src),
                     "ldsw\t$dst, $src",
                     [(set DREGS:$dst, (load imm:$src))]>;

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def LDRdPtr : Pseudo<(outs GPR8:$dst),
                     (ins PTRREGS:$src),
                     "ld\t$dst, $src",
                     [(set GPR8:$dst, (load PTRREGS:$src))]>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
Constraints = "@earlyclobber $dst" in
def LDWRdPtr : Pseudo<(outs DREGS:$dst),
                      (ins PTRDISPREGS:$src),
                      "ldw\t$dst, $src",
                      [(set DREGS:$dst, (load PTRDISPREGS:$src))]>;

let mayLoad = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LDRdPtrPi : Pseudo<(outs GPR8:$dst, PTRREGS:$base_wb),
                       (ins PTRREGS:$src),
                       "ld\t$dst, $src+",
                       []>;

let mayLoad = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LDWRdPtrPi : Pseudo<(outs DREGS:$dst, PTRREGS:$base_wb),
                        (ins PTRREGS:$src),
                        "ldw\t$dst, $src+",
                        []>;

let mayLoad = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LDRdPtrPd : Pseudo<(outs GPR8:$dst, PTRREGS:$base_wb),
                       (ins PTRREGS:$src),
                       "ld\t$dst, -$src",
                       []>;

let mayLoad = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LDWRdPtrPd : Pseudo<(outs DREGS:$dst, PTRREGS:$base_wb),
                        (ins PTRREGS:$src),
                        "ldw\t$dst, -$src",
                        []>;

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def LDDRdPtrQ : Pseudo<(outs GPR8:$dst),
                       (ins memri:$src),
                       "ldd\t$dst, $src",
                       [(set GPR8:$dst, (load addr:$src))]>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
Constraints = "@earlyclobber $dst" in
def LDDWRdPtrQ : Pseudo<(outs DREGS:$dst),
                        (ins memri:$src),
                        "lddw\t$dst, $src",
                        [(set DREGS:$dst, (load addr:$src))]>;

//:FIXME: remove this once PR13375 gets fixed
let canFoldAsLoad = 1,
isReMaterializable = 1,
mayLoad = 1,
neverHasSideEffects = 1 in
def LDDWRdYQ : Pseudo<(outs DREGS:$dst),
                      (ins memri:$src),
                      "lddw\t$dst, $src",
                      []>;

def STSKRr : Pseudo<(outs),
                    (ins i16imm:$dst, GPR8:$src),
                    "sts\t$dst, $src",
                    [(store GPR8:$src, imm:$dst)]>;

def STSWKRr : Pseudo<(outs),
                     (ins i16imm:$dst, DREGS:$src),
                     "stsw\t$dst, $src",
                     [(store DREGS:$src, imm:$dst)]>;

def STPtrRr : Pseudo<(outs),
                     (ins PTRREGS:$dst, GPR8:$src),
                     "st\t$dst, $src",
                     [(store GPR8:$src, PTRREGS:$dst)]>;

def STWPtrRr : Pseudo<(outs),
                      (ins PTRDISPREGS:$dst, DREGS:$src),
                      "stw\t$dst, $src",
                      [(store DREGS:$src, PTRDISPREGS:$dst)]>;

let Constraints = "$dst = $base_wb,@earlyclobber $base_wb" in
def STPtrPiRr : Pseudo<(outs PTRREGS:$base_wb),
                       (ins PTRREGS:$dst, GPR8:$src, i8imm:$offs),
                       "st\t$dst+, $src",
                       [(set PTRREGS:$base_wb,
                        (post_store GPR8:$src, PTRREGS:$dst, imm:$offs))]>;

let Constraints = "$dst = $base_wb,@earlyclobber $base_wb" in
def STWPtrPiRr : Pseudo<(outs PTRREGS:$base_wb),
                        (ins PTRREGS:$dst, DREGS:$src, i8imm:$offs),
                        "stw\t$dst+, $src",
                        [(set PTRREGS:$base_wb,
                         (post_store DREGS:$src, PTRREGS:$dst, imm:$offs))]>;

let Constraints = "$dst = $base_wb,@earlyclobber $base_wb" in
def STPtrPdRr : Pseudo<(outs PTRREGS:$base_wb),
                       (ins PTRREGS:$dst, GPR8:$src, i8imm:$offs),
                       "st\t-$dst, $src",
                       [(set PTRREGS:$base_wb,
                        (pre_store GPR8:$src, PTRREGS:$dst, imm:$offs))]>;

let Constraints = "$dst = $base_wb,@earlyclobber $base_wb" in
def STWPtrPdRr : Pseudo<(outs PTRREGS:$base_wb),
                        (ins PTRREGS:$dst, DREGS:$src, i8imm:$offs),
                        "stw\t-$dst, $src",
                        [(set PTRREGS:$base_wb,
                         (pre_store DREGS:$src, PTRREGS:$dst, imm:$offs))]>;

def STDPtrQRr : Pseudo<(outs),
                       (ins memri:$dst, GPR8:$src),
                       "std\t$dst, $src",
                       [(store GPR8:$src, addr:$dst)]>;

def STDWPtrQRr : Pseudo<(outs),
                        (ins memri:$dst, DREGS:$src),
                        "stdw\t$dst, $src",
                        [(store DREGS:$src, addr:$dst)]>;

/*
def LPM             1001 0101 1100 1000     //(R0)<-(Z)
*/

let canFoldAsLoad = 1,
isReMaterializable = 1,
neverHasSideEffects = 1 in
def LPMRdZ : Pseudo<(outs GPR8:$dst),
                    (ins),
                    "lpm\t$dst, Z",
                    []>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LPMWRdZ : Pseudo<(outs DREGS:$dst, DREGS:$base_wb),
                     (ins DREGS:$src),
                     "lpmw\t$dst, Z",
                     []>;

let mayLoad = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LPMRdZPi : Pseudo<(outs GPR8:$dst, DREGS:$base_wb),
                      (ins DREGS:$src),
                      "lpm\t$dst, Z+",
                      []>;

let mayLoad = 1,
neverHasSideEffects = 1,
Constraints = "$src = $base_wb,@earlyclobber $dst,@earlyclobber $base_wb" in
def LPMWRdZPi : Pseudo<(outs DREGS:$dst, DREGS:$base_wb),
                       (ins DREGS:$src),
                       "lpmw\t$dst, Z+",
                       []>;

/*
def ELPM            1001 0101 1101 1000
def ELPMrZ          1001 000d dddd 0110
def ELPMrZpi        1001 000d dddd 0111

def SPM             //:TODO:
def SPMZpi
*/

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def INRdA : FIORdA<(outs GPR8:$dst),
                   (ins i16imm:$src),
                   "in\t$dst, $src",
                   [(set GPR8:$dst, (load ioaddr8:$src))]>;

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def INWRdA : Pseudo<(outs DREGS:$dst),
                    (ins i16imm:$src),
                    "inw\t$dst, $src",
                    [(set DREGS:$dst, (load ioaddr16:$src))]>;

def OUTARr : FIOARr<(outs),
                    (ins i16imm:$dst, GPR8:$src),
                    "out\t$dst, $src",
                    [(store GPR8:$src, ioaddr8:$dst)]>;

def OUTWARr : Pseudo<(outs),
                     (ins i16imm:$dst, DREGS:$src),
                     "outw\t$dst, $src",
                     [(store DREGS:$src, ioaddr16:$dst)]>;

let Defs = [SP],
Uses = [SP],
neverHasSideEffects = 1,
mayStore = 1 in
def PUSHRr : FRd<0b1001,
                 0b0011111,
                 (outs),
                 (ins GPR8:$reg),
                 "push\t$reg",
                 []>;

let Defs = [SP],
Uses = [SP],
neverHasSideEffects = 1,
mayStore = 1 in
def PUSHWRr : Pseudo<(outs),
                     (ins DREGS:$reg),
                     "pushw\t$reg",
                     []>;

let Defs = [SP],
Uses = [SP],
neverHasSideEffects = 1,
mayLoad = 1 in
def POPRd : FRd<0b1001,
                0b0001111,
                (outs GPR8:$reg),
                (ins),
                "pop\t$reg",
                []>;

let Defs = [SP],
Uses = [SP],
neverHasSideEffects = 1,
mayLoad = 1 in
def POPWRd : Pseudo<(outs DREGS:$reg),
                    (ins),
                    "popw\t$reg",
                    []>;

/*__________________________________________________________________________________
bit and bit-test instructions
*/

let Constraints = "$src = $dst",
Defs = [SREG] in
def LSLRd : FRdRr<0b0000,
                  0b11,
                  (outs GPR8:$dst),
                  (ins GPR8:$src),
                  "lsl\t$dst",
                  [(set GPR8:$dst, (AVRlsl GPR8:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def LSLWRd : Pseudo<(outs DREGS:$dst),
                    (ins DREGS:$src),
                    "lslw\t$dst",
                    [(set DREGS:$dst, (AVRlsl DREGS:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def LSRRd : FRd<0b1001,
                0b0100110,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "lsr\t$dst",
                [(set GPR8:$dst, (AVRlsr GPR8:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def LSRWRd : Pseudo<(outs DREGS:$dst),
                    (ins DREGS:$src),
                    "lsrw\t$dst",
                    [(set DREGS:$dst, (AVRlsr DREGS:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG],
Uses = [SREG] in
def ROLRd : FRdRr<0b0001,
                  0b11,
                  (outs GPR8:$dst),
                  (ins GPR8:$src),
                  "rol\t$dst",
                  [(set GPR8:$dst, (AVRrol GPR8:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG],
Uses = [SREG] in
def ROLWRd : Pseudo<(outs DREGS:$dst),
                    (ins DREGS:$src),
                    "rolw\t$dst",
                    [(set DREGS:$dst, (AVRrol DREGS:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG],
Uses = [SREG] in
def RORRd : FRd<0b1001,
                0b0100111,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "ror\t$dst",
                [(set GPR8:$dst, (AVRror GPR8:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG],
Uses = [SREG] in
def RORWRd : Pseudo<(outs DREGS:$dst),
                    (ins DREGS:$src),
                    "rorw\t$dst",
                    [(set DREGS:$dst, (AVRror DREGS:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ASRRd : FRd<0b1001,
                0b0100101,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "asr\t$dst",
                [(set GPR8:$dst, (AVRasr GPR8:$src)), (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ASRWRd : Pseudo<(outs DREGS:$dst),
                    (ins DREGS:$src),
                    "asrw\t$dst",
                    [(set DREGS:$dst, (AVRasr DREGS:$src)), (implicit SREG)]>;

//:TODO: add 16 bit and wider version of this
let Constraints = "$src = $dst" in
def SWAPRd : FRd<0b1001,
                 0b0100010,
                 (outs GPR8:$dst),
                 (ins GPR8:$src),
                 "swap\t$dst",
                 [(set GPR8:$dst, (bswap GPR8:$src))]>;

//:TODO: add 16 bit versions of cbi/sbi
//:TODO: add patterns when popcount(imm)==2 to be expanded with 2 sbi/cbi
// instead of in+ori+out which requires one more instr.
def SBIAb : Pseudo<(outs),
                   (ins i16imm:$addr, i8imm:$bit),
                   "sbi\t$addr, $bit",
                   [(store (or (i8 (load lowioaddr8:$addr)), iobitpos8:$bit),
                     lowioaddr8:$addr)]>;

def CBIAb : Pseudo<(outs),
                   (ins i16imm:$addr, i8imm:$bit),
                   "cbi\t$addr, $bit",
                   [(store (and (i8 (load lowioaddr8:$addr)), iobitposn8:$bit),
                     lowioaddr8:$addr)]>;

/*
def BSTrb           1111 101d dddd 0bbb
def BLDrb           1111 100d dddd 0bbb

def BSETs           1001 0100 0sss 1000
def BCLRs           1001 0100 1sss 1000 // probably all the todo below comes from here
*/
//:TODO: all SREG instr manipulation (SEC, CLC, SEN, etc...)
/*
  SEC
  CLC
  SEN
  CLN
  SEZ
  CLZ
 */

def SEI : FI<0b1001010001111000,
               (outs),
               (ins),
               "cli",
               []>;

def CLI : FI<0b1001010011111000,
               (outs),
               (ins),
               "cli",
               []>;

/*
  SES
  CLS
  SEV
  CLV
  SET
  CLT
  SEH
  CLH
 */

// MCU control instructions
/*
def NOP             0000 0000 0000 0000
*/

//===----------------------------------------------------------------------===//
// Pseudo instructions for later expansion
//===----------------------------------------------------------------------===//

//:TODO: optimize this for wider types AND optimize the following code
//   compile int foo(char a, char b, char c, char d) {return d+b;}
//   looks like a missed sext_inreg opportunity
let Defs = [SREG] in
def SEXT : Pseudo<(outs DREGS:$dst),
                  (ins GPR8:$src),
                  "sext\t$dst, $src",
                  [(set DREGS:$dst, (sext GPR8:$src)), (implicit SREG)]>;

let Defs = [SREG] in
def ZEXT : Pseudo<(outs DREGS:$dst),
                  (ins GPR8:$src),
                  "zext\t$dst, $src",
                  [(set DREGS:$dst, (zext GPR8:$src)), (implicit SREG)]>;

// This pseudo gets expanded into a movw+adiw thus it clobbers SREG.
let Defs = [SREG],
neverHasSideEffects = 1 in
def FRMIDX : Pseudo<(outs DLDREGS:$dst),
                    (ins DLDREGS:$src, i16imm:$src2),
                    "frmidx\t$dst, $src, $src2",
                    []>;

// This pseudo is either converted to a regular store or a push which clobbers
// SP.
let Defs = [SP] in
def STDSPQRr : Pseudo<(outs),
                      (ins memspi:$dst, GPR8:$src),
                      "stdstk\t$dst, $src",
                      [(store GPR8:$src, addr:$dst)]>;

// This pseudo is either converted to a regular store or a push which clobbers
// SP.
let Defs = [SP] in
def STDWSPQRr : Pseudo<(outs),
                       (ins memspi:$dst, DREGS:$src),
                       "stdwstk\t$dst, $src",
                       [(store DREGS:$src, addr:$dst)]>;

let Uses = [SP],
neverHasSideEffects = 1 in
def SPREAD : Pseudo<(outs DREGS:$dst),
                    (ins GPRSP:$src),
                    "spread\t$dst, $src",
                    []>;

let Defs = [SP],
neverHasSideEffects = 1 in
def SPWRITE : Pseudo<(outs GPRSP:$dst),
                     (ins DREGS:$src),
                     "spwrite\t$dst, $src",
                     []>;

let usesCustomInserter = 1 in
{
  let Uses = [SREG] in
  def Select8 : Pseudo<(outs GPR8:$dst),
                       (ins GPR8:$src, GPR8:$src2, i8imm:$cc),
                       "# Select8 PSEUDO",
                       [(set GPR8:$dst,
                        (AVRselectcc GPR8:$src, GPR8:$src2, imm:$cc))]>;
  let Uses = [SREG] in
  def Select16 : Pseudo<(outs DREGS:$dst),
                        (ins DREGS:$src, DREGS:$src2, i8imm:$cc),
                        "# Select16 PSEUDO",
                        [(set DREGS:$dst,
                         (AVRselectcc DREGS:$src, DREGS:$src2, imm:$cc))]>;

  let Defs = [SREG] in
  def Lsl8 : Pseudo<(outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$cnt),
                    "# Lsl8 PSEUDO",
                    [(set GPR8:$dst, (AVRlslLoop GPR8:$src, GPR8:$cnt))]>;
  let Defs = [SREG] in
  def Lsl16 : Pseudo<(outs DREGS:$dst),
                     (ins DREGS:$src, GPR8:$cnt),
                     "# Lsl16 PSEUDO",
                     [(set DREGS:$dst, (AVRlslLoop DREGS:$src, GPR8:$cnt))]>;

  let Defs = [SREG] in
  def Lsr8 : Pseudo<(outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$cnt),
                    "# Lsr8 PSEUDO",
                    [(set GPR8:$dst, (AVRlsrLoop GPR8:$src, GPR8:$cnt))]>;
  let Defs = [SREG] in
  def Lsr16 : Pseudo<(outs DREGS:$dst),
                     (ins DREGS:$src, GPR8:$cnt),
                     "# Lsr16 PSEUDO",
                     [(set DREGS:$dst, (AVRlsrLoop DREGS:$src, GPR8:$cnt))]>;

  let Defs = [SREG] in
  def Asr8 : Pseudo<(outs GPR8:$dst),
                    (ins GPR8:$src, GPR8:$cnt),
                    "# Asr8 PSEUDO",
                    [(set GPR8:$dst, (AVRasrLoop GPR8:$src, GPR8:$cnt))]>;
  let Defs = [SREG] in
  def Asr16 : Pseudo<(outs DREGS:$dst),
                     (ins DREGS:$src, GPR8:$cnt),
                     "# Asr16 PSEUDO",
                     [(set DREGS:$dst, (AVRasrLoop DREGS:$src, GPR8:$cnt))]>;
}

//===----------------------------------------------------------------------===//
// Non-Instruction Patterns
//===----------------------------------------------------------------------===//

//:TODO: look in x86InstrCompiler.td for odd encoding trick related to
// add x, 128 -> sub x, -128. Clang is emitting an eor for this (ldi+eor)

// the add instruction always writes the carry flag
def : Pat<(addc GPR8:$src, GPR8:$src2),
          (ADDRdRr GPR8:$src, GPR8:$src2)>;
def : Pat<(addc DREGS:$src, DREGS:$src2),
          (ADDWRdRr DREGS:$src, DREGS:$src2)>;

// all sub instruction variants always writes the carry flag
def : Pat<(subc GPR8:$src, GPR8:$src2),
          (SUBRdRr GPR8:$src, GPR8:$src2)>;
def : Pat<(subc DREGS:$src, DREGS:$src2),
          (SUBWRdRr DREGS:$src, DREGS:$src2)>;
def : Pat<(subc LD8:$src, imm:$src2),
          (SUBIRdK LD8:$src, imm:$src2)>;
def : Pat<(subc DLDREGS:$src, imm:$src2),
          (SUBIWRdK DLDREGS:$src, imm:$src2)>;

// these patterns convert add (x, -imm) to sub (x, imm) since we dont have
// any add with imm instructions. Also take care of the adiw/sbiw instructions
//:TODO: Use adiw/sbiw in 32/64 ops if the lower part is < 64 like
// int32var + 3 -> adiw R25:R24, 3; adc R26, 0; adc r27, 0
def : Pat<(add IWREGS:$src1, imm0_63_neg:$src2),
          (SBIWRdK IWREGS:$src1, (imm0_63_neg:$src2))>;
def : Pat<(add DLDREGS:$src1, imm:$src2),
          (SUBIWRdK DLDREGS:$src1, (imm16_neg_XFORM imm:$src2))>;
def : Pat<(addc DLDREGS:$src1, imm:$src2),
          (SUBIWRdK DLDREGS:$src1, (imm16_neg_XFORM imm:$src2))>;
def : Pat<(adde DLDREGS:$src1, imm:$src2),
          (SBCIWRdK DLDREGS:$src1, (imm16_neg_XFORM imm:$src2))>;

def : Pat<(add LD8:$src1, imm:$src2),
          (SUBIRdK LD8:$src1, (imm8_neg_XFORM imm:$src2))>;
def : Pat<(addc LD8:$src1, imm:$src2),
          (SUBIRdK LD8:$src1, (imm8_neg_XFORM imm:$src2))>;
def : Pat<(adde LD8:$src1, imm:$src2),
          (SBCIRdK LD8:$src1, (imm8_neg_XFORM imm:$src2))>;

// calls
def : Pat<(AVRcall (i16 tglobaladdr:$dst)),
          (CALLk tglobaladdr:$dst)>;
def : Pat<(AVRcall (i16 texternalsym:$dst)),
          (CALLk texternalsym:$dst)>;

// anyext
def : Pat<(i16 (anyext GPR8:$src)),
          (INSERT_SUBREG (i16 (IMPLICIT_DEF)), GPR8:$src, sub_lo)>;

// trunc
def : Pat<(i8 (trunc DREGS:$src)),
          (EXTRACT_SUBREG DREGS:$src, sub_lo)>;

// sext_inreg
def : Pat<(sext_inreg DREGS:$src, i8),
          (SEXT (i8 (EXTRACT_SUBREG DREGS:$src, sub_lo)))>;

// GlobalAddress
def : Pat<(i16 (AVRWrapper tglobaladdr:$dst)),
          (LDIWRdK tglobaladdr:$dst)>;
def : Pat<(add DREGS:$src, (AVRWrapper tglobaladdr:$src2)),
          (SUBIWRdK DREGS:$src, tglobaladdr:$src2)>;
def : Pat<(i8 (load (AVRWrapper tglobaladdr:$dst))),
          (LDSRdK tglobaladdr:$dst)>;
def : Pat<(i16 (load (AVRWrapper tglobaladdr:$dst))),
          (LDSWRdK tglobaladdr:$dst)>;
def : Pat<(store GPR8:$src, (i16 (AVRWrapper tglobaladdr:$dst))),
          (STSKRr tglobaladdr:$dst, GPR8:$src)>;
def : Pat<(store DREGS:$src, (i16 (AVRWrapper tglobaladdr:$dst))),
          (STSWKRr tglobaladdr:$dst, DREGS:$src)>;

// BlockAddress
def : Pat<(i16 (AVRWrapper tblockaddress:$dst)),
          (LDIWRdK tblockaddress:$dst)>;

// hi-reg truncation : trunc(int16 >> 8)
//:FIXME: i think it's better to emit an extract subreg node in the DAG that
// all this mess once we get optimal shift code
def : Pat<(i8 (trunc (AVRlsr (AVRlsr (AVRlsr (AVRlsr (AVRlsr (AVRlsr (AVRlsr
                     (AVRlsr DREGS:$src)))))))))),
          (EXTRACT_SUBREG DREGS:$src, sub_hi)>;

// :FIXME: DAGCombiner produces an shl node after legalization from these seq:
// BR_JT -> (mul x, 2) -> (shl x, 1)
def : Pat<(shl DREGS:$src1, (i8 1)),
          (LSLWRd DREGS:$src1)>;
