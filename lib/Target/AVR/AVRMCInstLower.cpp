//===-- AVRMCInstLower.cpp - Convert AVR MachineInstr to an MCInst --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower AVR MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "AVRMCInstLower.h"
#include "AVRInstrInfo.h"
#include "MCTargetDesc/AVRMCExpr.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/IR/Mangler.h"

using namespace llvm;

MCOperand AVRMCInstLower::
LowerSymbolOperand(const MachineOperand &MO, MCSymbol *Sym) const
{
  // FIXME: We would like an efficient form for this, so we don't have to do a
  // lot of extra uniquing.
  unsigned char TF = MO.getTargetFlags();
  const MCExpr *Expr = MCSymbolRefExpr::Create(Sym, Ctx);

  if (TF & AVRII::MO_NEG)
  {
    Expr = MCUnaryExpr::CreateMinus(Expr, Ctx);
  }

  if (!MO.isJTI() && MO.getOffset())
  {
    Expr = MCBinaryExpr::CreateAdd(Expr, MCConstantExpr::Create(MO.getOffset(),
                                                                Ctx), Ctx);
  }

  if (TF & AVRII::MO_LO)
  {
    Expr = AVRMCExpr::CreateLower8(Expr, Ctx);
  }
  else if (TF & AVRII::MO_HI)
  {
    Expr = AVRMCExpr::CreateUpper8(Expr, Ctx);
  }
  else if (TF != 0)
  {
    llvm_unreachable("Unknown target flag on symbol operand");
  }

  return MCOperand::CreateExpr(Expr);
}

void AVRMCInstLower::Lower(const MachineInstr *MI, MCInst &OutMI) const
{
  OutMI.setOpcode(MI->getOpcode());

  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i)
  {
    MCOperand MCOp;
    const MachineOperand &MO = MI->getOperand(i);

    switch (MO.getType())
    {
    default:
      MI->dump();
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_Register:
      // Ignore all implicit register operands.
      if (MO.isImplicit()) continue;
      MCOp = MCOperand::CreateReg(MO.getReg());
      break;
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::CreateImm(MO.getImm());
      break;
    case MachineOperand::MO_GlobalAddress:
      // TODO: Cleanup 3.4
      MCOp = LowerSymbolOperand(MO, Printer.getSymbol(MO.getGlobal()));
      //const GlobalValue *GV = MO.getGlobal();
      
      //MCOp = LowerSymbolOperand(MO, GetSymbolFromOperand(MO));
      break;
    case MachineOperand::MO_ExternalSymbol:
      MCOp = LowerSymbolOperand(MO,
               Printer.GetExternalSymbolSymbol(MO.getSymbolName()));
      break;
    case MachineOperand::MO_MachineBasicBlock:
      MCOp = MCOperand::CreateExpr(MCSymbolRefExpr::Create(
        MO.getMBB()->getSymbol(), Ctx));
      break;
    case MachineOperand::MO_RegisterMask:
      continue;
    case MachineOperand::MO_BlockAddress:
      MCOp = LowerSymbolOperand(MO,
               Printer.GetBlockAddressSymbol(MO.getBlockAddress()));
      break;
      //:FIXME: readd this when needed
      /*
      case MachineOperand::MO_JumpTableIndex:
        MCOp = LowerSymbolOperand(MO, AsmPrinter.GetJTISymbol(MO.getIndex()));
        break;
      case MachineOperand::MO_ConstantPoolIndex:
        MCOp = LowerSymbolOperand(MO, AsmPrinter.GetCPISymbol(MO.getIndex()));
        break;*/
    }

    OutMI.addOperand(MCOp);
  }
}
