//===-- AVRMCInstLower.h - Lower MachineInstr to MCInst ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRMCINSTLOWER_H__
#define __INCLUDE_AVRMCINSTLOWER_H__

#include "llvm/Support/Compiler.h"

namespace llvm
{

class AsmPrinter;
class MachineInstr;
class MachineOperand;
class MCContext;
class MCInst;
class MCOperand;
class MCSymbol;

class LLVM_LIBRARY_VISIBILITY AVRMCInstLower
{
public:
  AVRMCInstLower(MCContext &ctx, AsmPrinter &printer) :
    Ctx(ctx), Printer(printer) {}

  void Lower(const MachineInstr *MI, MCInst &OutMI) const;
  MCOperand LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const;
private:
  MCContext &Ctx;
  AsmPrinter &Printer;
};

} // end namespace llvm

#endif //__INCLUDE_AVRMCINSTLOWER_H__
