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
  return CurDAG->getTargetConstant(-N->getAPIntValue(), SDLoc(N), MVT::i8);
}]>;

def imm16_neg_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(-N->getAPIntValue(), SDLoc(N), MVT::i16);
}]>;

def imm0_63_neg : PatLeaf<(imm),
[{
  int64_t val = -N->getSExtValue();
  return val >= 0 && val < 64;
}], imm16_neg_XFORM>;

def uimm6 : PatLeaf<(imm), [{ return isUInt<6>(N->getZExtValue()); }]>;

def ioaddr_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(uint8_t(N->getZExtValue()) - 0x20, SDLoc(N), MVT::i8);
}]>;

def iobitpos8_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(Log2_32(uint8_t(N->getZExtValue())),
                                   SDLoc(N), MVT::i8);
}]>;

def iobitposn8_XFORM : SDNodeXForm<imm,
[{
  return CurDAG->getTargetConstant(Log2_32(uint8_t(~N->getZExtValue())),
                                   SDLoc(N), MVT::i8);
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

// Address operand for SP+imm used by STD{W}SPQRr
def memspi : Operand<iPTR>
{
  let MIOperandInfo = (ops GPRSP, i16imm);
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

// The target of a 22 or 16-bit call/jmp instruction.
def call_target : Operand<iPTR>
{
    let EncoderMethod = "getCallTargetEncoding";
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
// It converts the pointer register number into a two-bit field used in the instruction.
def LDSTPtrReg : Operand<i16>
{
    let MIOperandInfo = (ops PTRREGS);
    let EncoderMethod = "getLDSTPtrRegEncoding";
    
    let ParserMatchClass = PtrRegAsmOperand;
}

// A special operand type for the LDD/STD instructions.
// It behaves identically to the LD/ST version, except restricts
// the pointer registers to Y and Z.
def LDDSTDPtrReg : Operand<i16>
{
    let MIOperandInfo = (ops PTRDISPREGS);
    let EncoderMethod = "getLDSTPtrRegEncoding";

    let ParserMatchClass = PtrRegAsmOperand;
}

//===----------------------------------------------------------------------===//
// AVR predicates for subtarget features
//===----------------------------------------------------------------------===//

def HasSRAM       :    Predicate<"Subtarget->hasSRAM()">,
                         AssemblerPredicate<"FeatureSRAM">;

def HasJMPCALL    :    Predicate<"Subtarget->hasJMPCALL()">,
                         AssemblerPredicate<"FeatureJMPCALL">;

def HasIJMPCALL   :    Predicate<"Subtarget->hasIJMPCALL()">,
                         AssemblerPredicate<"FeatureIJMPCALL">;

def HasEIJMPCALL  :    Predicate<"Subtarget->hasEIJMPCALL()">,
                         AssemblerPredicate<"FeatureEIJMPCALL">;

def HasADDSUBIW   :    Predicate<"Subtarget->hasADDSUBIW()">,
                         AssemblerPredicate<"FeatureADDSUBIW">;

def HasSmallStack :    Predicate<"Subtarget->HasSmallStack()">,
                         AssemblerPredicate<"FeatureSmallStack">;

def HasMOVW       :    Predicate<"Subtarget->hasMOVW()">,
                         AssemblerPredicate<"FeatureMOVW">;

def HasLPM        :    Predicate<"Subtarget->hasLPM()">,
                         AssemblerPredicate<"FeatureLPM">;

def HasLPMX       :    Predicate<"Subtarget->hasLPMX()">,
                         AssemblerPredicate<"FeatureLPMX">;

def HasELPM       :    Predicate<"Subtarget->hasELPM()">,
                         AssemblerPredicate<"FeatureELPM">;

def HasELPMX      :    Predicate<"Subtarget->hasELPMX()">,
                         AssemblerPredicate<"FeatureELPMX">;

def HasSPM        :    Predicate<"Subtarget->hasSPM()">,
                         AssemblerPredicate<"FeatureSPM">;

def HasSPMX       :    Predicate<"Subtarget->hasSPMX()">,
                         AssemblerPredicate<"FeatureSPMX">;

def HasDES        :    Predicate<"Subtarget->hasDES()">,
                         AssemblerPredicate<"FeatureDES">;

def SupportsRMW   :    Predicate<"Subtarget->supportsRMW()">,
                         AssemblerPredicate<"FeatureRMW">;

def SupportsMultiplication : Predicate<"Subtarget->supportsMultiplication()">,
                               AssemblerPredicate<"FeatureMultiplication">;

def HasBREAK      :    Predicate<"Subtarget->hasBREAK()">,
                         AssemblerPredicate<"FeatureBREAK">;

def HasTinyEncoding : Predicate<"Subtarget->hasTinyEncoding()">,
                        AssemblerPredicate<"FeatureTinyEncoding">;


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

// Addition operations.
let isCommutable = 1,
Constraints = "$src = $rd",
Defs = [SREG] in
{
    // ADD Rd, Rr
    // Adds two 8-bit registers.
    def ADDRdRr : FRdRr<0b0000,
                        0b11,
                        (outs GPR8:$rd),
                        (ins GPR8:$src, GPR8:$rr),
                        "add\t$rd, $rr",
                        [(set GPR8:$rd, (add GPR8:$src, GPR8:$rr)),
                         (implicit SREG)]>;

    // ADDW Rd+1:Rd, Rr+1:Rr
    // Pseudo instruction to add four 8-bit registers as two 16-bit values.
    //
    // Expands to:
    // add Rd,    Rr
    // adc Rd+1, Rr+1
    def ADDWRdRr : Pseudo<(outs DREGS:$rd),
                          (ins DREGS:$src, DREGS:$rr),
                          "addw\t$rd, $rr",
                          [(set DREGS:$rd, (add DREGS:$src, DREGS:$rr)),
                           (implicit SREG)]>;

    // ADC Rd, Rr
    // Adds two 8-bit registers with carry.
    let Uses = [SREG] in
    def ADCRdRr : FRdRr<0b0001,
                        0b11,
                        (outs GPR8:$rd),
                        (ins GPR8:$src, GPR8:$rr),
                        "adc\t$rd, $rr",
                        [(set GPR8:$rd, (adde GPR8:$src, GPR8:$rr)),
                         (implicit SREG)]>;

    // ADCW Rd+1:Rd, Rr+1:Rr
    // Pseudo instruction to add four 8-bit registers as two 16-bit values with carry.
    //
    // Expands to:
    // adc Rd,   Rr
    // adc Rd+1, Rr+1
    let Uses = [SREG] in
    def ADCWRdRr : Pseudo<(outs DREGS:$rd),
                          (ins DREGS:$src, DREGS:$rr),
                          "adcw\t$rd, $rr",
                          [(set DREGS:$rd, (adde DREGS:$src, DREGS:$rr)),
                           (implicit SREG)]>;

    // AIDW Rd, k
    // Adds an immediate 6-bit value K to Rd, placing the result in Rd.
    def ADIWRdK : FWRdK<0b0,
                        (outs IWREGS:$rd),
                        (ins IWREGS:$src, i16imm:$k),
                        "adiw\t$rd, $k",
                        [(set IWREGS:$rd, (add IWREGS:$src, uimm6:$k)),
                         (implicit SREG)]>,
                  Requires<[HasADDSUBIW]>;
}

// Subtraction operations
let Constraints = "$src = $rd",
Defs = [SREG] in
{
    // SUB Rd, Rr
    // Subtracts the 8-bit value of Rr from Rd and places the value in Rd.
    def SUBRdRr : FRdRr<0b0001,
                        0b10,
                        (outs GPR8:$rd),
                        (ins GPR8:$src, GPR8:$rr),
                        "sub\t$rd, $rr",
                        [(set GPR8:$rd, (sub GPR8:$src, GPR8:$rr)),
                         (implicit SREG)]>;

    // SUBW Rd+1:Rd, Rr+1:Rr
    // Subtracts two 16-bit values and places the result into Rd.
    //
    // Expands to:
    // sub Rd,   Rr
    // sbc Rd+1, Rr+1
    def SUBWRdRr : Pseudo<(outs DREGS:$rd),
                          (ins DREGS:$src, DREGS:$rr),
                          "subw\t$rd, $rr",
                          [(set DREGS:$rd, (sub DREGS:$src, DREGS:$rr)),
                           (implicit SREG)]>;

    def SUBIRdK : FRdK<0b0101,
                       (outs LD8:$rd),
                       (ins LD8:$src, i8imm:$k),
                       "subi\t$rd, $k",
                       [(set LD8:$rd, (sub LD8:$src, imm:$k)),
                        (implicit SREG)]>;

    // SUBIW Rd+1:Rd, K+1:K
    //
    // Expands to:
    // subi Rd,   K
    // sbci Rd+1, K+1
    def SUBIWRdK : Pseudo<(outs DLDREGS:$rd),
                          (ins DLDREGS:$src, i16imm:$rr),
                          "subiw\t$rd, $rr",
                          [(set DLDREGS:$rd, (sub DLDREGS:$src, imm:$rr)),
                           (implicit SREG)]>;
    
    def SBIWRdK : FWRdK<0b1,
                        (outs IWREGS:$rd),
                        (ins IWREGS:$src, i16imm:$k),
                        "sbiw\t$rd, $k",
                        [(set IWREGS:$rd, (sub IWREGS:$src, uimm6:$k)),
                         (implicit SREG)]>,
                  Requires<[HasADDSUBIW]>;

    // Subtract with carry operations which must read the carry flag in SREG.
    let Uses = [SREG] in
    {
        def SBCRdRr : FRdRr<0b0000,
                            0b10,
                            (outs GPR8:$rd),
                            (ins GPR8:$src, GPR8:$rr),
                            "sbc\t$rd, $rr",
                            [(set GPR8:$rd, (sube GPR8:$src, GPR8:$rr)),
                             (implicit SREG)]>;

        // SBCW Rd+1:Rd, Rr+1:Rr
        //
        // Expands to:
        // sbc Rd,   Rr
        // sbc Rd+1, Rr+1
        def SBCWRdRr : Pseudo<(outs DREGS:$rd),
                              (ins DREGS:$src, DREGS:$rr),
                              "sbcw\t$rd, $rr",
                              [(set DREGS:$rd, (sube DREGS:$src, DREGS:$rr)),
                               (implicit SREG)]>;

        def SBCIRdK : FRdK<0b0100,
                           (outs LD8:$rd),
                           (ins LD8:$src, i8imm:$k),
                           "sbci\t$rd, $k",
                           [(set LD8:$rd, (sube LD8:$src, imm:$k)),
                            (implicit SREG)]>;

        // SBCIW Rd+1:Rd, K+1:K
        // sbci Rd,   K
        // sbci Rd+1, K+1
        def SBCIWRdK : Pseudo<(outs DLDREGS:$rd),
                              (ins DLDREGS:$src, i16imm:$rr),
                              "sbciw\t$rd, $rr",
                              [(set DLDREGS:$rd, (sube DLDREGS:$src, imm:$rr)),
                               (implicit SREG)]>;
    }
}

// Increment/decrement operations.
let Constraints = "$src = $rd",
Defs = [SREG] in
{
    def INCRd : FRd<0b1001,
                    0b0100011,
                    (outs GPR8:$rd),
                    (ins GPR8:$src),
                    "inc\t$rd",
                    [(set GPR8:$rd, (add GPR8:$src, 1)), (implicit SREG)]>;

    def DECRd : FRd<0b1001,
                    0b0101010,
                    (outs GPR8:$rd),
                    (ins GPR8:$src),
                    "dec\t$rd",
                    [(set GPR8:$rd, (add GPR8:$src, -1)), (implicit SREG)]>;
}

// Multiplication operations.
let isCommutable = 1,
Defs = [R1, R0, SREG] in
{
    // MUL Rd, Rr
    // Multiplies Rd by Rr and places the result into Rd.
    def MULRdRr : FRdRr<0b1001,
                        0b11,
                        (outs),
                        (ins GPR8:$lhs, GPR8:$rhs),
                        "mul\t$lhs, $rhs",
                        []>,
                  Requires<[SupportsMultiplication]>;

    // MULW DST+1:DST, LHS+1:LHS, RHS+1:RHS
    //
    // The earlyclobber flag is required because the pseudo expansion needs $rd
    // and $src to have different registers.
    //
    // Expands to:
    // mul  LHS, RHS
    // movw DST+1:DST, r1:r0
    // mul  LHS, RHS+1
    // add  DST+1, r0
    // mul  LHS+1, RHS
    // add  DST+1, r0
    // clr r1
    let Constraints = "@earlyclobber $dst" in
    def MULWRdRr : Pseudo<(outs DREGS:$dst),
                          (ins DREGS:$lhs, DREGS:$rhs),
                          "mulw\t$dst, $lhs, $rhs",
                          [(set DREGS:$dst, (mul DREGS:$lhs, DREGS:$rhs)),
                           (implicit SREG)]>,
                   Requires<[SupportsMultiplication]>;

    // MULP Rd, Rr
    //
    // This pseudo is used as a marker to insert a "clr r1" after the operation.
    //
    // Expands to:
    // mul Rd, Rr
    // mov Rd, r0
    // clr r1
    let Constraints = "$src = $rd" in
    def MULRdRrP : Pseudo<(outs GPR8:$rd),
                          (ins GPR8:$src, GPR8:$rr),
                          "mulp\t$rd, $src, $rr",
                          [(set GPR8:$rd, (mul GPR8:$src, GPR8:$rr)),
                           (implicit SREG)]>,
                   Requires<[SupportsMultiplication]>;


    def MULSRdRr : FMUL2RdRr<0,
                             (outs),
                             (ins GPR8:$lhs, GPR8:$rhs),
                             "muls\t$lhs, $rhs",
                             []>,
                   Requires<[SupportsMultiplication]>;

    def MULSURdRr : FMUL2RdRr<1,
                              (outs),
                              (ins GPR8:$lhs, GPR8:$rhs),
                              "mulsu\t$lhs, $rhs",
                              []>,
                    Requires<[SupportsMultiplication]>;

    def FMUL : FFMULRdRr<0b01,
                         (outs),
                         (ins GPR8:$lhs, GPR8:$rhs),
                         "fmul\t$lhs, $rhs",
                         []>,
               Requires<[SupportsMultiplication]>;

    def FMULS : FFMULRdRr<0b10,
                          (outs),
                          (ins GPR8:$lhs, GPR8:$rhs),
                          "fmuls\t$lhs, $rhs",
                          []>,
                Requires<[SupportsMultiplication]>;

    def FMULSU : FFMULRdRr<0b11,
                           (outs),
                           (ins GPR8:$lhs, GPR8:$rhs),
                           "fmulsu\t$lhs, $rhs",
                           []>,
                 Requires<[SupportsMultiplication]>;
}

let Defs = [R15, R14, R13, R12, R11, R10, R9,
            R8, R7, R6, R5, R4, R3, R2, R1, R0] in
def DESK : FDES<(outs),
                (ins i8imm:$k),
                "des\t$k",
                []>,
           Requires<[HasDES]>;

// Logic instructions
let Constraints = "$src = $rd",
Defs = [SREG] in
{
    // Register-Register logic instructions (which have the property of commutativity).
    let isCommutable = 1 in
    {
        def ANDRdRr : FRdRr<0b0010,
                            0b00,
                            (outs GPR8:$rd),
                            (ins GPR8:$src, GPR8:$rr),
                            "and\t$rd, $rr",
                            [(set GPR8:$rd, (and GPR8:$src, GPR8:$rr)),
                             (implicit SREG)]>;

        // ANDW Rd+1:Rd, Rr+1:Rr
        //
        // Expands to:
        // and Rd,   Rr
        // and Rd+1, Rr+1
        def ANDWRdRr : Pseudo<(outs DREGS:$rd),
                              (ins DREGS:$src, DREGS:$rr),
                              "andw\t$rd, $rr",
                              [(set DREGS:$rd, (and DREGS:$src, DREGS:$rr)),
                               (implicit SREG)]>;

        def ORRdRr : FRdRr<0b0010,
                           0b10,
                           (outs GPR8:$rd),
                           (ins GPR8:$src, GPR8:$rr),
                           "or\t$rd, $rr",
                           [(set GPR8:$rd, (or GPR8:$src, GPR8:$rr)),
                            (implicit SREG)]>;

        // ORW Rd+1:Rd, Rr+1:Rr
        //
        // Expands to:
        // or Rd,   Rr
        // or Rd+1, Rr+1
        def ORWRdRr : Pseudo<(outs DREGS:$rd),
                             (ins DREGS:$src, DREGS:$rr),
                             "orw\t$rd, $rr",
                             [(set DREGS:$rd, (or DREGS:$src, DREGS:$rr)),
                              (implicit SREG)]>;

        def EORRdRr : FRdRr<0b0010,
                            0b01,
                            (outs GPR8:$rd),
                            (ins GPR8:$src, GPR8:$rr),
                            "eor\t$rd, $rr",
                            [(set GPR8:$rd, (xor GPR8:$src, GPR8:$rr)),
                             (implicit SREG)]>;

        // EORW Rd+1:Rd, Rr+1:Rr
        //
        // Expands to:
        // eor Rd,   Rr
        // eor Rd+1, Rr+1
        def EORWRdRr : Pseudo<(outs DREGS:$rd),
                              (ins DREGS:$src, DREGS:$rr),
                              "eorw\t$rd, $rr",
                              [(set DREGS:$rd, (xor DREGS:$src, DREGS:$rr)),
                               (implicit SREG)]>;
    }

    def ANDIRdK : FRdK<0b0111,
                       (outs LD8:$rd),
                       (ins LD8:$src, i8imm:$k),
                       "andi\t$rd, $k",
                       [(set LD8:$rd, (and LD8:$src, imm:$k)),
                        (implicit SREG)]>;

    // ANDI Rd+1:Rd, K+1:K
    //
    // Expands to:
    // andi Rd,   K
    // andi Rd+1, K+1
    def ANDIWRdK : Pseudo<(outs DLDREGS:$rd),
                          (ins DLDREGS:$src, i16imm:$k),
                          "andiw\t$rd, $k",
                          [(set DLDREGS:$rd, (and DLDREGS:$src, imm:$k)),
                           (implicit SREG)]>;

    def ORIRdK : FRdK<0b0110,
                      (outs LD8:$rd),
                      (ins LD8:$src, i8imm:$k),
                      "ori\t$rd, $k",
                      [(set LD8:$rd, (or LD8:$src, imm:$k)),
                       (implicit SREG)]>;

    // ORIW Rd+1:Rd, K+1,K
    //
    // Expands to:
    // ori Rd,   K
    // ori Rd+1, K+1
    def ORIWRdK : Pseudo<(outs DLDREGS:$rd),
                         (ins DLDREGS:$src, i16imm:$rr),
                         "oriw\t$rd, $rr",
                         [(set DLDREGS:$rd, (or DLDREGS:$src, imm:$rr)),
                          (implicit SREG)]>;
}

// One's/two's compliment operations.
let Constraints = "$src = $rd",
Defs = [SREG] in
{
    def COMRd : FRd<0b1001,
                    0b0100000,
                    (outs GPR8:$rd),
                    (ins GPR8:$src),
                    "com\t$rd",
                    [(set GPR8:$rd, (not GPR8:$src)), (implicit SREG)]>;

    // COMW Rd+1:Rd
    //
    // Expands to:
    // com Rd
    // com Rd+1
    def COMWRd : Pseudo<(outs DREGS:$rd),
                        (ins DREGS:$src),
                        "comw\t$rd",
                        [(set DREGS:$rd, (not DREGS:$src)), (implicit SREG)]>;

    //:TODO: optimize NEG for wider types
    def NEGRd : FRd<0b1001,
                    0b0100001,
                    (outs GPR8:$rd),
                    (ins GPR8:$src),
                    "neg\t$rd",
                    [(set GPR8:$rd, (ineg GPR8:$src)), (implicit SREG)]>;
}

// TST Rd
// Test for zero of minus.
// This operation is identical to a `Rd AND Rd`.
//def : InstAlias<"tst\t$rd", (ANDRdRr GPR8:$rd, GPR8:$rd), 1>;

let Defs = [SREG] in
def TSTRd : FTST<0b0010,
                  0b00,
                  (outs),
                  (ins GPR8:$rd),
                  "tst\t$rd",
                  [(AVRtst GPR8:$rd)]>;

// Jump instructions
let isBarrier = 1,
isBranch = 1,
isTerminator = 1 in
{
    def RJMPk : FBRk<0,
                     (outs),
                     (ins rel_condbr_13_target:$target),
                     "rjmp\t$target",
                     [(br bb:$target)]>;

    let isIndirectBranch = 1,
    Uses = [R31R30] in
    def IJMP : F16<0b1001010000001001,
                   (outs),
                   (ins),
                   "ijmp",
                   []>,
               Requires<[HasIJMPCALL]>;

    let isIndirectBranch = 1,
    Uses = [R31R30] in
    def EIJMP : F16<0b1001010000011001,
                    (outs),
                    (ins),
                    "eijmp",
                    []>,
                Requires<[HasEIJMPCALL]>;

    def JMPk : F32BRk<0b110,
                      (outs),
                      (ins call_target:$k),
                      "jmp\t$k",
                      []>,
               Requires<[HasJMPCALL]>;
}

// Call instructions
let isCall = 1 in
{
    // SP is marked as a use to prevent stack-pointer assignments that appear
    // immediately before calls from potentially appearing dead.
    let Uses = [SP] in
    def RCALLk : FBRk<1,
                      (outs),
                      (ins rel_condbr_13_target:$target),
                      "rcall\t$target",
                      []>;

    // SP is marked as a use to prevent stack-pointer assignments that appear
    // immediately before calls from potentially appearing dead.
    let Uses = [SP, R31R30] in
    def ICALL : F16<0b1001010100001001,
                    (outs),
                    (ins variable_ops),
                    "icall",
                    []>,
                Requires<[HasIJMPCALL]>;

    // SP is marked as a use to prevent stack-pointer assignments that appear
    // immediately before calls from potentially appearing dead.
    let Uses = [SP, R31R30] in
    def EICALL : F16<0b1001010000011001,
                     (outs),
                     (ins variable_ops),
                     "eicall",
                     []>,
                 Requires<[HasEIJMPCALL]>;

    // SP is marked as a use to prevent stack-pointer assignments that appear
    // immediately before calls from potentially appearing dead.
    //
    //:TODO: the imm field can be either 16 or 22 bits in devices with more
    // than 64k of ROM, fix it once we support the largests devices.
    let Uses = [SP] in
    def CALLk : F32BRk<0b111,
                       (outs),
                       (ins call_target:$k),
                       "call\t$k",
                       [(AVRcall imm:$k)]>,
                Requires<[HasJMPCALL]>;
}

// Return instructions.
let isTerminator = 1,
isReturn = 1,
isBarrier = 1 in 
{
    def RET : F16<0b1001010100001000,
                  (outs),
                  (ins),
                  "ret",
                  [(AVRretflag)]>;

    def RETI : F16<0b1001010100011000,
                   (outs),
                   (ins),
                   "reti",
                   [(AVRretiflag)]>;
}

// Compare operations.
let Defs = [SREG] in
{
    // CPSE Rd, Rr
    // Compare Rd and Rr, skipping the next instruction if they are equal.
    let isBarrier = 1,
    isBranch = 1,
    isTerminator = 1 in
    def CPSE : FRdRr<0b0001,
                     0b00,
                     (outs),
                     (ins GPR8:$rd, GPR8:$rr),
                     "cpse\t$rd, $rr",
                     []>;

    def CPRdRr : FRdRr<0b0001,
                       0b01,
                       (outs),
                       (ins GPR8:$rd, GPR8:$rr),
                       "cp\t$rd, $rr",
                       [(AVRcmp GPR8:$rd, GPR8:$rr), (implicit SREG)]>;

    // CPW Rd+1:Rd, Rr+1:Rr
    //
    // Expands to:
    // cp  Rd,   Rr
    // cpc Rd+1, Rr+1
    def CPWRdRr : Pseudo<(outs),
                         (ins DREGS:$src, DREGS:$src2),
                         "cpw\t$src, $src2",
                         [(AVRcmp DREGS:$src, DREGS:$src2), (implicit SREG)]>;

    let Uses = [SREG] in
    def CPCRdRr : FRdRr<0b0000,
                        0b01,
                        (outs),
                        (ins GPR8:$rd, GPR8:$rr),
                        "cpc\t$rd, $rr",
                        [(AVRcmpc GPR8:$rd, GPR8:$rr), (implicit SREG)]>;

    // CPCW Rd+1:Rd. Rr+1:Rr
    //
    // Expands to:
    // cpc Rd,   Rr
    // cpc Rd+1, Rr+1
    let Uses = [SREG] in
    def CPCWRdRr : Pseudo<(outs),
                          (ins DREGS:$src, DREGS:$src2),
                          "cpcw\t$src, $src2",
                          [(AVRcmpc DREGS:$src, DREGS:$src2), (implicit SREG)]>;

    // CPI Rd, K
    // Compares a register with an 8 bit immediate.
    let Uses = [SREG] in
    def CPIRdK : FRdK<0b0011,
                      (outs i8imm:$k),
                      (ins GPR8:$rd),
                      "cpi\t$rd, $k",
                      []>;
}

// Register conditional skipping/branching operations.
let isBranch = 1,
isTerminator = 1 in
{
    // Conditional skipping on GPR register bits, and
    // conditional skipping on IO register bits.
    let isBarrier = 1 in
    {
        def SBRCRrB : FRdB<0b10,
                           (outs),
                           (ins GPR8:$rr, i8imm:$b),
                           "sbrc\t$rr, $b",
                           []>;

        def SBRSRrB : FRdB<0b11,
                           (outs),
                           (ins GPR8:$rr, i8imm:$b),
                           "sbrs\t$rr, $b",
                           []>;

        def SBICAb : FIOBIT<0b01,
                            (outs),
                            (ins i16imm:$a, i8imm:$b),
                            "sbic\t$a, $b",
                            []>;

        def SBISAb : FIOBIT<0b11,
                            (outs),
                            (ins i16imm:$a, i8imm:$b),
                            "sbis\t$a, $b",
                            []>;
    }

    // Relative branches on status flag bits.
    let Uses = [SREG] in
    {
        // BRBS s, k
        // Branch if `s` flag in status register is set.
        def BRBSsk : FSK<0,
                         (outs),
                         (ins i8imm:$s, rel_condbr_7_target:$k),
                         "brbs\t$s, $k",
                         []>;

        // BRBC s, k
        // Branch if `s` flag in status register is clear.
        def BRBCsk : FSK<1,
                         (outs),
                         (ins i8imm:$s, rel_condbr_7_target:$k),
                         "brbc\t$s, $k",
                         []>;
    }
}


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

// PC-relative conditional branches based on status register.
// We cannot simplify these into instruction aliases because we
// also need to be able to specify a pattern to match for ISel.
let isBranch = 1,
isTerminator = 1,
Uses = [SREG] in
{
    def BREQk : FBRsk<0,
                      0b001,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "breq\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_EQ)]>;

    def BRNEk : FBRsk<1,
                      0b001,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brne\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_NE)]>;


    def BRSHk : FBRsk<1,
                      0b000,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brsh\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_SH)]>;

    def BRLOk : FBRsk<0,
                      0b000,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brlo\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_LO)]>;

    def BRMIk : FBRsk<0,
                      0b010,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brmi\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_MI)]>;

    def BRPLk : FBRsk<1,
                      0b010,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brpl\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_PL)]>;

    def BRGEk : FBRsk<1,
                      0b100,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brge\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_GE)]>;

    def BRLTk : FBRsk<0,
                      0b100,
                      (outs),
                      (ins rel_condbr_7_target:$target),
                      "brlt\t$target",
                      [(AVRbrcond bb:$target, AVR_COND_LT)]>;
}

