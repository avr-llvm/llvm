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

def AVRMemAsmOperand : AsmOperandClass {
  let Name = "Mem";
  let ParserMethod = "parseMemOperand";
}

// Address operand for reg+imm
def memri : Operand<iPTR>
{
  let PrintMethod = "printMemriOperand";
  let EncoderMethod = "getMemriEncoding";
  let ParserMatchClass = AVRMemAsmOperand;
  
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

def i8imm_com : Operand<i8>
{
  let EncoderMethod = "getI8ImmComEncoding";

  let MIOperandInfo = (ops i8imm);
}

def rel_condbr_7_target : Operand<OtherVT>
{
    let PrintMethod = "print_pcrel_imm";
    let EncoderMethod = "getRelCondBr7TargetEncoding";
}

def rel_condbr_13_target : Operand<OtherVT>
{
    let PrintMethod = "print_pcrel_imm";
    let EncoderMethod = "getRelCondBr13TargetEncoding";
}

// Addressing mode pattern reg+imm6
def addr : ComplexPattern<iPTR, 2, "SelectAddr", [], [SDNPWantRoot]>;

// AsmOperand class for a pointer register.
// Used with the LD/ST family of instructions.
// See FSTLD in AVRInstrFormats.td
def PtrRegAsmOperand : AsmOperandClass
{
   let Name = "Reg";
} 
 
// A special operand type for the LD/ST instructions.
//
// The 12th bit in the instruction follows no pattern. It is 1 in some
// cases of pointer reg and predec/postinc mode, and 0 in others.
// We enumerate all cases in the custom encoder method and set this
// bit appropriately.
//
// See FSTLD in AVRInstrFormats.td for more information.
def LDSTPtrReg : Operand<i16>
{
    let MIOperandInfo = (ops PTRREGS);
    let EncoderMethod = "getLDSTPtrRegEncoding";
    
    let ParserMatchClass = PtrRegAsmOperand;
}

//===----------------------------------------------------------------------===//
// AVR predicates for subtarget features
//===----------------------------------------------------------------------===//

def HasLPM       :     Predicate<"Subtarget->hasLPM()">,
                         AssemblerPredicate<"FeatureILPM">;

def HasLPMX      :     Predicate<"Subtarget->hasLPMX()">,
                         AssemblerPredicate<"FeatureILPMX">;

def HasELPM      :     Predicate<"Subtarget->hasELPM()">,
                         AssemblerPredicate<"FeatureIELPM">;

def HasELPMX     :     Predicate<"Subtarget->hasELPMX()">,
                         AssemblerPredicate<"FeatureIELPMX">;

def HasMUL       :     Predicate<"Subtarget->hasMUL()">,
                         AssemblerPredicate<"FeatureIMUL">;

def HasFMUL      :     Predicate<"Subtarget->hasFMUL()">,
                         AssemblerPredicate<"FeatureIFMUL">;

def HasMOVW      :    Predicate<"Subtarget->hasMOVW()">,
                         AssemblerPredicate<"FeatureIMOVW">;

def HasDES       :    Predicate<"Subtarget->hasDES()">,
                         AssemblerPredicate<"FeatureIDES">;

def HasBREAK     :    Predicate<"Subtarget->hasBREAK()">,
                         AssemblerPredicate<"FeatureIBREAK">;

def SupportsRMW  :    Predicate<"Subtarget->supportsRMW()">,
                         AssemblerPredicate<"FeatureRMW">;

def HasSRAM      :    Predicate<"Subtarget->hasSRAM()">,
                         AssemblerPredicate<"FeatureSRAM">;

def HasEIND      :    Predicate<"Subtarget->hasEIND()">,
                         AssemblerPredicate<"FeatureEIND">;

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

// Arithmetic instructions

// ADD Rd, Rr
// Adds two 8-bit registers.
let isCommutable = 1,
Constraints = "$src = $rd",
Defs = [SREG] in
def ADDRdRr : FRdRr<0b0000,
                    0b11,
                    (outs GPR8:$rd),
                    (ins GPR8:$src, GPR8:$rr),
                    "add\t$rd, $rr",
                    [(set GPR8:$rd, (add GPR8:$src, GPR8:$rr)),
                     (implicit SREG)]>;

// ADDW Rd, Rr
// Pseudo instruction to add four 8-bit registers as two 16-bit values.
let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ADDWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "addw\t$dst, $src2",
                      [(set DREGS:$dst, (add DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

// ADC Rd, Rr
// Adds two 8-bit registers with carry.
let isCommutable = 1,
Constraints = "$src = $rd",
Uses = [SREG],
Defs = [SREG] in
def ADCRdRr : FRdRr<0b0001,
                    0b11,
                    (outs GPR8:$rd),
                    (ins GPR8:$src, GPR8:$rr),
                    "adc\t$rd, $rr",
                    [(set GPR8:$rd, (adde GPR8:$src, GPR8:$rr)),
                     (implicit SREG)]>;

// ADCW Rd, Rr
// Pseudo instruction to add four 8-bit registers as two 16-bit values with carry.
let isCommutable = 1,
Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def ADCWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "adcw\t$dst, $src2",
                      [(set DREGS:$dst, (adde DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

// AIDW Rd, k
// Adds an immediate 6-bit value K to Rd, placing the result in Rd.
let Constraints = "$src = $dst",
Defs = [SREG] in
def ADIWRdK : FWRdK<0b0,
                    (outs IWREGS:$dst),
                    (ins IWREGS:$src, i16imm:$k),
                    "adiw\t$dst, $k",
                    [(set IWREGS:$dst, (add IWREGS:$src, uimm6:$k)),
                     (implicit SREG)]>;

// SUB Rd, Rr
// Subtracts the 8-bit value of Rr from Rd and places the value in Rd.
let Constraints = "$src = $rd",
Defs = [SREG] in
def SUBRdRr : FRdRr<0b0001,
                    0b10,
                    (outs GPR8:$rd),
                    (ins GPR8:$src, GPR8:$rr),
                    "sub\t$rd, $rr",
                    [(set GPR8:$rd, (sub GPR8:$src, GPR8:$rr)),
                     (implicit SREG)]>;

// SUBW Rd, Rr
// Subtracts two 16-bit values and places the result into Rd.
let Constraints = "$src = $dst",
Defs = [SREG] in
def SUBWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "subw\t$dst, $src2",
                      [(set DREGS:$dst, (sub DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $rd",
Defs = [SREG] in
def SUBIRdK : FRdK<0b0101,
                   (outs LD8:$rd),
                   (ins LD8:$src, i8imm:$k),
                   "subi\t$rd, $k",
                   [(set LD8:$rd, (sub LD8:$src, imm:$k)),
                    (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def SUBIWRdK : Pseudo<(outs DLDREGS:$dst),
                      (ins DLDREGS:$src, i16imm:$src2),
                      "subiw\t$dst, $src2",
                      [(set DLDREGS:$dst, (sub DLDREGS:$src, imm:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $rd",
Uses = [SREG],
Defs = [SREG] in
def SBCRdRr : FRdRr<0b0000,
                    0b10,
                    (outs GPR8:$rd),
                    (ins GPR8:$src, GPR8:$rr),
                    "sbc\t$rd, $rr",
                    [(set GPR8:$rd, (sube GPR8:$src, GPR8:$rr)),
                     (implicit SREG)]>;

let Constraints = "$src = $dst",
Uses = [SREG],
Defs = [SREG] in
def SBCWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "sbcw\t$dst, $src2",
                      [(set DREGS:$dst, (sube DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $rd",
Uses = [SREG],
Defs = [SREG] in
def SBCIRdK : FRdK<0b0100,
                   (outs LD8:$rd),
                   (ins LD8:$src, i8imm:$k),
                   "sbci\t$rd, $k",
                   [(set LD8:$rd, (sube LD8:$src, imm:$k)),
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
                    (ins IWREGS:$src, i16imm:$k),
                    "sbiw\t$dst, $k",
                    [(set IWREGS:$dst, (sub IWREGS:$src, uimm6:$k)),
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

// MUL Rd, Rr
// Multiplies Rd by Rr and places the result into Rd.
let isCommutable = 1,
Defs = [R1, R0, SREG],
hasSideEffects = 0 in
def MULRdRr : Mu1RdRr<(outs GPR8:$dst),
                      (ins GPR8:$src, GPR8:$src2),
                      "mul\t$src, $src2",
                      []>,
              Requires<[HasMUL]>;

// The earlyclobber flag is required because the pseudo expansion needs $dst
// and $src to have different registers.
let isCommutable = 1,
Constraints = "@earlyclobber $dst",
Defs = [R1, R0, SREG] in
def MULWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "mulw\t$dst, $src, $src2",
                      [(set DREGS:$dst, (mul DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>,
               Requires<[HasMUL]>;

// This instruction produces a 16bit result from two 8bit operands.
let isCommutable = 1,
Defs = [R1R0, SREG],
hasSideEffects = 0 in
def MUL16RdRr : Mu1RdRr<(outs DREGS:$dst),
                        (ins GPR8:$src, GPR8:$src2),
                        "mul\t$src, $src2",
                        []>,
                Requires<[HasMUL]>;

let isCommutable = 1,
Defs = [R1, R0, SREG] in
def MULSRdRr : Mu2RdRr<0,
                       (outs GPR8:$dst),
                       (ins GPR8:$rd, GPR8:$rr),
                       "muls\t$rd, $rr",
                       []>;

let isCommutable = 1,
Defs = [R1, R0, SREG] in
def MULSURdRr : Mu2RdRr<1,
                        (outs GPR8:$dst),
                        (ins GPR8:$rd, GPR8:$rr),
                        "mulsu\t$rd, $rr",
                        []>;

let isCommutable = 1,
Defs = [R1, R0, SREG] in
def FMUL : Mu3RdRr<0b01,
                   (outs GPR8:$dst),
                   (ins GPR8:$rd, GPR8:$rr),
                   "fmul\t$rd, $rr",
                   []>;

let isCommutable = 1,
Defs = [R1, R0, SREG] in
def FMULS : Mu3RdRr<0b10,
                    (outs GPR8:$dst),
                    (ins GPR8:$rd, GPR8:$rr),
                    "fmuls\t$rd, $rr",
                    []>;

let isCommutable = 1,
Defs = [R1, R0, SREG] in
def FMULSU : Mu3RdRr<0b11,
                     (outs GPR8:$dst),
                     (ins GPR8:$rd, GPR8:$rr),
                     "fmulsu\t$rd, $rr",
                     []>;
let Defs = [R15, R14, R13, R12, R11, R10, R9,
            R8, R7, R6, R5, R4, R3, R2, R1, R0] in
def DESK : FDES<(outs),
                (ins i8imm:$k),
                "des\t$k",
                []>;

//_______________________________________
// Logic Instructions
//_______________________________________

/// AND Rd, Rr
let isCommutable = 1,
Constraints = "$src = $rd",
Defs = [SREG] in
def ANDRdRr : FRdRr<0b0010,
                    0b00,
                    (outs GPR8:$rd),
                    (ins GPR8:$src, GPR8:$rr),
                    "and\t$rd, $rr",
                    [(set GPR8:$rd, (and GPR8:$src, GPR8:$rr)),
                     (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ANDWRdRr : Pseudo<(outs DREGS:$dst),
                      (ins DREGS:$src, DREGS:$src2),
                      "andw\t$dst, $src2",
                      [(set DREGS:$dst, (and DREGS:$src, DREGS:$src2)),
                       (implicit SREG)]>;

let Constraints = "$src = $rd",
Defs = [SREG] in
def ANDIRdK : FRdK<0b0111,
                   (outs LD8:$rd),
                   (ins LD8:$src, i8imm:$k),
                   "andi\t$rd, $k",
                   [(set LD8:$rd, (and LD8:$src, imm:$k)),
                    (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ANDIWRdK : Pseudo<(outs DLDREGS:$dst),
                      (ins DLDREGS:$src, i16imm:$src2),
                      "andiw\t$dst, $src2",
                      [(set DLDREGS:$dst, (and DLDREGS:$src, imm:$src2)),
                       (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $rd",
Defs = [SREG] in
def ORRdRr : FRdRr<0b0010,
                   0b10,
                   (outs GPR8:$rd),
                   (ins GPR8:$src, GPR8:$rr),
                   "or\t$rd, $rr",
                   [(set GPR8:$rd, (or GPR8:$src, GPR8:$rr)),
                    (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $dst",
Defs = [SREG] in
def ORWRdRr : Pseudo<(outs DREGS:$dst),
                     (ins DREGS:$src, DREGS:$src2),
                     "orw\t$dst, $src2",
                     [(set DREGS:$dst, (or DREGS:$src, DREGS:$src2)),
                      (implicit SREG)]>;

let Constraints = "$src = $rd",
Defs = [SREG] in
def ORIRdK : FRdK<0b0110,
                  (outs LD8:$rd),
                  (ins LD8:$src, i8imm:$k),
                  "ori\t$rd, $k",
                  [(set LD8:$rd, (or LD8:$src, imm:$k)),
                   (implicit SREG)]>;

let Constraints = "$src = $dst",
Defs = [SREG] in
def ORIWRdK : Pseudo<(outs DLDREGS:$dst),
                     (ins DLDREGS:$src, i16imm:$src2),
                     "oriw\t$dst, $src2",
                     [(set DLDREGS:$dst, (or DLDREGS:$src, imm:$src2)),
                      (implicit SREG)]>;

let isCommutable = 1,
Constraints = "$src = $rd",
Defs = [SREG] in
def EORRdRr : FRdRr<0b0010,
                    0b01,
                    (outs GPR8:$rd),
                    (ins GPR8:$src, GPR8:$rr),
                    "eor\t$rd, $rr",
                    [(set GPR8:$rd, (xor GPR8:$src, GPR8:$rr)),
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
/// TST Rd
// TODO: turn into an InstAlias
let Constraints = "$src = $dst",
Defs = [SREG] in
def TSTRd : FRdRr<0b0010,
                  0b00,
                  (outs GPR8:$dst),
                  (ins GPR8:$src, GPR8:$src2),
                  "tst\t$dst",
                  [(set GPR8:$dst, (and GPR8:$src, GPR8:$src2)),
                  (implicit SREG)]>;

// Branch instructions

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def RJMPk : FBRk<0,
                 (outs),
                 (ins rel_condbr_13_target:$target),
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

let isBranch = 1,
isTerminator = 1,
isBarrier = 1,
isIndirectBranch = 1,
Uses = [R31R30] in
def EIJMP : FBR<0b1001010000011001,
                (outs),
                (ins),
                "eijmp",
                []>;

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def JMPk : F32BRk<0b110,
                  (outs),
                  (ins brtarget:$target),
                  "jmp\t$target",
                  []>;
                  
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

let isCall = 1 in
  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP, R31R30] in
  {
    def EICALL : FBR<0b1001010000011001,
                     (outs),
                     (ins variable_ops),
                     "eicall",
                     []>;
  }

let isCall = 1 in
  // SP is marked as a use to prevent stack-pointer assignments that appear
  // immediately before calls from potentially appearing dead.
  let Uses = [SP] in
  {
    def RCALL : FBRk<1,
                     (outs),
                     (ins rel_condbr_13_target:$target),
                     "rcall\t$target",
                     []>;
  }


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
isBarrier = 1,
Uses = [SP]  in
def RET : FBR<0b1001010100001000,
              (outs),
              (ins),
              "ret",
              [(AVRretflag)]>;

let isTerminator = 1,
isReturn = 1,
isBarrier = 1,
Uses = [SP] in
def RETI : FBR<0b1001010100011000,
               (outs),
               (ins),
               "reti",
               [(AVRretiflag)]>;


let isBarrier = 1,
isBranch = 1,
isTerminator = 1,
Defs = [SREG] in
def CPSE : FRdRr<0b0001,
                 0b00,
                 (outs),
                 (ins GPR8:$rd, GPR8:$rr),
                 "cpse\t$rd, $rr",
                 []>;

let Defs = [SREG] in
def CPRdRr : FRdRr<0b0001,
                   0b01,
                   (outs),
                   (ins GPR8:$rd, GPR8:$rr),
                   "cp\t$rd, $rr",
                   [(AVRcmp GPR8:$rd, GPR8:$rr), (implicit SREG)]>;

let Defs = [SREG] in
def CPWRdRr : Pseudo<(outs),
                     (ins DREGS:$src, DREGS:$src2),
                     "cpw\t$src, $src2",
                     [(AVRcmp DREGS:$src, DREGS:$src2), (implicit SREG)]>;

let Defs = [SREG],
Uses = [SREG] in
def CPCRdRr : FRdRr<0b0000,
                    0b01,
                    (outs),
                    (ins GPR8:$rd, GPR8:$rr),
                    "cpc\t$rd, $rr",
                    [(AVRcmpc GPR8:$rd, GPR8:$rr), (implicit SREG)]>;

let Defs = [SREG],
Uses = [SREG] in
def CPCWRdRr : Pseudo<(outs),
                      (ins DREGS:$src, DREGS:$src2),
                      "cpcw\t$src, $src2",
                      [(AVRcmpc DREGS:$src, DREGS:$src2), (implicit SREG)]>;

// CPI Rd, K
// -----
// Compares a register with an 8 bit immediate.
let Defs = [SREG],
Uses = [SREG] in
def CPIRdK : FRdK<0b0011,
                  (outs LD8:$k),
                  (ins GPR8:$rd),
                  "cpi\t$rd, $k",
                  []>;

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def SBRCRrB : FRdB<0b10,
                   (outs),
                   (ins GPR8:$rr, i8imm:$b),
                   "sbrc\t$rr, $b",
                   []>;

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def SBRSRrB : FRdB<0b11,
                   (outs),
                   (ins GPR8:$rr, i8imm:$b),
                   "sbrs\t$rr, $b",
                   []>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BREQk : FBRsk<0,
                  0b001,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "breq\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_EQ)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRNEk : FBRsk<1,
                  0b001,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brne\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_NE)]>;


let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRSHk : FBRsk<1,
                  0b000,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brsh\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_SH)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRLOk : FBRsk<0,
                  0b000,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brlo\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_LO)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRMIk : FBRsk<0,
                  0b010,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brmi\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_MI)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRPLk : FBRsk<1,
                  0b101,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brpl\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_PL)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRGEk : FBRsk<1,
                  0b100,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brge\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_GE)]>;

let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRLTk : FBRsk<0,
                  0b100,
                  (outs),
                  (ins rel_condbr_7_target:$target),
                  "brlt\t$target",
                  [(AVRbrcond bb:$target, AVR_COND_LT)]>;

// BRBS s, k
// Branch if `s` flag in status register is set.
let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRBSsk : FSK<0,
                 (outs),
                 (ins i8imm:$s, rel_condbr_7_target:$k),
                 "brbs\t$s, $k",
                 []>;

// BRBC s, k
// Branch if `s` flag in status register is clear.
let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
def BRBCsk : FSK<1,
                 (outs),
                 (ins i8imm:$s, rel_condbr_7_target:$k),
                 "brbc\t$s, $k",
                 []>;


// BRCS k
// Branch if carry flag is set
def : InstAlias<"brcs\t$k", (BRBSsk 0, rel_condbr_7_target:$k)>;

// BRCC k
// Branch if carry flag is clear
def : InstAlias<"brcc\t$k", (BRBCsk 0, rel_condbr_7_target:$k)>;

// BRHS k
// Branch if half carry flag is set
def : InstAlias<"brhs\t$k", (BRBSsk 5, rel_condbr_7_target:$k)>;

// BRHC k
// Branch if half carry flag is clear
def : InstAlias<"brhc\t$k", (BRBCsk 5, rel_condbr_7_target:$k)>;

// BRTS k
// Branch if the T flag is set
def : InstAlias<"brts\t$k", (BRBSsk 6, rel_condbr_7_target:$k)>;

// BRTC k
// Branch if the T flag is clear
def : InstAlias<"brtc\t$k", (BRBCsk 6, rel_condbr_7_target:$k)>;

// BRVS k
// Branch if the overflow flag is set
def : InstAlias<"brvs\t$k", (BRBSsk 3, rel_condbr_7_target:$k)>;

// BRVC k
// Branch if the overflow flag is clear
def : InstAlias<"brvc\t$k", (BRBCsk 3, rel_condbr_7_target:$k)>;

// BRIE k
// Branch if the global interrupt flag is enabled
def : InstAlias<"brie\t$k", (BRBSsk 7, rel_condbr_7_target:$k)>;

// BRID k
// Branch if the global interrupt flag is disabled
def : InstAlias<"brid\t$k", (BRBCsk 7, rel_condbr_7_target:$k)>;

// __________________________
// Data transfer instructions
// __________________________

let hasSideEffects = 0 in
def MOVRdRr : FRdRr<0b0010,
                    0b11,
                    (outs GPR8:$rd),
                    (ins GPR8:$rr),
                    "mov\t$rd, $rr",
                    []>;

let hasSideEffects = 0 in
def MOVWRdRr : FMOVWRdRr<(outs DREGS:$dst),
                         (ins DREGS:$src),
                         "movw\t$dst, $src",
                         []>,
               Requires<[HasMOVW]>;

let isReMaterializable = 1,
isAsCheapAsAMove = 1 in
def LDIRdK : FRdK<0b1110,
                  (outs LD8:$rd),
                  (ins i8imm:$k),
                  "ldi\t$rd, $k",
                  [(set LD8:$rd, imm:$k)]>;

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
def LDRdPtr : FSTLDPtrReg<0b0,
                          (outs GPR8:$reg),
                          (ins LDSTPtrReg:$ptrreg),
                          "ld\t$reg, $ptrreg",
                          [(set GPR8:$reg, (load i16:$ptrreg))]>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
Constraints = "@earlyclobber $ptrreg" in
def LDWRdPtr : Pseudo<(outs DREGS:$reg),
                      (ins PTRDISPREGS:$ptrreg),
                      "ldw\t$reg, $ptrreg",
                      [(set DREGS:$reg, (load PTRDISPREGS:$ptrreg))]>;

let mayLoad = 1,
hasSideEffects = 0,
Constraints = "$ptrreg = $base_wb,@earlyclobber $reg,@earlyclobber $base_wb" in
def LDRdPtrPi : FSTLDPtrPiReg<0b0,
                              (outs GPR8:$reg, PTRREGS:$base_wb),
                              (ins LDSTPtrReg:$ptrreg),
                              "ld\t$reg, $ptrreg +",
                              []>;

let mayLoad = 1,
hasSideEffects = 0,
Constraints = "$ptrreg = $base_wb,@earlyclobber $reg,@earlyclobber $base_wb" in
def LDWRdPtrPi : Pseudo<(outs DREGS:$reg, PTRREGS:$base_wb),
                        (ins PTRREGS:$ptrreg),
                        "ldw\t$reg, $ptrreg +",
                        []>;

let mayLoad = 1,
hasSideEffects = 0,
Constraints = "$ptrreg = $base_wb,@earlyclobber $reg,@earlyclobber $base_wb" in
def LDRdPtrPd : FSTLDPtrPdReg<0b0,
                              (outs GPR8:$reg, PTRREGS:$base_wb),
                              (ins LDSTPtrReg:$ptrreg),
                              "ld\t$reg, -$ptrreg",
                              []>;

let mayLoad = 1,
hasSideEffects = 0,
Constraints = "$ptrreg = $base_wb,@earlyclobber $reg,@earlyclobber $base_wb" in
def LDWRdPtrPd : Pseudo<(outs DREGS:$reg, PTRREGS:$base_wb),
                        (ins PTRREGS:$ptrreg),
                        "ldw\t$reg, -$ptrreg",
                        []>;

let canFoldAsLoad = 1,
isReMaterializable = 1 in
def LDDRdPtrQ : FPqRr<0b0,
                      (outs GPR8:$reg),
                      (ins memri:$ptrreg),
                      "ldd\t$reg, $ptrreg",
                      [(set GPR8:$reg, (load addr:$ptrreg))]>;

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
hasSideEffects = 0 in
def LDDWRdYQ : Pseudo<(outs DREGS:$dst),
                      (ins memri:$src),
                      "lddw\t$dst, $src",
                      []>;

def STSKRr : F32DM<0b0,
                   (outs),
                   (ins i16imm:$dst, GPR8:$src),
                   "sts\t$dst, $src",
                   [(store GPR8:$src, imm:$dst)]>;

def STSWKRr : Pseudo<(outs),
                     (ins i16imm:$dst, DREGS:$src),
                     "stsw\t$dst, $src",
                     [(store DREGS:$src, imm:$dst)]>;

// ST P, Rr
// Stores the value of Rr into the location addressed by pointer P.
def STPtrRr : FSTLDPtrReg<0b1,
                         (outs),
                         (ins LDSTPtrReg:$ptrreg, GPR8:$reg),
                         "st\t$ptrreg, $reg",
                         [(store GPR8:$reg, i16:$ptrreg)]>;

// STW P, Rr
// Stores the value of Rr into the location addressed by pointer P.
def STWPtrRr : Pseudo<(outs),
                      (ins PTRDISPREGS:$ptrreg, DREGS:$reg),
                      "stw\t$ptrreg, $reg",
                      [(store DREGS:$reg, PTRDISPREGS:$ptrreg)]>;

// ST P+, Rr
// Stores the value of Rr into the location addressed by pointer P.
// Post increments P.
let Constraints = "$ptrreg = $base_wb,@earlyclobber $base_wb" in
def STPtrPiRr : FSTLDPtrPiReg<0b1,
                              (outs LDSTPtrReg:$base_wb),
                              (ins LDSTPtrReg:$ptrreg, GPR8:$reg, i8imm:$offs),
                              "st\t$ptrreg +, $reg",
                              [(set i16:$base_wb,
                              (post_store GPR8:$reg, i16:$ptrreg, imm:$offs))]>;

// STW P+, Rr
// Stores the value of Rr into the location addressed by pointer P.
// Post increments P.
let Constraints = "$ptrreg = $base_wb,@earlyclobber $base_wb" in
def STWPtrPiRr : Pseudo<(outs PTRREGS:$base_wb),
                        (ins PTRREGS:$ptrreg, DREGS:$trh, i8imm:$offs),
                        "stw\t$ptrreg +, $trh",
                        [(set PTRREGS:$base_wb,
                         (post_store DREGS:$trh, PTRREGS:$ptrreg, imm:$offs))]>;

// ST -P, Rr
// Stores the value of Rr into the location addressed by pointer P.
// Pre decrements P.
let Constraints = "$ptrreg = $base_wb,@earlyclobber $base_wb" in
def STPtrPdRr : FSTLDPtrPdReg<0b1,
                              (outs LDSTPtrReg:$base_wb),
                              (ins LDSTPtrReg:$ptrreg, GPR8:$reg, i8imm:$offs),
                              "st\t-$ptrreg, $reg",
                              [(set i16:$base_wb,
                              (pre_store GPR8:$reg, i16:$ptrreg, imm:$offs))]>;

// STW -P, Rr
// Stores the value of Rr into the location addressed by pointer P.
// Pre decrements P.
let Constraints = "$ptrreg = $base_wb,@earlyclobber $base_wb" in
def STWPtrPdRr : Pseudo<(outs PTRREGS:$base_wb),
                        (ins PTRREGS:$ptrreg, DREGS:$reg, i8imm:$offs),
                        "stw\t-$ptrreg, $reg",
                        [(set PTRREGS:$base_wb,
                         (pre_store DREGS:$reg, PTRREGS:$ptrreg, imm:$offs))]>;

/// STD P+q, Rr
// Stores the value of Rr into the location addressed by pointer P with a displacement of q.
// Does not modify P.
def STDPtrQRr : FPqRr<0b1,
                      (outs),
                      (ins memri:$dst, GPR8:$src),
                      "std\t$dst, $src",
                      [(store GPR8:$src, addr:$dst)]>;
/// STDW P+q, Rr
// Stores the value of Rr into the location addressed by pointer P with a displacement of q.
// Does not modify P.
def STDWPtrQRr : Pseudo<(outs),
                        (ins memri:$dst, DREGS:$src),
                        "stdw\t$dst, $src",
                        [(store DREGS:$src, addr:$dst)]>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
hasSideEffects = 0,
Uses = [R31R30],
Defs = [R0]  in
def LPM : F16<0b1001010111001000,
              (outs),
              (ins),
              "lpm",
              []>,
          Requires<[HasLPM]>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
hasSideEffects = 0,
Uses = [R31R30] in
def LPMRdZ : FLPMX<0,
                   0,
                   (outs GPR8:$dst),
                   (ins),
                   "lpm\t$dst, Z",
                   []>,
             Requires<[HasLPMX]>;

let canFoldAsLoad = 1,
isReMaterializable = 1,
hasSideEffects = 0,
Uses = [R31R30] in
def LPMWRdZ : Pseudo<(outs DREGS:$dst),
                     (ins),
                     "lpmw\t$dst, Z",
                     []>;

let mayLoad = 1,
hasSideEffects = 0,
Uses = [R31R30],
Defs = [R31R30] in
def LPMRdZPi : FLPMX<0,
                     1,
                     (outs GPR8:$dst),
                     (ins),
                     "lpm\t$dst, Z+",
                     []>;

let mayLoad = 1,
hasSideEffects = 0,
Uses = [R31R30],
Defs = [R31R30] in
def LPMWRdZPi : Pseudo<(outs DREGS:$dst),
                       (ins),
                       "lpmw\t$dst, Z+",
                       []>;

let mayLoad = 1,
hasSideEffects = 0,
Uses = [R31R30],
Defs = [R0] in
def ELPM : F16<0b1001010111011000,
               (outs),
               (ins),
               "elpm",
               []>;

let mayLoad = 1,
hasSideEffects = 0,
Uses = [R31R30] in 
def ELPMRdZ : FLPMX<1,
                    0,
                    (outs GPR8:$dst),
                    (ins),
                    "elpm\t$dst, Z",
                    []>;

let mayLoad = 1,
hasSideEffects = 0,
Uses = [R31R30],
Defs = [R31R30] in
def ELPMRdZPi : FLPMX<1,
                      1,
                      (outs GPR8:$dst),
                      (ins),
                      "elpm\t$dst, Z+",
                      []>;

// TODO: set instruction flags properly (hasSideEffects,etc)
let Uses = [R31R30, R0, R1] in
def SPM : F16<0b1001010111101000,
              (outs),
              (ins),
              "spm",
              []>;

// TODO: set instruction flags properly (hasSideEffects,etc)
let Uses = [R31R30, R0, R1],
Defs = [R31R30] in
def SPMZPi : F16<0b1001010111111000,
                 (outs),
                 (ins),
                 "spm Z+",
                 []>;

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
hasSideEffects = 0,
mayStore = 1 in
def PUSHRr : FRd<0b1001,
                 0b0011111,
                 (outs),
                 (ins GPR8:$reg),
                 "push\t$reg",
                 []>;

let Defs = [SP],
Uses = [SP],
hasSideEffects = 0,
mayStore = 1 in
def PUSHWRr : Pseudo<(outs),
                     (ins DREGS:$reg),
                     "pushw\t$reg",
                     []>;

let Defs = [SP],
Uses = [SP],
hasSideEffects = 0,
mayLoad = 1 in
def POPRd : FRd<0b1001,
                0b0001111,
                (outs GPR8:$reg),
                (ins),
                "pop\t$reg",
                []>;

let Defs = [SP],
Uses = [SP],
hasSideEffects = 0,
mayLoad = 1 in
def POPWRd : Pseudo<(outs DREGS:$reg),
                    (ins),
                    "popw\t$reg",
                    []>;

let Uses = [R31R30] in
def XCHZRd : FZRd<0b100,
                  (outs GPR8:$rd),
                  (ins),
                  "xch\tZ, $rd",
                  []>;

let Uses = [R31R30] in
def LASZRd : FZRd<0b101,
                  (outs GPR8:$rd),
                  (ins),
                  "las\tZ, $rd",
                  []>;

let Uses = [R31R30] in
def LACZRd : FZRd<0b110,
                  (outs GPR8:$rd),
                  (ins),
                  "lac\tZ, $rd",
                  []>;

let Uses = [R31R30] in
def LATZRd : FZRd<0b111,
                  (outs GPR8:$rd),
                  (ins),
                  "lat\tZ, $rd",
                  []>;

// _______________________________
// Bit and bit-test instructions
// _______________________________

let Constraints = "$src = $rd",
Defs = [SREG] in
def LSLRd : FRdRr<0b0000,
                  0b11,
                  (outs GPR8:$rd),
                  (ins GPR8:$src),
                  "lsl\t$rd",
                  [(set GPR8:$rd, (AVRlsl GPR8:$src)), (implicit SREG)]>;

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

let Constraints = "$rr = $rd",
Defs = [SREG],
Uses = [SREG] in
def ROLRd : FRdRr<0b0001,
                  0b11,
                  (outs GPR8:$rd),
                  (ins GPR8:$rr),
                  "rol\t$rd",
                  [(set GPR8:$rd, (AVRrol GPR8:$rr)), (implicit SREG)]>;

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
def SBIAb : FIOBIT<0b10,
                   (outs),
                   (ins i16imm:$addr, i8imm:$bit),
                   "sbi\t$addr, $bit",
                   [(store (or (i8 (load lowioaddr8:$addr)), iobitpos8:$bit),
                     lowioaddr8:$addr)]>;

def CBIAb : FIOBIT<0b00,
                   (outs),
                   (ins i16imm:$addr, i8imm:$bit),
                   "cbi\t$addr, $bit",
                   [(store (and (i8 (load lowioaddr8:$addr)), iobitposn8:$bit),
                     lowioaddr8:$addr)]>;

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def SBICAb : FIOBIT<0b01,
                    (outs),
                    (ins i16imm:$a, i8imm:$b),
                    "sbic\t$a, $b",
                    []>;

let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
def SBISAb : FIOBIT<0b11,
                    (outs),
                    (ins i16imm:$a, i8imm:$b),
                    "sbis\t$a, $b",
                    []>;

let Defs = [SREG] in
def BST : FRdB<0b01,
               (outs),
               (ins GPR8:$rd, i8imm:$b),
               "bst\t$rd, $b",
               []>;

let Uses = [SREG] in
def BLD : FRdB<0b00,
               (outs),
               (ins GPR8:$rd, i8imm:$b),
               "bld\t$rd, $b",
               []>;

// SBR Rd, K
// Alias for ORI Rd, K
let Constraints = "$src = $rd",
Defs = [SREG] in
def SBRRdK : FRdK<0b0110,
                  (outs LD8:$rd),
                  (ins LD8:$src, i8imm:$k),
                  "sbr\t$rd, $k",
                  [(set LD8:$rd, (or LD8:$src, imm:$k)),
                   (implicit SREG)]>;

let Defs = [SREG] in
def CBRRdK : FRdK<0b0111,
                  (outs LD8:$rd),
                  (ins LD8:$src, i8imm_com:$k),
                  "cbr\t$rd, $k",
                  []>;

// CLR Rd
// Alias for EOR Rd, Rd
// -------------
// Clears all bits in a register.
let Constraints = "$src = $rd, $rd = $src2",
Defs = [SREG] in
def CLRRd : FRdRr<0b0010,
                  0b01,
                  (outs GPR8:$rd),
                  (ins GPR8:$src, GPR8:$src2),
                  "clr\t$rd",
                  []>;

// SER Rd
// ---------
// Sets all bits in a register.
let Constraints = "$src = $dst" in
def SERRd : FRd<0b1110,
                0b1111111,
                (outs GPR8:$dst),
                (ins GPR8:$src),
                "ser\t$dst",
                []>;

//===----------------------------------------------------------------------===//
// Status register instructions
//
// Instructions derive from format FFL (FL as in flag)
// <|1001|0100|bfff|1000>
//
// Where b = 0 if we are setting the flag
//   and b = 1 if we are clearing the flag
//   and f is the flag mask
//
// Flag masks:
// C - 000 (carry)
// Z - 001 (zero)
// N - 010 (negative)
// V - 011 (overflow)
// S - 100 (signed)
// H - 101 (half carry)
// T - 110 (T flag)
// I - 111 (interrupt)
//===----------------------------------------------------------------------===//

let Defs = [SREG] in
def BSET : FS<0,
              (outs),
              (ins i8imm:$s),
              "bset\t$s",
              []>;

let Defs = [SREG] in
def BCLR : FS<1,
              (outs),
              (ins i8imm:$s),
              "bclr\t$s",
              []>;

let Defs = [SREG] in
def SEC : FFL<0,
              0b000,
              (outs),
              (ins),
              "sec",
              []>;

let Defs = [SREG] in
def CLC : FFL<1,
              0b000,
              (outs),
              (ins),
              "clc",
              []>;

let Defs = [SREG] in
def SEZ : FFL<0,
              0b001,
              (outs),
              (ins),
              "sez",
              []>;

let Defs = [SREG] in
def CLZ : FFL<1,
              0b001,
              (outs),
              (ins),
              "clz",
              []>;

let Defs = [SREG] in
def SEN : FFL<0,
              0b010,
              (outs),
              (ins),
              "sen",
              []>;

let Defs = [SREG] in
def CLN : FFL<1,
              0b010,
              (outs),
              (ins),
              "cln",
              []>;

let Defs = [SREG] in
def SEV : FFL<0,
              0b011,
              (outs),
              (ins),
              "sev",
              []>;

let Defs = [SREG] in
def CLV : FFL<1,
              0b011,
              (outs),
              (ins),
              "clv",
              []>;

let Defs = [SREG] in
def SES : FFL<0,
              0b100,
              (outs),
              (ins),
              "ses",
              []>;

let Defs = [SREG] in
def CLS : FFL<1,
              0b100,
              (outs),
              (ins),
              "cls",
              []>;

let Defs = [SREG] in
def SEH : FFL<0,
              0b101,
              (outs),
              (ins),
              "seh",
              []>;

let Defs = [SREG] in
def CLH : FFL<1,
              0b101,
              (outs),
              (ins),
              "clh",
              []>;

let Defs = [SREG] in
def SET : FFL<0,
              0b110,
              (outs),
              (ins),
              "set",
              []>;

let Defs = [SREG] in
def CLT : FFL<1,
              0b110,
              (outs),
              (ins),
              "clt",
              []>;

let Defs = [SREG] in
def SEI : FFL<0,
              0b111,
              (outs),
              (ins),
              "sei",
              []>;

let Defs = [SREG] in
def CLI : FFL<1,
              0b111,
              (outs),
              (ins),
              "cli",
              []>;

//===----------------------------------------------------------------------===//
// Special/Control instructions
//===----------------------------------------------------------------------===//

// BREAK
// Breakpoint instruction
// ---------
// <|1001|0101|1001|1000>
def BREAK : F16<0b1001010110011000,
                (outs),
                (ins),
                "break",
                []>;

// NOP
// No-operation instruction
// ---------
// <|0000|0000|0000|0000>
def NOP : F16<0b0000000000000000,
              (outs),
              (ins),
              "nop",
              []>;

// SLEEP
// Sleep instruction
// ---------
// <|1001|0101|1000|1000>
def SLEEP : F16<0b1001010110001000,
                (outs),
                (ins),
                "sleep",
                []>;

// WDR
// Watchdog reset
// ---------
// <|1001|0101|1010|1000>
def WDR : F16<0b1001010110101000,
              (outs),
              (ins),
              "wdr",
              []>;


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
hasSideEffects = 0 in
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
hasSideEffects = 0 in
def SPREAD : Pseudo<(outs DREGS:$dst),
                    (ins GPRSP:$src),
                    "spread\t$dst, $src",
                    []>;

let Defs = [SP],
hasSideEffects = 0 in
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
//:FIXME: i think it's better to emit an extract subreg node in the DAG than
// all this mess once we get optimal shift code
def : Pat<(i8 (trunc (AVRlsr (AVRlsr (AVRlsr (AVRlsr (AVRlsr (AVRlsr (AVRlsr
                     (AVRlsr DREGS:$src)))))))))),
          (EXTRACT_SUBREG DREGS:$src, sub_hi)>;

// :FIXME: DAGCombiner produces an shl node after legalization from these seq:
// BR_JT -> (mul x, 2) -> (shl x, 1)
def : Pat<(shl DREGS:$src1, (i8 1)),
          (LSLWRd DREGS:$src1)>;
