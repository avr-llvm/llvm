//===---- AVRAsmParser.cpp - Parse AVR assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "MCTargetDesc/AVRMCExpr.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

namespace {
class AVROperand;
class AVRAsmParser : public MCTargetAsmParser {

  MCSubtargetInfo &STI;
  MCAsmParser &Parser;

  /// @name Auto-generated Match Functions
  /// {

#define GET_ASSEMBLER_HEADER
#include "AVRGenAsmMatcher.inc"

  /// }

public:
  AVRAsmParser(MCSubtargetInfo &sti, MCAsmParser &parser,
                const MCInstrInfo &MII,
                const MCTargetOptions &Options)
      : MCTargetAsmParser(), STI(sti), Parser(parser) {
    // Initialize the set of available features.
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

};


/// AVROperand - Instances of this class represent a parsed Sparc machine
/// instruction.
class AVROperand : public MCParsedAsmOperand {
public:

private:
  enum KindTy {
    k_Token,
    k_Register,
    k_Immediate,
    k_MemoryReg,
    k_MemoryImm
  } Kind;

  SMLoc StartLoc, EndLoc;

  struct Token {
    const char *Data;
    unsigned Length;
  };

  struct RegOp {
    unsigned RegNum;
    RegisterKind Kind;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  struct MemOp {
    unsigned Base;
    unsigned OffsetReg;
    const MCExpr *Off;
  };

  union {
    struct Token Tok;
    struct RegOp Reg;
    struct ImmOp Imm;
    struct MemOp Mem;
  };
public:
  AVROperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

  bool isToken() const override { return Kind == k_Token; }
  bool isReg() const override { return Kind == k_Register; }
  bool isImm() const override { return Kind == k_Immediate; }
  bool isMem() const override { return Kind == k_MemoryReg || Kind == k_MemoryImm }

  StringRef getToken() const {
    assert(Kind == k_Token && "Invalid access!");
    return StringRef(Tok.Data, Tok.Length);
  }

  unsigned getReg() const override {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg.RegNum;
  }

  const MCExpr *getImm() const {
    assert((Kind == k_Immediate) && "Invalid access!");
    return Imm.Val;
  }

  unsigned getMemBase() const {
    assert((Kind == k_MemoryReg || Kind == k_MemoryImm) && "Invalid access!");
    return Mem.Base;
  }

  unsigned getMemOffsetReg() const {
    assert((Kind == k_MemoryReg) && "Invalid access!");
    return Mem.OffsetReg;
  }

  const MCExpr *getMemOff() const {
    assert((Kind == k_MemoryImm) && "Invalid access!");
    return Mem.Off;
  }

  /// getStartLoc - Get the location of the first token of this operand.
  SMLoc getStartLoc() const override {
    return StartLoc;
  }
  /// getEndLoc - Get the location of the last token of this operand.
  SMLoc getEndLoc() const override {
    return EndLoc;
  }

  void print(raw_ostream &OS) const override {
    switch (Kind) {
    case k_Token:     OS << "Token: " << getToken() << "\n"; break;
    case k_Register:  OS << "Reg: #" << getReg() << "\n"; break;
    case k_Immediate: OS << "Imm: " << getImm() << "\n"; break;
    case k_MemoryReg: OS << "Mem: " << getMemBase() << "+"
                         << getMemOffsetReg() << "\n"; break;
    case k_MemoryImm: assert(getMemOff() != nullptr);
      OS << "Mem: " << getMemBase()
         << "+" << *getMemOff()
         << "\n"; break;
    }
  }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCExpr *Expr = getImm();
    addExpr(Inst, Expr);
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const{
    // Add as immediate when possible.  Null MCExpr = 0.
    if (!Expr)
      Inst.addOperand(MCOperand::CreateImm(0));
    else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::CreateImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::CreateExpr(Expr));
  }

  void addMEMrrOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");

    Inst.addOperand(MCOperand::CreateReg(getMemBase()));

    assert(getMemOffsetReg() != 0 && "Invalid offset");
    Inst.addOperand(MCOperand::CreateReg(getMemOffsetReg()));
  }

  void addMEMriOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");

    Inst.addOperand(MCOperand::CreateReg(getMemBase()));

    const MCExpr *Expr = getMemOff();
    addExpr(Inst, Expr);
  }