// __________________________
// Data transfer instructions
// __________________________

// 8 and 16-bit register move instructions.
let hasSideEffects = 0 in
{
    def MOVRdRr : FRdRr<0b0010,
                        0b11,
                        (outs GPR8:$rd),
                        (ins GPR8:$rr),
                        "mov\t$rd, $rr",
                        []>;

    def MOVWRdRr : FMOVWRdRr<(outs DREGS:$dst),
                             (ins DREGS:$src),
                             "movw\t$dst, $src",
                             []>,
                   Requires<[HasMOVW]>;
}

// Load immediate values into registers.
let isReMaterializable = 1,
isAsCheapAsAMove = 1 in
{
    def LDIRdK : FRdK<0b1110,
                      (outs LD8:$rd),
                      (ins i8imm:$k),
                      "ldi\t$rd, $k",
                      [(set LD8:$rd, imm:$k)]>;

    // LDIW Rd+1:Rd, K+1:K
    //
    // Expands to:
    // ldi Rd,   K
    // ldi Rd+1, K+1
    def LDIWRdK : Pseudo<(outs DLDREGS:$dst),
                         (ins i16imm:$src),
                         "ldiw\t$dst, $src",
                         [(set DLDREGS:$dst, imm:$src)]>;
}

// Load from data space into register.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
    def LDSRdK : F32DM<0b0,
                       (outs GPR8:$rd),
                       (ins i16imm:$k),
                       "lds\t$rd, $k",
                       [(set GPR8:$rd, (load imm:$k))]>,
                 Requires<[HasSRAM]>;

    // LDSW Rd+1:Rd, K+1:K
    //
    // Expands to:
    // lds Rd,  (K+1:K)
    // lds Rd+1 (K+1:K) + 1
    def LDSWRdK : Pseudo<(outs DREGS:$dst),
                         (ins i16imm:$src),
                         "ldsw\t$dst, $src",
                         [(set DREGS:$dst, (load imm:$src))]>,
                  Requires<[HasSRAM]>;
}

