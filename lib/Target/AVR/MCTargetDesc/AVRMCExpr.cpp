//===-- AVRMCExpr.cpp - AVR specific MC expression classes ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AVRMCExpr.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"

using namespace llvm;

const AVRMCExpr *
AVRMCExpr::Create(VariantKind Kind, const MCExpr *Expr, MCContext &Ctx)
{
  return new (Ctx) AVRMCExpr(Kind, Expr);
}

void AVRMCExpr::PrintImpl(raw_ostream &OS) const
{
  switch (Kind)
  {
  default:                      llvm_unreachable("Invalid kind!");
  case VK_AVR_HI8:              OS << "hi8("; break;
  case VK_AVR_LO8:              OS << "lo8("; break;
  }

  const MCExpr *Expr = getSubExpr();
  if (Expr->getKind() != MCExpr::SymbolRef) OS << '(';
  Expr->print(OS);
  if (Expr->getKind() != MCExpr::SymbolRef) OS << ')';

  OS << ')';
}

bool
AVRMCExpr::EvaluateAsRelocatableImpl(MCValue &Res,
                                     const MCAsmLayout *Layout,
                                     const MCFixup *Fixup) const
{
  return Expr->EvaluateAsRelocatable(Res, Layout, Fixup);
}

void AVRMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

static void addValueSymbols_(const MCExpr *Value, MCAssembler *Asm)
{
  switch (Value->getKind())
  {
  case MCExpr::Target:
    llvm_unreachable("Can't handle nested target expr!");
  case MCExpr::Constant:
    break;
  case MCExpr::Binary:
    {
      const MCBinaryExpr *BE = cast<MCBinaryExpr>(Value);
      addValueSymbols_(BE->getLHS(), Asm);
      addValueSymbols_(BE->getRHS(), Asm);
      break;
    }
  case MCExpr::SymbolRef:
    Asm->getOrCreateSymbolData(cast<MCSymbolRefExpr>(Value)->getSymbol());
    break;
  case MCExpr::Unary:
    addValueSymbols_(cast<MCUnaryExpr>(Value)->getSubExpr(), Asm);
    break;
  }
}

void AVRMCExpr::AddValueSymbols(MCAssembler *Asm) const
{
  addValueSymbols_(getSubExpr(), Asm);
}