  static std::unique_ptr<AVROperand> CreateToken(StringRef Str, SMLoc S) {
    auto Op = make_unique<AVROperand>(k_Token);
    Op->Tok.Data = Str.data();
    Op->Tok.Length = Str.size();
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static std::unique_ptr<AVROperand> CreateReg(unsigned RegNum, unsigned Kind,
                                                 SMLoc S, SMLoc E) {
    auto Op = make_unique<AVROperand>(k_Register);
    Op->Reg.RegNum = RegNum;
    Op->Reg.Kind   = (AVROperand::RegisterKind)Kind;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<AVROperand> CreateImm(const MCExpr *Val, SMLoc S,
                                                 SMLoc E) {
    auto Op = make_unique<AVROperand>(k_Immediate);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<AVROperand>
  MorphToMEMrr(unsigned Base, std::unique_ptr<AVROperand> Op) {
    unsigned offsetReg = Op->getReg();
    Op->Kind = k_MemoryReg;
    Op->Mem.Base = Base;
    Op->Mem.OffsetReg = offsetReg;
    Op->Mem.Off = nullptr;
    return Op;
  }

  static std::unique_ptr<AVROperand>
  CreateMEMri(unsigned Base, const MCExpr *Off, SMLoc S, SMLoc E) {
    auto Op = make_unique<AVROperand>(k_MemoryImm);
    Op->Mem.Base = Base;
    Op->Mem.OffsetReg = 0;
    Op->Mem.Off = Off;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<AVROperand>
  MorphToMEMri(unsigned Base, std::unique_ptr<AVROperand> Op) {
    const MCExpr *Imm  = Op->getImm();
    Op->Kind = k_MemoryImm;
    Op->Mem.Base = Base;
    Op->Mem.OffsetReg = 0;
    Op->Mem.Off = Imm;
    return Op;
  }
};

} // end namespace

bool AVRAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             uint64_t &ErrorInfo,
                                             bool MatchingInlineAsm) {
  /*MCInst Inst;
  SmallVector<MCInst, 8> Instructions;
  unsigned MatchResult = MatchInstructionImpl(Operands, Inst, ErrorInfo,
                                              MatchingInlineAsm);
  switch (MatchResult) {
  default:
    break;

  case Match_Success: {
    Inst.setLoc(IDLoc);
    Out.EmitInstruction(Inst, STI);
    return false;
  }

  case Match_MissingFeature:
    return Error(IDLoc,
                 "instruction requires a CPU feature not currently enabled");

  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0ULL) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "too few operands for instruction");

      ErrorLoc = ((AVROperand &)*Operands[ErrorInfo]).getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }

    return Error(ErrorLoc, "invalid operand for instruction");
  }
  case Match_MnemonicFail:
    return Error(IDLoc, "invalid instruction mnemonic");
  }
  return true;*/
  llvm_unreachable("unimplemented");
  return false;
}

bool AVRAsmParser::
ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc)
{
  const AsmToken &Tok = Parser.getTok();
  StartLoc = Tok.getLoc();
  EndLoc = Tok.getEndLoc();
  RegNo = 0;
  if (getLexer().getKind() != AsmToken::Percent)
    return false;
  Parser.Lex();
  unsigned regKind = AVROperand::rk_None;
  if (matchRegisterName(Tok, RegNo, regKind)) {
    Parser.Lex();
    return false;
  }

  return Error(StartLoc, "invalid register name");
}

bool AVRAsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                      StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) {

  // First operand in MCInst is instruction mnemonic.
  Operands.push_back(AVROperand::CreateToken(Name, NameLoc));

  // apply mnemonic aliases, if any, so that we can parse operands correctly.
  applyMnemonicAliases(Name, getAvailableFeatures(), 0);

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (parseOperand(Operands, Name) != MatchOperand_Success) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token");
    }

    while (getLexer().is(AsmToken::Comma)) {
      Parser.Lex(); // Eat the comma.
      // Parse and remember the operand.
      if (parseOperand(Operands, Name) != MatchOperand_Success) {
        SMLoc Loc = getLexer().getLoc();
        Parser.eatToEndOfStatement();
        return Error(Loc, "unexpected token");
      }
    }
  }
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = getLexer().getLoc();
    Parser.eatToEndOfStatement();
    return Error(Loc, "unexpected token");
  }
  Parser.Lex(); // Consume the EndOfStatement.
  return false;
}

bool AVRAsmParser::
ParseDirective(AsmToken DirectiveID)
{
  StringRef IDVal = DirectiveID.getString();

  if (IDVal == ".byte")
    return parseDirectiveWord(1, DirectiveID.getLoc());

  // Let the MC layer to handle other directives.
  return true;
}