// Indirect loads.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
    def LDRdPtr : FSTLD<0,
                        0b00,
                        (outs GPR8:$reg),
                        (ins LDSTPtrReg:$ptrreg),
                        "ld\t$reg, $ptrreg",
                        [(set GPR8:$reg, (load i16:$ptrreg))]>,
                  Requires<[HasSRAM]>;

    // LDW Rd+1:Rd, P
    //
    // Expands to:
    // ld Rd,   P+
    // ld Rd+1, P+
    let Constraints = "@earlyclobber $reg" in
    def LDWRdPtr : Pseudo<(outs DREGS:$reg),
                          (ins PTRDISPREGS:$ptrreg),
                          "ldw\t$reg, $ptrreg",
                          [(set DREGS:$reg, (load PTRDISPREGS:$ptrreg))]>,
                   Requires<[HasSRAM]>;
}

// Indirect loads (with postincrement or predecrement).
let mayLoad = 1,
hasSideEffects = 0,
Constraints = "$ptrreg = $base_wb,@earlyclobber $reg,@earlyclobber $base_wb" in
{
    def LDRdPtrPi : FSTLD<0,
                          0b01,
                          (outs GPR8:$reg, PTRREGS:$base_wb),
                          (ins LDSTPtrReg:$ptrreg),
                          "ld\t$reg, $ptrreg+",
                          []>,
                    Requires<[HasSRAM]>;

    // LDW Rd+1:Rd, P+
    // Expands to:
    // ld Rd,   P+
    // ld Rd+1, P+
    def LDWRdPtrPi : Pseudo<(outs DREGS:$reg, PTRREGS:$base_wb),
                            (ins PTRREGS:$ptrreg),
                            "ldw\t$reg, $ptrreg+",
                            []>,
                     Requires<[HasSRAM]>;

    def LDRdPtrPd : FSTLD<0,
                          0b10,
                          (outs GPR8:$reg, PTRREGS:$base_wb),
                          (ins LDSTPtrReg:$ptrreg),
                          "ld\t$reg, -$ptrreg",
                          []>,
                    Requires<[HasSRAM]>;

    // LDW Rd+1:Rd, -P
    //
    // Expands to:
    // ld Rd+1, -P
    // ld Rd,   -P
    def LDWRdPtrPd : Pseudo<(outs DREGS:$reg, PTRREGS:$base_wb),
                            (ins PTRREGS:$ptrreg),
                            "ldw\t$reg, -$ptrreg",
                            []>,
                     Requires<[HasSRAM]>;
}

