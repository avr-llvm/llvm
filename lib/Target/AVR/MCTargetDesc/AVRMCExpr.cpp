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
AVRMCExpr::create(VariantKind Kind, const MCExpr *Expr, MCContext &Ctx)
{
  return new (Ctx) AVRMCExpr(Kind, Expr);
}

void AVRMCExpr::printImpl(raw_ostream &OS) const
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
AVRMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                     const MCAsmLayout *Layout,
                                     const MCFixup *Fixup) const
{
  return Expr->evaluateAsRelocatable(Res, Layout, Fixup);
}

void AVRMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