bool AVRAsmParser:: parseDirectiveWord(unsigned Size, SMLoc L) {
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    for (;;) {
      const MCExpr *Value;
      if (getParser().parseExpression(Value))
        return true;

      getParser().getStreamer().EmitValue(Value, Size);

      if (getLexer().is(AsmToken::EndOfStatement))
        break;

      // FIXME: Improve diagnostic.
      if (getLexer().isNot(AsmToken::Comma))
        return Error(L, "unexpected token in directive");
      Parser.Lex();
    }
  }
  Parser.Lex();
  return false;
}

AVRAsmParser::OperandMatchResultTy
AVRAsmParser::parseMEMOperand(OperandVector &Operands) {

  SMLoc S, E;
  unsigned BaseReg = 0;

  if (ParseRegister(BaseReg, S, E)) {
    return MatchOperand_NoMatch;
  }

  switch (getLexer().getKind()) {
  default: return MatchOperand_NoMatch;

  case AsmToken::Comma:
  case AsmToken::RBrac:
  case AsmToken::EndOfStatement:
    Operands.push_back(AVROperand::CreateMEMri(BaseReg, nullptr, S, E));
    return MatchOperand_Success;

  case AsmToken:: Plus:
    Parser.Lex(); // Eat the '+'
    break;
  case AsmToken::Minus:
    break;
  }

  std::unique_ptr<AVROperand> Offset;
  OperandMatchResultTy ResTy = parseAVRAsmOperand(Offset);
  if (ResTy != MatchOperand_Success || !Offset)
    return MatchOperand_NoMatch;

  Operands.push_back(
      Offset->isImm() ? AVROperand::MorphToMEMri(BaseReg, std::move(Offset))
                      : AVROperand::MorphToMEMrr(BaseReg, std::move(Offset)));

  return MatchOperand_Success;
}

AVRAsmParser::OperandMatchResultTy
AVRAsmParser::parseOperand(OperandVector &Operands, StringRef Mnemonic) {

  OperandMatchResultTy ResTy = MatchOperandParserImpl(Operands, Mnemonic);

  // If there wasn't a custom match, try the generic matcher below. Otherwise,
  // there was a match, but an error occurred, in which case, just return that
  // the operand parsing failed.
  if (ResTy == MatchOperand_Success || ResTy == MatchOperand_ParseFail)
    return ResTy;

  if (getLexer().is(AsmToken::LBrac)) {
    // Memory operand
    Operands.push_back(AVROperand::CreateToken("[",
                                                 Parser.getTok().getLoc()));
    Parser.Lex(); // Eat the [

    if (Mnemonic == "cas" || Mnemonic == "casx") {
      SMLoc S = Parser.getTok().getLoc();
      if (getLexer().getKind() != AsmToken::Percent)
        return MatchOperand_NoMatch;
      Parser.Lex(); // eat %

      unsigned RegNo, RegKind;
      if (!matchRegisterName(Parser.getTok(), RegNo, RegKind))
        return MatchOperand_NoMatch;

      Parser.Lex(); // Eat the identifier token.
      SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer()-1);
      Operands.push_back(AVROperand::CreateReg(RegNo, RegKind, S, E));
      ResTy = MatchOperand_Success;
    } else {
      ResTy = parseMEMOperand(Operands);
    }

    if (ResTy != MatchOperand_Success)
      return ResTy;

    if (!getLexer().is(AsmToken::RBrac))
      return MatchOperand_ParseFail;

    Operands.push_back(AVROperand::CreateToken("]",
                                                 Parser.getTok().getLoc()));
    Parser.Lex(); // Eat the ]
    return MatchOperand_Success;
  }

  std::unique_ptr<AVROperand> Op;

  ResTy = parseSparcAsmOperand(Op, (Mnemonic == "call"));
  if (ResTy != MatchOperand_Success || !Op)
    return MatchOperand_ParseFail;

  // Push the parsed operand into the list of operands
  Operands.push_back(std::move(Op));

  return MatchOperand_Success;
}