// Load indirect with displacement operations.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
    def LDDRdPtrQ : FSTDLDD<0,
                            (outs GPR8:$reg),
                            (ins LDDSTDPtrReg:$ptr, i16imm:$imm),
                            "ldd\t$reg, $ptr+$imm",
                            [(set GPR8:$reg, (load (add addr:$ptr, imm:$imm)))]>,
                    Requires<[HasSRAM]>;

    // LDDW Rd+1:Rd, P+q
    //
    // Expands to:
    // ldd Rd,   P+q
    // ldd Rd+1, P+q+1
    let Constraints = "@earlyclobber $dst" in
    def LDDWRdPtrQ : Pseudo<(outs DREGS:$dst),
                            (ins PTRDISPREGS:$p, i16imm:$q),
                            "lddw\t$dst, $p+$q",
                            [(set DREGS:$dst, (load (add addr:$p, imm:$q)))]>,
                     Requires<[HasSRAM]>;

    //:FIXME: remove this once PR13375 gets fixed
    // Bug report: https://llvm.org/bugs/show_bug.cgi?id=13375
    let mayLoad = 1,
    hasSideEffects = 0 in
    def LDDWRdYQ : Pseudo<(outs DREGS:$dst),
                          (ins PTRDISPREGS:$p, i16imm:$q),
                          "lddw\t$dst, $p+$q",
                          []>,
                   Requires<[HasSRAM]>;
}

