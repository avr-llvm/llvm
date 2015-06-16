//===-- AVRInstPrinter.cpp - Convert AVR MCInst to assembly syntax --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class prints an AVR MCInst to a .s file.
//
//===----------------------------------------------------------------------===//


#include "AVRInstPrinter.h"

#include <cstring>

#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"

#include "MCTargetDesc/AVRMCTargetDesc.h"

#define DEBUG_TYPE "asm-printer"

namespace llvm {

// Include the auto-generated portion of the assembly writer.
#define PRINT_ALIAS_INSTR
#include "AVRGenAsmWriter.inc"

void AVRInstPrinter::printInst(const MCInst *MI, raw_ostream &O,
                               StringRef Annot, const MCSubtargetInfo &STI)
{
  unsigned Opcode = MI->getOpcode();

  // First handle load and store instructions with postinc or predec
  // of the form "ld reg, X+".
  // TODO: is this necessary anymore now that we encode this into AVRInstrInfo.td
  switch (Opcode)
  {
  case AVR::LDRdPtr:
  case AVR::LDRdPtrPi:
  case AVR::LDRdPtrPd:
    O << "\tld\t";
    printOperand(MI, 0, O);
    O << ", ";
    if (Opcode == AVR::LDRdPtrPd) O << '-';
    printOperand(MI, 1, O);
    if (Opcode == AVR::LDRdPtrPi) O << '+';
    return;
  case AVR::STPtrRr:
    O << "\tst\t";
    printOperand(MI, 0, O);
    O << ", ";
    printOperand(MI, 1, O);
    return;
  case AVR::STPtrPiRr:
  case AVR::STPtrPdRr:
    O << "\tst\t";
    if (Opcode == AVR::STPtrPdRr) O << '-';
    printOperand(MI, 1, O);
    if (Opcode == AVR::STPtrPiRr) O << '+';
    O << ", ";
    printOperand(MI, 2, O);
    return;
  }

  if(!printAliasInstr(MI, O))
    printInstruction(MI, O);

  printAnnotation(O, Annot);
}

const char *
AVRInstPrinter::getPrettyRegisterName(unsigned RegNo, MCRegisterInfo const& MRI) {
#ifdef LLVM_AVR_GCC_COMPAT
  if (AVRMCRegisterClasses[AVR::DREGSRegClassID].contains(RegNo) &&
      !AVRMCRegisterClasses[AVR::PTRREGSRegClassID].contains(RegNo))
  {
    return getRegisterName(MRI.getSubReg(RegNo, AVR::sub_lo));
  }
#endif
  return getRegisterName(RegNo);
}


void AVRInstPrinter::printOperand(const MCInst *MI, unsigned OpNo,
                                  raw_ostream &O)
{
  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isReg()) {
    bool isDREGS = false;

    if(isDREGS) {

    } else {
        O << getPrettyRegisterName(Op.getReg(), MRI);
    }
  } else if (Op.isImm()) {
    O << Op.getImm();
  } else {
    assert(Op.isExpr() && "Unknown operand kind in printOperand");
    O << *Op.getExpr();
  }
}


/// print_pcrel_imm - This is used to print an immediate value that ends up
/// being encoded as a pc-relative value.
void AVRInstPrinter::print_pcrel_imm(const MCInst *MI, unsigned OpNo,
                                     raw_ostream &O)
{
  const MCOperand &Op = MI->getOperand(OpNo);

  if (Op.isImm())
  {
    int64_t Imm = Op.getImm();
    O << '.';

    // Print a position sign if needed.
    // Negative values have their sign printed automatically.
    if(Imm >= 0)
      O << '+';

    O << Imm;
    return;
  }

  assert(Op.isExpr() && "Unknown pcrel immediate operand");
  O << *Op.getExpr();
}

} // end of namespace llvm
