//===---- AVRAsmParser.cpp - Parse AVR assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include "llvm/ADT/APInt.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstBuilder.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"

#include "AVR.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "AVRRegisterInfo.h"
#include "MCTargetDesc/AVRMCExpr.h"


#define DEBUG_TYPE "avr-asm-parser"
#define LLVM_AVR_GCC_COMPAT

namespace llvm {

class AVRAsmParser : public MCTargetAsmParser {
  MCSubtargetInfo &STI;
  MCAsmParser &Parser;
  const MCRegisterInfo * MRI;

#define GET_ASSEMBLER_HEADER
#include "AVRGenAsmMatcher.inc"

  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo, bool MatchingInlineAsm) 
                               override;

  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc,
                        OperandVector &Operands) override;
  
  bool ParseDirective(AsmToken directiveID) override;


  bool parseOperand(OperandVector &Operands);
  int  parseRegisterName();
  int  parseRegister();
  bool tryParseRegisterOperand(OperandVector &Operands);
  bool tryParseExpression(OperandVector & Operands);
  void appendToken(OperandVector & Operands);
  void eatComma();

  // Handles target specific special cases. See definition for notes.
  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op, unsigned Kind);

  // Given a register returns the corresponding DREG
  unsigned toDREG(unsigned Reg, unsigned From = AVR::sub_lo) {
    return MRI->getMatchingSuperReg(Reg, From, &AVRMCRegisterClasses[AVR::DREGSRegClassID]);
  }

  bool InvalidOperand(SMLoc const& Loc, OperandVector const& Operands, uint64_t const& ErrorInfo);
public:
  AVRAsmParser(MCSubtargetInfo &sti, MCAsmParser &parser,
                const MCInstrInfo &MII, const MCTargetOptions &Options)
    : MCTargetAsmParser(), STI(sti), Parser(parser) {
    MCAsmParserExtension::Initialize(Parser);
    MRI = getContext().getRegisterInfo();

    // Initialize the set of available features.
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  MCAsmParser &getParser() const { return Parser; }
  MCAsmLexer &getLexer() const { return Parser.getLexer(); }

};

class AVROperand : public MCParsedAsmOperand {
  typedef MCParsedAsmOperand Base;
  enum KindTy {
    k_Immediate,
    k_Register,
    k_Token
  } Kind;

public:
  AVROperand(StringRef Tok, SMLoc const& S)
    : Base(), Kind(k_Token), Tok(Tok), Start(S), End(S) {}
  AVROperand(unsigned  Reg, SMLoc const& S, SMLoc const& E)
    : Base(), Kind(k_Register), Reg(Reg), Start(S), End(E) {}
  AVROperand(MCExpr const* Imm, SMLoc const& S, SMLoc const& E)
    : Base(), Kind(k_Immediate), Imm(Imm), Start(S), End(E) {}

  union {
    StringRef      Tok;
    unsigned       Reg;
    MCExpr const*  Imm;
  };

  SMLoc Start, End;

public:
  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::createReg(getReg()));
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const{
    // Add as immediate when possible.  Null MCExpr = 0.
    if (Expr == 0)
      Inst.addOperand(MCOperand::createImm(0));
    else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::createImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::createExpr(Expr));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCExpr *Expr = getImm();
    addExpr(Inst,Expr);
  }

  bool isReg()   const { return Kind == k_Register; }
  bool isImm()   const { return Kind == k_Immediate; }
  bool isToken() const { return Kind == k_Token; }
  bool isMem()   const { return false; }

  StringRef getToken() const {
    assert(Kind == k_Token && "Invalid access!");
    return Tok;
  }

  unsigned getReg() const {
    assert((Kind == k_Register) && "Invalid access!");
    return Reg;
  }

  const MCExpr *getImm() const {
    assert((Kind == k_Immediate) && "Invalid access!");
    return Imm;
  }

  static std::unique_ptr<AVROperand> CreateToken(StringRef Str, SMLoc S) {
    return make_unique<AVROperand>(Str, S);
  }

  static std::unique_ptr<AVROperand> CreateReg(unsigned RegNum, SMLoc S, SMLoc E) {
    return make_unique<AVROperand>(RegNum, S, E);
  }

  static std::unique_ptr<AVROperand> CreateImm(const MCExpr *Val, SMLoc S, SMLoc E) {
    return make_unique<AVROperand>(Val, S, E);
  }

  void makeToken(StringRef Token) { Kind = k_Token; Tok = Token; }
  void makeReg(unsigned RegNo)    { Kind = k_Register; Reg = RegNo; }
  void makeImm(MCExpr const* Ex)  { Kind = k_Immediate; Imm = Ex; }

  SMLoc getStartLoc() const { return Start; }
  SMLoc getEndLoc()   const { return End;   }

  virtual void print(raw_ostream &OS) const {
    switch(Kind) {
      case k_Token:     OS << "Token: \"" << Tok << "\"";                  break;
      case k_Register:  OS << "Register: " << Reg;                         break;
      case k_Immediate: OS << "Immediate: \""; Imm->print(OS); OS << "\""; break;
    }
    OS << "\n";
  }
};