AVRAsmParser::OperandMatchResultTy
AVRAsmParser::parseAVRAsmOperand(std::unique_ptr<AVROperand> &Op,
                                     bool isCall) {

  SMLoc S = Parser.getTok().getLoc();
  SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  const MCExpr *EVal;

  Op = nullptr;
  switch (getLexer().getKind()) {
  default:  break;

  case AsmToken::Percent:
    Parser.Lex(); // Eat the '%'.
    unsigned RegNo;
    unsigned RegKind;
    if (matchRegisterName(Parser.getTok(), RegNo, RegKind)) {
      StringRef name = Parser.getTok().getString();
      Parser.Lex(); // Eat the identifier token.
      E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
      switch (RegNo) {
      default:
        Op = AVROperand::CreateReg(RegNo, RegKind, S, E);
        break;
      case Sparc::Y:
        Op = AVROperand::CreateToken("%y", S);
        break;

      case Sparc::ICC:
        if (name == "xcc")
          Op = AVROperand::CreateToken("%xcc", S);
        else
          Op = AVROperand::CreateToken("%icc", S);
        break;
      }
      break;
    }
    if (matchSparcAsmModifiers(EVal, E)) {
      E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
      Op = AVROperand::CreateImm(EVal, S, E);
    }
    break;

  case AsmToken::Minus:
  case AsmToken::Integer:
    if (!getParser().parseExpression(EVal, E))
      Op = AVROperand::CreateImm(EVal, S, E);
    break;

  case AsmToken::Identifier: {
    StringRef Identifier;
    if (!getParser().parseIdentifier(Identifier)) {
      E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
      MCSymbol *Sym = getContext().GetOrCreateSymbol(Identifier);

      const MCExpr *Res = MCSymbolRefExpr::Create(Sym, MCSymbolRefExpr::VK_None,
                                                  getContext());
      if (isCall &&
          getContext().getObjectFileInfo()->getRelocM() == Reloc::PIC_)
        Res = SparcMCExpr::Create(SparcMCExpr::VK_Sparc_WPLT30, Res,
                                  getContext());
      Op = AVROperand::CreateImm(Res, S, E);
    }
    break;
  }
  }
  return (Op) ? MatchOperand_Success : MatchOperand_ParseFail;
}

bool AVRAsmParser::matchRegisterName(const AsmToken &Tok,
                                       unsigned &RegNo,
                                       unsigned &RegKind)
{
  int64_t intVal = 0;
  RegNo = 0;
  RegKind = AVROperand::rk_None;
  if (Tok.is(AsmToken::Identifier)) {
    StringRef name = Tok.getString();

    // %fp
    if (name.equals("fp")) {
      RegNo = Sparc::I6;
      RegKind = AVROperand::rk_IntReg;
      return true;
    }
    // %sp
    if (name.equals("sp")) {
      RegNo = Sparc::O6;
      RegKind = AVROperand::rk_IntReg;
      return true;
    }

    if (name.equals("y")) {
      RegNo = Sparc::Y;
      RegKind = AVROperand::rk_Y;
      return true;
    }

    if (name.equals("icc")) {
      RegNo = Sparc::ICC;
      RegKind = AVROperand::rk_CCReg;
      return true;
    }

    if (name.equals("xcc")) {
      // FIXME:: check 64bit.
      RegNo = Sparc::ICC;
      RegKind = AVROperand::rk_CCReg;
      return true;
    }

    // %fcc0 - %fcc3
    if (name.substr(0, 3).equals_lower("fcc")
        && !name.substr(3).getAsInteger(10, intVal)
        && intVal < 4) {
      // FIXME: check 64bit and  handle %fcc1 - %fcc3
      RegNo = Sparc::FCC0 + intVal;
      RegKind = AVROperand::rk_CCReg;
      return true;
    }

    // %g0 - %g7
    if (name.substr(0, 1).equals_lower("g")
        && !name.substr(1).getAsInteger(10, intVal)
        && intVal < 8) {
      RegNo = IntRegs[intVal];
      RegKind = AVROperand::rk_IntReg;
      return true;
    }
    // %o0 - %o7
    if (name.substr(0, 1).equals_lower("o")
        && !name.substr(1).getAsInteger(10, intVal)
        && intVal < 8) {
      RegNo = IntRegs[8 + intVal];
      RegKind = AVROperand::rk_IntReg;
      return true;
    }
    if (name.substr(0, 1).equals_lower("l")
        && !name.substr(1).getAsInteger(10, intVal)
        && intVal < 8) {
      RegNo = IntRegs[16 + intVal];
      RegKind = AVROperand::rk_IntReg;
      return true;
    }
    if (name.substr(0, 1).equals_lower("i")
        && !name.substr(1).getAsInteger(10, intVal)
        && intVal < 8) {
      RegNo = IntRegs[24 + intVal];
      RegKind = AVROperand::rk_IntReg;
      return true;
    }
    // %f0 - %f31
    if (name.substr(0, 1).equals_lower("f")
        && !name.substr(1, 2).getAsInteger(10, intVal) && intVal < 32) {
      RegNo = FloatRegs[intVal];
      RegKind = AVROperand::rk_FloatReg;
      return true;
    }
    // %f32 - %f62
    if (name.substr(0, 1).equals_lower("f")
        && !name.substr(1, 2).getAsInteger(10, intVal)
        && intVal >= 32 && intVal <= 62 && (intVal % 2 == 0)) {
      // FIXME: Check V9
      RegNo = DoubleRegs[intVal/2];
      RegKind = AVROperand::rk_DoubleReg;
      return true;
    }

    // %r0 - %r31
    if (name.substr(0, 1).equals_lower("r")
        && !name.substr(1, 2).getAsInteger(10, intVal) && intVal < 31) {
      RegNo = IntRegs[intVal];
      RegKind = AVROperand::rk_IntReg;
      return true;
    }
  }
  return false;
}