// Indirect store from register to data space.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "" in
{
    def STSKRr : F32DM<0b1,
                       (outs),
                       (ins i16imm:$k, GPR8:$rd),
                       "sts\t$k, $rd",
                       [(store GPR8:$rd, imm:$k)]>,
                 Requires<[HasSRAM]>;

    // STSW K+1:K, Rr+1:Rr
    //
    // Expands to:
    // sts Rr+1, (K+1:K) + 1
    // sts Rr,   (K+1:K)
    def STSWKRr : Pseudo<(outs),
                         (ins i16imm:$dst, DREGS:$src),
                         "stsw\t$dst, $src",
                         [(store DREGS:$src, imm:$dst)]>,
                  Requires<[HasSRAM]>;
}
 
// Indirect stores.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "" in
{
    // ST P, Rr
    // Stores the value of Rr into the location addressed by pointer P.
    def STPtrRr : FSTLD<1,
                        0b00,
                        (outs),
                        (ins LDSTPtrReg:$ptrreg, GPR8:$reg),
                        "st\t$ptrreg, $reg",
                        [(store GPR8:$reg, i16:$ptrreg)]>,
                  Requires<[HasSRAM]>;

    // STW P, Rr+1:Rr
    // Stores the value of Rr into the location addressed by pointer P.
    //
    // Expands to:
    // st P, Rr
    // std P+1, Rr+1
    def STWPtrRr : Pseudo<(outs),
                          (ins PTRDISPREGS:$ptrreg, DREGS:$reg),
                          "stw\t$ptrreg, $reg",
                          [(store DREGS:$reg, PTRDISPREGS:$ptrreg)]>,
                   Requires<[HasSRAM]>;
}