// Auto-generated Match Functions

// Returns the register number, or 0 if the register is invalid.
static unsigned MatchRegisterName(StringRef Name);

bool
AVRAsmParser::InvalidOperand(SMLoc const& Loc, OperandVector const& Operands,
                             uint64_t const& ErrorInfo)
{
  SMLoc ErrorLoc = Loc;
  char const* Diag = 0;
  if (ErrorInfo != ~0U) {
    if (ErrorInfo >= Operands.size()) {
      Diag = "too few operands for instruction.";
    } else {
      AVROperand const& Op = (AVROperand const&)*Operands[ErrorInfo];
      // TODO: See if we can do better than just "invalid ...".
      if (Op.getStartLoc() != SMLoc()) {
        ErrorLoc = Op.getStartLoc();
      }
    }
  }

  if (not Diag) {
    Diag = "invalid operand for instruction";
  }
  return Error(ErrorLoc, Diag);
}

bool
AVRAsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                      OperandVector &Operands, MCStreamer &Out,
                                      uint64_t &ErrorInfo, bool MatchingInlineAsm)
{
  MCInst Inst;
  unsigned MatchResult = MatchInstructionImpl(Operands, Inst, ErrorInfo,
                                              MatchingInlineAsm);
  switch (MatchResult) {
    case Match_Success:
      Inst.setLoc(IDLoc);
      Out.EmitInstruction(Inst, STI);
      return false;

    case Match_MissingFeature:
      return Error(IDLoc, "instruction requires a CPU feature not currently enabled");

    case Match_InvalidOperand:
      return InvalidOperand(IDLoc, Operands, ErrorInfo);

    case Match_MnemonicFail:
      return Error(IDLoc, "invalid instruction");
  }
  return true;
}

int
AVRAsmParser::parseRegisterName() {
  StringRef Name = Parser.getTok().getString();
  int RegNum = MatchRegisterName(Name);
#ifdef LLVM_AVR_GCC_COMPAT
  // GCC supports case insensitive register names. Some of the AVR registers
  // are all lower case, some are all upper case but non are mixed. We prefer
  // to use the original names in the register definitions. That is why we
  // have to test both upper and lower case here.
  if (RegNum == AVR::NoRegister) {
    RegNum = MatchRegisterName(Name.lower());
  }
  if (RegNum == AVR::NoRegister) {
    RegNum = MatchRegisterName(Name.upper());
  }
#endif
  return RegNum;
}

int
AVRAsmParser::parseRegister() {
  int RegNum = AVR::NoRegister;
  if (Parser.getTok().is(AsmToken::Identifier)) {
    // check for register pair syntax
    if (Parser.getLexer().peekTok().is(AsmToken::Colon)) {
      Parser.Lex(); Parser.Lex(); // eat high (odd) register and colon
      if (Parser.getTok().is(AsmToken::Identifier)) {
        // convert lower (even) register to DREG
        RegNum = toDREG(parseRegisterName());
      }
    } else {
      RegNum = parseRegisterName();
    }
  }
  return RegNum;
}

bool
AVRAsmParser::tryParseRegisterOperand(OperandVector &Operands){

  int RegNo = parseRegister();
  if (RegNo == AVR::NoRegister) return true;

  AsmToken const& T = Parser.getTok();
  Operands.push_back(AVROperand::CreateReg(RegNo, T.getLoc(), T.getEndLoc()));
  Parser.Lex(); // Eat register token.

  // If the next token is a sign, add it.
  AsmToken const& Sign = Parser.getTok();
  if (Sign.getKind() == AsmToken::Plus || Sign.getKind() == AsmToken::Minus) {
    appendToken(Operands);
  }
  return false;
}

bool
AVRAsmParser::tryParseExpression(OperandVector & Operands) {
  AVRMCExpr::VariantKind ModifierKind = AVRMCExpr::VK_AVR_None;
  SMLoc S = Parser.getTok().getLoc();

  // check if we have a target specific modifier (lo8, hi8, &c)
  if (Parser.getTok().getKind() == AsmToken::Identifier &&
      Parser.getLexer().peekTok().getKind() == AsmToken::LParen)
  {

    StringRef ModifierName = Parser.getTok().getString();
    ModifierKind = AVRMCExpr::getKindByName(ModifierName.str().c_str());
    if (ModifierKind != AVRMCExpr::VK_AVR_None) {
      Parser.Lex(); Parser.Lex(); // eat modifier name and parenthesis
    }
  }

  // parse (potentially inner) expression
  MCExpr const* Expression;
  if (getParser().parseExpression(Expression)) return true;

  // if we have a modifier wrap the inner expression
  if (ModifierKind != AVRMCExpr::VK_AVR_None) {
    assert(Parser.getTok().getKind() == AsmToken::RParen);
    Parser.Lex(); // eat closing parenthesis
    Expression = AVRMCExpr::create(ModifierKind, Expression, getContext());
  }

  SMLoc E = SMLoc::getFromPointer(Parser.getTok().getLoc().getPointer() - 1);
  Operands.push_back(AVROperand::CreateImm(Expression, S, E));
  return false;
}