static bool hasGOTReference(const MCExpr *Expr) {
  switch (Expr->getKind()) {
  case MCExpr::Target:
    if (const SparcMCExpr *SE = dyn_cast<SparcMCExpr>(Expr))
      return hasGOTReference(SE->getSubExpr());
    break;

  case MCExpr::Constant:
    break;

  case MCExpr::Binary: {
    const MCBinaryExpr *BE = cast<MCBinaryExpr>(Expr);
    return hasGOTReference(BE->getLHS()) || hasGOTReference(BE->getRHS());
  }

  case MCExpr::SymbolRef: {
    const MCSymbolRefExpr &SymRef = *cast<MCSymbolRefExpr>(Expr);
    return (SymRef.getSymbol().getName() == "_GLOBAL_OFFSET_TABLE_");
  }

  case MCExpr::Unary:
    return hasGOTReference(cast<MCUnaryExpr>(Expr)->getSubExpr());
  }
  return false;
}

bool AVRAsmParser::matchSparcAsmModifiers(const MCExpr *&EVal,
                                            SMLoc &EndLoc)
{
  AsmToken Tok = Parser.getTok();
  if (!Tok.is(AsmToken::Identifier))
    return false;

  StringRef name = Tok.getString();

  SparcMCExpr::VariantKind VK = SparcMCExpr::parseVariantKind(name);

  if (VK == SparcMCExpr::VK_Sparc_None)
    return false;

  Parser.Lex(); // Eat the identifier.
  if (Parser.getTok().getKind() != AsmToken::LParen)
    return false;

  Parser.Lex(); // Eat the LParen token.
  const MCExpr *subExpr;
  if (Parser.parseParenExpression(subExpr, EndLoc))
    return false;

  bool isPIC = getContext().getObjectFileInfo()->getRelocM() == Reloc::PIC_;

  switch(VK) {
  default: break;
  case SparcMCExpr::VK_Sparc_LO:
    VK =  (hasGOTReference(subExpr)
           ? SparcMCExpr::VK_Sparc_PC10
           : (isPIC ? SparcMCExpr::VK_Sparc_GOT10 : VK));
    break;
  case SparcMCExpr::VK_Sparc_HI:
    VK =  (hasGOTReference(subExpr)
           ? SparcMCExpr::VK_Sparc_PC22
           : (isPIC ? SparcMCExpr::VK_Sparc_GOT22 : VK));
    break;
  }

  EVal = SparcMCExpr::Create(VK, subExpr, getContext());
  return true;
}


extern "C" void LLVMInitializeAVRAsmParser() {
  RegisterMCAsmParser<AVRAsmParser> A(TheSparcTarget);
  RegisterMCAsmParser<AVRAsmParser> B(TheSparcV9Target);
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "AVRGenAsmMatcher.inc"

unsigned AVRAsmParser::validateTargetOperandClass(MCParsedAsmOperand &GOp,
                                                    unsigned Kind) {
  AVROperand &Op = (AVROperand &)GOp;
  if (Op.isFloatOrDoubleReg()) {
    switch (Kind) {
    default: break;
    case MCK_DFPRegs:
      if (!Op.isFloatReg() || AVROperand::MorphToDoubleReg(Op))
        return MCTargetAsmParser::Match_Success;
      break;
    case MCK_QFPRegs:
      if (AVROperand::MorphToQuadReg(Op))
        return MCTargetAsmParser::Match_Success;
      break;
    }
  }
  return Match_InvalidOperand;
}