// Indirect stores (with postincrement or predecrement).
let Constraints = "$ptrreg = $base_wb,@earlyclobber $base_wb" in
{

    // ST P+, Rr
    // Stores the value of Rr into the location addressed by pointer P.
    // Post increments P.
    def STPtrPiRr : FSTLD<1,
                          0b01,
                          (outs LDSTPtrReg:$base_wb),
                          (ins LDSTPtrReg:$ptrreg, GPR8:$reg, i8imm:$offs),
                          "st\t$ptrreg+, $reg",
                          [(set i16:$base_wb,
                           (post_store GPR8:$reg, i16:$ptrreg, imm:$offs))]>,
                    Requires<[HasSRAM]>;

    // STW P+, Rr+1:Rr
    // Stores the value of Rr into the location addressed by pointer P.
    // Post increments P.
    //
    // Expands to:
    // st P+, Rr
    // st P+, Rr+1
    def STWPtrPiRr : Pseudo<(outs PTRREGS:$base_wb),
                            (ins PTRREGS:$ptrreg, DREGS:$trh, i8imm:$offs),
                            "stw\t$ptrreg+, $trh",
                            [(set PTRREGS:$base_wb,
                             (post_store DREGS:$trh, PTRREGS:$ptrreg, imm:$offs))]>,
                     Requires<[HasSRAM]>;

    // ST -P, Rr
    // Stores the value of Rr into the location addressed by pointer P.
    // Pre decrements P.
    def STPtrPdRr : FSTLD<1,
                          0b10,
                          (outs LDSTPtrReg:$base_wb),
                          (ins LDSTPtrReg:$ptrreg, GPR8:$reg, i8imm:$offs),
                          "st\t-$ptrreg, $reg",
                          [(set i16:$base_wb,
                           (pre_store GPR8:$reg, i16:$ptrreg, imm:$offs))]>,
                    Requires<[HasSRAM]>;

    // STW -P, Rr+1:Rr
    // Stores the value of Rr into the location addressed by pointer P.
    // Pre decrements P.
    //
    // Expands to:
    // st -P, Rr+1
    // st -P, Rr
    def STWPtrPdRr : Pseudo<(outs PTRREGS:$base_wb),
                            (ins PTRREGS:$ptrreg, DREGS:$reg, i8imm:$offs),
                            "stw\t-$ptrreg, $reg",
                            [(set PTRREGS:$base_wb,
                             (pre_store DREGS:$reg, PTRREGS:$ptrreg, imm:$offs))]>,
                     Requires<[HasSRAM]>;
}

// Store indirect with displacement operations.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "" in 
{
    // STD P+q, Rr
    // Stores the value of Rr into the location addressed by pointer P with a displacement of q.
    // Does not modify P.
    def STDPtrQRr : FSTDLDD<1,
                            (outs),
                            (ins LDDSTDPtrReg:$ptr, i16imm:$imm, GPR8:$reg),
                            "std\t$ptr+$imm, $reg",
                            [(store GPR8:$reg, (add addr:$ptr, imm:$imm))]>,
                    Requires<[HasSRAM]>;

    // STDW P+q, Rr+1:Rr
    // Stores the value of Rr into the location addressed by pointer P with a displacement of q.
    // Does not modify P.
    //
    // Expands to:
    // std P+q,   Rr
    // std P+q+1, Rr+1
    def STDWPtrQRr : Pseudo<(outs),
                            (ins PTRDISPREGS:$p, i16imm:$q, DREGS:$src),
                            "stdw\t$p+$q, $src",
                            [(store DREGS:$src, (add addr:$p, imm:$q))]>,
                     Requires<[HasSRAM]>;
}