void
AVRAsmParser::appendToken(OperandVector & Operands) {
  AsmToken const& T(Parser.getTok());
  Operands.push_back(AVROperand::CreateToken(T.getString(), T.getLoc()));
  Parser.Lex();
}

bool
AVRAsmParser::parseOperand(OperandVector &Operands) {
  DEBUG(dbgs() << "parseOperand\n");

  switch (getLexer().getKind()) {
    default:
      return Error(Parser.getTok().getLoc(), "unexpected token in operand");

    case AsmToken::Identifier:
      if(!tryParseRegisterOperand(Operands)) {
        return false;
      }
      return tryParseExpression(Operands);

    case AsmToken::LParen:
    case AsmToken::Integer:
      return tryParseExpression(Operands);

    case AsmToken::Dot:
      // Parse the '.[+-]offset' syntax for PC-relative call.
      Parser.Lex(); // Eat `.`
      return tryParseExpression(Operands);

    case AsmToken::Plus:
    case AsmToken::Minus: {
      if (Parser.getLexer().peekTok().getKind() == AsmToken::Integer) {
        return tryParseExpression(Operands);
      }
      appendToken(Operands);
      return false;
    }
  }
  
  // Could not parse operand
  return true;
}

bool
AVRAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) {
  StartLoc = Parser.getTok().getLoc();
  RegNo = parseRegister();
  EndLoc = Parser.getTok().getLoc();
  return (RegNo == AVR::NoRegister);
}

void
AVRAsmParser::eatComma() {
  if (getLexer().is(AsmToken::Comma)) {
    Parser.Lex();
  } else {
#ifndef LLVM_AVR_GCC_COMPAT
    Error(Parser.getTok().getLoc(), "missing comma");
#endif
  }
}

bool
AVRAsmParser::ParseInstruction(ParseInstructionInfo &Info, StringRef Mnemonic,
                               SMLoc NameLoc, OperandVector & Operands)
{

  Operands.push_back(AVROperand::CreateToken(Mnemonic, NameLoc));
  bool first = true;
  while (getLexer().isNot(AsmToken::EndOfStatement)) {
    if(!first) eatComma();

    if (parseOperand(Operands)) {
      SMLoc Loc = getLexer().getLoc();
      Parser.eatToEndOfStatement();
      return Error(Loc, "unexpected token in argument list");
    }
    first = false;
  }
  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

bool
AVRAsmParser::ParseDirective(llvm::AsmToken DirectiveID) {
  return true;
}

extern "C" void LLVMInitializeAVRAsmParser() {
  RegisterMCAsmParser<AVRAsmParser> X(TheAVRTarget);
}

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION
#include "AVRGenAsmMatcher.inc"

// Uses enums defined in AVRGenAsmMatcher.inc
unsigned
AVRAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp,
                                         unsigned ExpectedKind)
{
  AVROperand & Op = static_cast<AVROperand&>(AsmOp);
  MatchClassKind Expected = static_cast<MatchClassKind>(ExpectedKind);
#ifdef LLVM_AVR_GCC_COMPAT
  // If need be, GCC converts bare numbers to register names
  if (Op.isImm()) {
    if (MCConstantExpr const*const Const = dyn_cast<MCConstantExpr>(Op.getImm())) {
      int64_t RegNum = Const->getValue();
      std::ostringstream RegName;
      RegName << "r" << RegNum;
      RegNum = MatchRegisterName(RegName.str().c_str());
      if (RegNum != AVR::NoRegister) {
        Op.makeReg(RegNum);
        if (validateOperandClass(Op, Expected) == Match_Success) return Match_Success;
      }
      // Let the other quirks try their magic.
    }
  }
#endif

  if (Op.isReg()) {
#ifdef LLVM_AVR_GCC_COMPAT
    // If the instruction uses a register pair but we got a single, lower
    // register we perform a "class cast".
    if (isSubclass(Expected, MCK_DREGS)) {
      unsigned correspondingDREG = toDREG(Op.getReg());
      if (correspondingDREG != AVR::NoRegister) {
        Op.makeReg(correspondingDREG);
        return validateOperandClass(Op, Expected);
      }
    }
#endif

    // Some instructions have hard coded values as operands.
    // For example, the `lpm Rd, Z` instruction, where the second
    // operand is always explicitly Z.
    if (isSubclass(Expected, MCK_Z) && Op.getReg() == AVR::R31R30) {
      return Match_Success;
    }
  }
  return Match_InvalidOperand;
}

} // end of namespace llvm

