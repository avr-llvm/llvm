//===-- AVRMCExpr.h - AVR specific MC expression classes --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef __INCLUDE_AVRMCEXPR_H__
#define __INCLUDE_AVRMCEXPR_H__

#include "llvm/MC/MCExpr.h"

namespace llvm
{

class AVRMCExpr : public MCTargetExpr
{
public:
  enum VariantKind
  {
    VK_AVR_None,
    VK_AVR_HI8,         // hi8 macro in the .s file
    VK_AVR_LO8          // lo8 macro in the .s file
  };
public:
  /// @name Construction
  /// @{

  static const AVRMCExpr *Create(VariantKind Kind, const MCExpr *Expr,
                                 MCContext &Ctx);
  static const AVRMCExpr *CreateUpper8(const MCExpr *Expr, MCContext &Ctx)
  {
    return Create(VK_AVR_HI8, Expr, Ctx);
  }
  static const AVRMCExpr *CreateLower8(const MCExpr *Expr, MCContext &Ctx)
  {
    return Create(VK_AVR_LO8, Expr, Ctx);
  }

  /// @}
  /// @name Accessors
  /// @{

  /// getOpcode - Get the kind of this expression.
  VariantKind getKind() const { return Kind; }

  /// getSubExpr - Get the child of this expression.
  const MCExpr *getSubExpr() const { return Expr; }

  /// @}
public: // MCTargetExpr
  void PrintImpl(raw_ostream &OS) const;
  bool EvaluateAsRelocatableImpl(MCValue &Res,
                                 const MCAsmLayout *Layout,
                                 const MCFixup *Fixup) const override;
  
  void visitUsedExpr(MCStreamer& streamer) const override;
  
  void AddValueSymbols(MCAssembler *) const;
  const MCSection *FindAssociatedSection() const
  {
    return getSubExpr()->FindAssociatedSection();
  }

  // There are no TLS AVRMCExprs at the moment.
  void fixELFSymbolsInTLSFixups(MCAssembler &Asm) const {}

  static bool classof(const MCExpr *E)
  {
    return E->getKind() == MCExpr::Target;
  }
private:
  const VariantKind Kind;
  const MCExpr *Expr;
private:
  explicit AVRMCExpr(VariantKind _Kind, const MCExpr *_Expr) :
    Kind(_Kind), Expr(_Expr) {}
  ~AVRMCExpr() {}
};

} // end namespace llvm

#endif //__INCLUDE_AVRMCEXPR_H__