// Load program memory operations.
let canFoldAsLoad = 1,
isReMaterializable = 1,
hasSideEffects = 0,
Uses = [R31R30] in
{
    let Defs = [R0] in
    def LPM : F16<0b1001010111001000,
                  (outs),
                  (ins),
                  "lpm",
                  []>,
              Requires<[HasLPM]>;

    def LPMRdZ : FLPMX<0,
                       0,
                       (outs GPR8:$dst),
                       (ins),
                       "lpm\t$dst, Z",
                       []>,
                 Requires<[HasLPMX]>;

    def LPMWRdZ : Pseudo<(outs DREGS:$dst),
                         (ins),
                         "lpmw\t$dst, Z",
                         []>,
                  Requires<[HasLPMX]>;

    // Load program memory, while postincrementing the Z register.
    let mayLoad = 1,
    Defs = [R31R30] in
    {
        def LPMRdZPi : FLPMX<0,
                             1,
                             (outs GPR8:$dst),
                             (ins),
                             "lpm\t$dst, Z+",
                             []>,
                       Requires<[HasLPMX]>;

        def LPMWRdZPi : Pseudo<(outs DREGS:$dst),
                               (ins),
                               "lpmw\t$dst, Z+",
                               []>,
                        Requires<[HasLPMX]>;
    }
}

// Extended load program memory operations.
let mayLoad = 1,
hasSideEffects = 0,
Uses = [R31R30] in
{
    let Defs = [R0] in
    def ELPM : F16<0b1001010111011000,
                   (outs),
                   (ins),
                   "elpm",
                   []>,
               Requires<[HasELPM]>;

    def ELPMRdZ : FLPMX<1,
                        0,
                        (outs GPR8:$dst),
                        (ins),
                        "elpm\t$dst, Z",
                        []>,
                  Requires<[HasELPMX]>;

    let Defs = [R31R30] in
    def ELPMRdZPi : FLPMX<1,
                          1,
                          (outs GPR8:$dst),
                          (ins),
                          "elpm\t$dst, Z+",
                          []>,
                    Requires<[HasELPMX]>;
}

// Store program memory operations.
// TODO: set instruction flags properly (hasSideEffects,etc)
let Uses = [R31R30, R1, R0] in
{
    def SPM : F16<0b1001010111101000,
                  (outs),
                  (ins),
                  "spm",
                  []>,
              Requires<[HasSPM]>;

    let Defs = [R31R30] in
    def SPMZPi : F16<0b1001010111111000,
                     (outs),
                     (ins),
                     "spm Z+",
                     []>,
                 Requires<[HasSPMX]>;
}

// Read data from IO location operations.
let canFoldAsLoad = 1,
isReMaterializable = 1 in
{
    def INRdA : FIORdA<(outs GPR8:$dst),
                       (ins i16imm:$src),
                       "in\t$dst, $src",
                       [(set GPR8:$dst, (load ioaddr8:$src))]>;

    def INWRdA : Pseudo<(outs DREGS:$dst),
                        (ins i16imm:$src),
                        "inw\t$dst, $src",
                        [(set DREGS:$dst, (load ioaddr16:$src))]>;
}

// Write data to IO location operations.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "" in
{
    def OUTARr : FIOARr<(outs),
                        (ins i16imm:$dst, GPR8:$src),
                        "out\t$dst, $src",
                        [(store GPR8:$src, ioaddr8:$dst)]>;

    def OUTWARr : Pseudo<(outs),
                         (ins i16imm:$dst, DREGS:$src),
                         "outw\t$dst, $src",
                         [(store DREGS:$src, ioaddr16:$dst)]>;
}

// Stack push/pop operations.
let Defs = [SP],
Uses = [SP],
hasSideEffects = 0 in
{
    // Stack push operations.
    let mayStore = 1 in
    {
        def PUSHRr : FRd<0b1001,
                         0b0011111,
                         (outs),
                         (ins GPR8:$reg),
                         "push\t$reg",
                         []>,
                     Requires<[HasSRAM]>;

        def PUSHWRr : Pseudo<(outs),
                             (ins DREGS:$reg),
                             "pushw\t$reg",
                             []>,
                      Requires<[HasSRAM]>;
    }

    // Stack pop operations.
    let mayLoad = 1 in
    {
        def POPRd : FRd<0b1001,
                        0b0001111,
                        (outs GPR8:$reg),
                        (ins),
                        "pop\t$reg",
                        []>,
                    Requires<[HasSRAM]>;

        def POPWRd : Pseudo<(outs DREGS:$reg),
                            (ins),
                            "popw\t$reg",
                            []>,
                     Requires<[HasSRAM]>;
    }
}

// Read-Write-Modify (RMW) instructions.
let Uses = [R31R30] in
{
    def XCHZRd : FZRd<0b100,
                      (outs GPR8:$rd),
                      (ins),
                      "xch\tZ, $rd",
                      []>,
                 Requires<[SupportsRMW]>;

    def LASZRd : FZRd<0b101,
                      (outs GPR8:$rd),
                      (ins),
                      "las\tZ, $rd",
                      []>,
                 Requires<[SupportsRMW]>;

    def LACZRd : FZRd<0b110,
                      (outs GPR8:$rd),
                      (ins),
                      "lac\tZ, $rd",
                      []>,
                 Requires<[SupportsRMW]>;

    def LATZRd : FZRd<0b111,
                      (outs GPR8:$rd),
                      (ins),
                      "lat\tZ, $rd",
                      []>,
                 Requires<[SupportsRMW]>;
}

// _______________________________
// Bit and bit-test instructions
// _______________________________


