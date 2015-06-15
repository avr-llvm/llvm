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
#include "llvm/MC/MCValue.h"
#include "llvm/MC/MCAsmLayout.h"

namespace llvm {

namespace {

const struct ModifierEntry {
  char const* Spelling;
  AVRMCExpr::VariantKind VariantKind;
} ModifierNames[] = {
  { "lo8",     AVRMCExpr::VK_AVR_LO8     },
  { "hi8",     AVRMCExpr::VK_AVR_HI8     },
  { "hh8",     AVRMCExpr::VK_AVR_HH8    }, // synonym with hlo8
  { "hlo8",    AVRMCExpr::VK_AVR_HH8    },
  { "hhi8",    AVRMCExpr::VK_AVR_HHI8    },

  { "pm_lo8",  AVRMCExpr::VK_AVR_PM_LO8  },
  { "pm_hi8",  AVRMCExpr::VK_AVR_PM_HI8  },
  { "pm_hh8",  AVRMCExpr::VK_AVR_PM_HH8 },
};

} // end of anonymous namespace

const AVRMCExpr *
AVRMCExpr::create(VariantKind Kind, const MCExpr *Expr, MCContext &Ctx) {
  return new (Ctx) AVRMCExpr(Kind, Expr);
}

void
AVRMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  assert(Kind != VK_AVR_None);
  OS << getName() << "("; getSubExpr()->print(OS, MAI); OS << ')';
}

bool
AVRMCExpr::evaluateAsConstant(int64_t & Result) const {
  MCValue Value;

  if (!getSubExpr()->evaluateAsRelocatable(Value, nullptr, nullptr))
    return false;

  if (!Value.isAbsolute())
    return false;

  Result = evaluateAsInt64(Value.getConstant());
  return true;
}

bool
AVRMCExpr::evaluateAsRelocatableImpl(MCValue &Result,
                                     const MCAsmLayout *Layout,
                                     const MCFixup *Fixup) const
{
  MCValue Value;
  if (!getSubExpr()->evaluateAsRelocatable(Value, Layout, Fixup))
    return false;

  if (Value.isAbsolute()) {
    Result = MCValue::get(evaluateAsInt64(Value.getConstant()));
  } else {

    if (!getSubExpr()->evaluateAsRelocatable(Value, Layout, Fixup))
      return false;

    if (!Layout)
      return false;

    MCContext &Context = Layout->getAssembler().getContext();
    const MCSymbolRefExpr *Sym = Value.getSymA();
    MCSymbolRefExpr::VariantKind Modifier = Sym->getKind();
    if (Modifier != MCSymbolRefExpr::VK_None)
      return false;
#if 0
    switch (Kind) {
      case VK_AVR_LO8: Modifier = MCSymbolRefExpr::VK_AVR_LO; break;
      case VK_AVR_HI8: Modifier = MCSymbolRefExpr::VK_AVR_HI; break;
    }
#endif
    Sym = MCSymbolRefExpr::create(&Sym->getSymbol(), Modifier, Context);
    Result = MCValue::get(Sym, Value.getSymB(), Value.getConstant());
  }
  return true;
}

int64_t
AVRMCExpr::evaluateAsInt64(int64_t Value) const {
  uint64_t v = static_cast<uint64_t>(Value);
  switch (Kind) {
    case AVRMCExpr::VK_AVR_LO8:              break;
    case AVRMCExpr::VK_AVR_HI8:    v >>= 8;  break;
    case AVRMCExpr::VK_AVR_HH8:    v >>= 16; break;
    case AVRMCExpr::VK_AVR_HHI8:   v >>= 24; break;
    case AVRMCExpr::VK_AVR_PM_LO8: v >>= 1;  break;
    case AVRMCExpr::VK_AVR_PM_HI8: v >>= 9;  break;
    case AVRMCExpr::VK_AVR_PM_HH8: v >>= 17; break;

    case AVRMCExpr::VK_AVR_None: llvm_unreachable("Uninitialized expression.");
  }
  return v & 0xff;
}

AVR::Fixups
AVRMCExpr::getFixupKind() const {
  switch (getKind()) {
    case VK_AVR_LO8:    return AVR::fixup_lo8_ldi;
    case VK_AVR_HI8:    return AVR::fixup_hi8_ldi;
    case VK_AVR_HH8:    return AVR::fixup_hh8_ldi;
    case VK_AVR_HHI8:   return AVR::fixup_ms8_ldi;

    case VK_AVR_PM_LO8: return AVR::fixup_lo8_ldi_pm;
    case VK_AVR_PM_HI8: return AVR::fixup_hi8_ldi_pm;
    case VK_AVR_PM_HH8: return AVR::fixup_hh8_ldi_pm;

    case VK_AVR_None: llvm_unreachable("Uninitialized expression");
  }
}

void
AVRMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

char const*
AVRMCExpr::getName() const {
  const auto & Modifier =
  std::find_if(std::begin(ModifierNames), std::end(ModifierNames),
               [this](ModifierEntry const& Mod) {
                 return Mod.VariantKind == Kind;
               });
  if (Modifier != std::end(ModifierNames)) {
    return Modifier->Spelling;
  }
  return 0;
}

AVRMCExpr::VariantKind
AVRMCExpr::getKindByName(StringRef Name) {
  const auto & Modifier =
  std::find_if(std::begin(ModifierNames), std::end(ModifierNames),
               [&Name](ModifierEntry const& Mod) {
                 return Mod.Spelling == Name;
               });
  if (Modifier != std::end(ModifierNames)) {
    return Modifier->VariantKind;
  }
  return VK_AVR_None;
}

} // end of namespace llvm