// Bit shift/rotate operations.
let Constraints = "$src = $rd",
Defs = [SREG] in
{
    def LSLRd : FRdRr<0b0000,
                      0b11,
                      (outs GPR8:$rd),
                      (ins GPR8:$src),
                      "lsl\t$rd",
                      [(set GPR8:$rd, (AVRlsl GPR8:$src)), (implicit SREG)]>;

    def LSLWRd : Pseudo<(outs DREGS:$rd),
                        (ins DREGS:$src),
                        "lslw\t$rd",
                        [(set DREGS:$rd, (AVRlsl DREGS:$src)), (implicit SREG)]>;

    def LSRRd : FRd<0b1001,
                    0b0100110,
                    (outs GPR8:$rd),
                    (ins GPR8:$src),
                    "lsr\t$rd",
                    [(set GPR8:$rd, (AVRlsr GPR8:$src)), (implicit SREG)]>;

    def LSRWRd : Pseudo<(outs DREGS:$rd),
                        (ins DREGS:$src),
                        "lsrw\t$rd",
                        [(set DREGS:$rd, (AVRlsr DREGS:$src)), (implicit SREG)]>;

    def ASRRd : FRd<0b1001,
                    0b0100101,
                    (outs GPR8:$rd),
                    (ins GPR8:$src),
                    "asr\t$rd",
                    [(set GPR8:$rd, (AVRasr GPR8:$src)), (implicit SREG)]>;

    def ASRWRd : Pseudo<(outs DREGS:$rd),
                        (ins DREGS:$src),
                        "asrw\t$rd",
                        [(set DREGS:$rd, (AVRasr DREGS:$src)), (implicit SREG)]>;

    // Bit rotate operations.
    let Uses = [SREG] in
    {
        def ROLRd : FRdRr<0b0001,
                          0b11,
                          (outs GPR8:$rd),
                          (ins GPR8:$src),
                          "rol\t$rd",
                          [(set GPR8:$rd, (AVRrol GPR8:$src)), (implicit SREG)]>;

        def ROLWRd : Pseudo<(outs DREGS:$rd),
                            (ins DREGS:$src),
                            "rolw\t$rd",
                            [(set DREGS:$rd, (AVRrol DREGS:$src)), (implicit SREG)]>;

        def RORRd : FRd<0b1001,
                        0b0100111,
                        (outs GPR8:$rd),
                        (ins GPR8:$src),
                        "ror\t$rd",
                        [(set GPR8:$rd, (AVRror GPR8:$src)), (implicit SREG)]>;

        def RORWRd : Pseudo<(outs DREGS:$rd),
                            (ins DREGS:$src),
                            "rorw\t$rd",
                            [(set DREGS:$rd, (AVRror DREGS:$src)), (implicit SREG)]>;
    }
}

// SWAP Rd
// Swaps the high and low nibbles in a register.
//:TODO: add 16 bit and wider version of this
let Constraints = "$src = $rd" in
def SWAPRd : FRd<0b1001,
                 0b0100010,
                 (outs GPR8:$rd),
                 (ins GPR8:$src),
                 "swap\t$rd",
                 [(set GPR8:$rd, (bswap GPR8:$src))]>;

// IO register bit set/clear operations.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "" in
{
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
}

// Status register bit load/store operations.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "" in
{

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
}

// Set/clear bit in register operations.
// We `let Constraints = ""` because it changes nothing, and we want
// to indent instructions by catagory.
let Constraints = "$src = $rd",
Defs = [SREG] in
{
    // SBR Rd, K
    // Alias for ORI Rd, K
    def SBRRdK : FRdK<0b0110,
                      (outs LD8:$rd),
                      (ins LD8:$src, i8imm:$k),
                      "sbr\t$rd, $k",
                      [(set LD8:$rd, (or LD8:$src, imm:$k)),
                       (implicit SREG)]>;

    // CBR Rd, K
    // Alias for `ANDI Rd, COM(K)` where COM(K) is the compliment of K.
    def CBRRdK : FRdK<0b0111,
                      (outs LD8:$rd),
                      (ins LD8:$src, i8imm_com:$k),
                      "cbr\t$rd, $k",
                      []>;
}

// CLR Rd
// Alias for EOR Rd, Rd
// -------------
// Clears all bits in a register.
def : InstAlias<"clr\t$rd", (EORRdRr GPR8:$rd, GPR8:$rd)>;

// SER Rd
// Alias for LDI Rd, 0xff
// ---------
// Sets all bits in a register.
def : InstAlias<"ser\t$rd", (LDIRdK LD8:$rd, 0xff), 0>;

let Defs = [SREG] in
def BSETs : FS<0,
               (outs),
               (ins i8imm:$s),
               "bset\t$s",
               []>;

let Defs = [SREG] in
def BCLRs : FS<1,
               (outs),
               (ins i8imm:$s),
               "bclr\t$s",
               []>;

// Set/clear aliases for the carry (C) status flag (bit 0).
def : InstAlias<"sec", (BSETs 0)>;
def : InstAlias<"clc", (BCLRs 0)>;

// Set/clear aliases for the zero (Z) status flag (bit 1).
def : InstAlias<"sez", (BSETs 1)>;
def : InstAlias<"clz", (BCLRs 1)>;

// Set/clear aliases for the negative (N) status flag (bit 2).
def : InstAlias<"sen", (BSETs 2)>;
def : InstAlias<"cln", (BCLRs 2)>;

// Set/clear aliases for the overflow (V) status flag (bit 3).
def : InstAlias<"sev", (BSETs 3)>;
def : InstAlias<"clv", (BCLRs 3)>;

// Set/clear aliases for the signed (S) status flag (bit 4).
def : InstAlias<"ses", (BSETs 4)>;
def : InstAlias<"cls", (BCLRs 4)>;

// Set/clear aliases for the half-carry (H) status flag (bit 5).
def : InstAlias<"seh", (BSETs 5)>;
def : InstAlias<"clh", (BCLRs 5)>;

// Set/clear aliases for the T status flag (bit 6).
def : InstAlias<"set", (BSETs 6)>;
def : InstAlias<"clt", (BCLRs 6)>;

// Set/clear aliases for the interrupt (I) status flag (bit 7).
def : InstAlias<"sei", (BSETs 7)>;
def : InstAlias<"cli", (BCLRs 7)>;

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
                []>,
            Requires<[HasBREAK]>;

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

// Sign extension operations.
let Defs = [SREG] in
{
    //:TODO: Optimize this for wider types AND optimize the following code
    //       compile int foo(char a, char b, char c, char d) {return d+b;}
    //       looks like a missed sext_inreg opportunity.
    def SEXT : Pseudo<(outs DREGS:$dst),
                      (ins GPR8:$src),
                      "sext\t$dst, $src",
                      [(set DREGS:$dst, (sext GPR8:$src)), (implicit SREG)]>;

    def ZEXT : Pseudo<(outs DREGS:$dst),
                      (ins GPR8:$src),
                      "zext\t$dst, $src",
                      [(set DREGS:$dst, (zext GPR8:$src)), (implicit SREG)]>;
}

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
