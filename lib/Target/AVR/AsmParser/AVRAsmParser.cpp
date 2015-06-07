//===---- AVRAsmParser.cpp - Parse AVR assembly to MCInst instructions ----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "AVR.h"
#include "MCTargetDesc/AVRMCTargetDesc.h"
#include "AVRRegisterInfo.h"
#include "MCTargetDesc/AVRMCExpr.h"
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

using namespace llvm;

#define DEBUG_TYPE "avr-asm-parser"

namespace {

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


  bool parseOperand(OperandVector &Operands, StringRef Mnemonic);
  int  parseRegister();
  bool tryParseRegisterOperand(OperandVector &Operands);
  bool tryParseExpression(OperandVector & Operands);
  void appendToken(OperandVector & Operands);

  bool parseDirectiveCodeSegment();
  bool parseDirectiveCodeSegmentSize();
  bool parseDirectiveDefineByte();
  bool parseDirectiveDefineSymbol();
  bool parseDirectiveDevice();
  bool parseDirectiveDataSegment();
  bool parseDirectiveDefineWord();
  bool parseDirectiveEEPromSegment();
  bool parseDirectiveExit();
  bool parseDirectiveList();
  bool parseDirectiveListMacro();
  bool parseDirectiveNoList();

  // Handles target specific special cases. See definition for notes.
  unsigned validateTargetOperandClass(MCParsedAsmOperand &Op, unsigned Kind);

  // Given a register returns the corresponding DREG
  unsigned toDREG(unsigned Reg, unsigned From = AVR::sub_lo) {
    return MRI->getMatchingSuperReg(Reg, From, &AVRMCRegisterClasses[AVR::DREGSRegClassID]);
  }
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

  enum KindTy {
    k_Immediate,
    k_Register,
    k_Token
  } Kind;

public:
  explicit AVROperand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}
  explicit AVROperand(StringRef Tok) : MCParsedAsmOperand(), Kind(k_Token), Tok(Tok) {}
  explicit AVROperand(unsigned  Reg) : MCParsedAsmOperand(), Kind(k_Register), Reg(Reg) {}
  explicit AVROperand(MCExpr const* Imm) : MCParsedAsmOperand(), Kind(k_Immediate), Imm(Imm) {}

  union {
    StringRef      Tok;
    unsigned       Reg;
    MCExpr const*  Imm;
  };

  SMLoc StartLoc, EndLoc;

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
    auto Op = make_unique<AVROperand>(Str);
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static std::unique_ptr<AVROperand> CreateReg(unsigned RegNum, SMLoc S,
                                                SMLoc E) {
    auto Op = make_unique<AVROperand>(RegNum);
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static std::unique_ptr<AVROperand> CreateImm(const MCExpr *Val, SMLoc S, SMLoc E) {
    auto Op = make_unique<AVROperand>(Val);
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  SMLoc getStartLoc() const { return StartLoc; }
  SMLoc getEndLoc()   const { return EndLoc;   }

  virtual void print(raw_ostream &OS) const {
    switch(Kind) {
      case k_Token:     OS << "Token: \""   << Tok << "\"";                break;
      case k_Register:  OS << "Register: "  << Reg;                        break;
      case k_Immediate: OS << "Immediate: \""; Imm->print(OS); OS << "\""; break;
    }
    OS << "\n";
  }
};

} // end of anonymous namespace

// Auto-generated Match Functions

// Returns the register number, or 0 if the register is invalid.
static unsigned MatchRegisterName(StringRef Name);

bool AVRAsmParser::
MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                        OperandVector &Operands,
                        MCStreamer &Out, uint64_t &ErrorInfo,
                        bool MatchingInlineAsm) {
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
    case Match_InvalidOperand: {
      SMLoc ErrorLoc = IDLoc;
      if (ErrorInfo != ~0U) {
        if (ErrorInfo >= Operands.size())
          return Error(IDLoc, "too few operands for instruction");

        ErrorLoc = ((AVROperand &)*Operands[ErrorInfo]).getStartLoc();
        if (ErrorLoc == SMLoc()) ErrorLoc = IDLoc;
      }

      return Error(ErrorLoc, "invalid operand for instruction");
    }
    case Match_MnemonicFail:
      return Error(IDLoc, "invalid instruction");
  }
  return true;
}

int AVRAsmParser::parseRegister() {
  const AsmToken &Tok = Parser.getTok();
  int RegNum = 0;
  if (Tok.is(AsmToken::Identifier)) {
      // check for register pair syntax
    if (Parser.getLexer().peekTok().is(AsmToken::Colon)) {
      Parser.Lex(); Parser.Lex(); // eat high (odd) register and colon
      if (Parser.getTok().is(AsmToken::Identifier)) {
        // convert lower (even) register to DREG
        RegNum = toDREG(MatchRegisterName(Parser.getTok().getString().lower()));
      }
    } else {
      RegNum = MatchRegisterName(Tok.getString().lower());
    }
  }
  return RegNum;
}

bool AVRAsmParser::
tryParseRegisterOperand(OperandVector &Operands){

  int RegNo = parseRegister();
  if (RegNo == 0) return true;

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

bool AVRAsmParser::parseOperand(OperandVector &Operands,
                                StringRef Mnemonic) {
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
    //case AsmToken::String:
      return tryParseExpression(Operands);

    // Parse the syntax .[+][-]offset
    // for PC-relative call.
    case AsmToken::Dot:
      Parser.Lex(); // eat `.`
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
  
  // could not parse operand
  return true;
}

bool AVRAsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                  SMLoc &EndLoc) {

  StartLoc = Parser.getTok().getLoc();
  RegNo = parseRegister();
  EndLoc = Parser.getTok().getLoc();
  return (RegNo == (unsigned)-1);
}

bool AVRAsmParser::
ParseInstruction(ParseInstructionInfo &Info, StringRef Mnemonic, SMLoc NameLoc,
                 OperandVector &Operands) {

  Operands.push_back(AVROperand::CreateToken(Mnemonic, NameLoc));
  bool first = true;
  while (getLexer().isNot(AsmToken::EndOfStatement)) {
    if(!first && getLexer().is(AsmToken::Comma))
      Parser.Lex();  // Eat the comma.

    // Parse and remember the operand.
    if (parseOperand(Operands, Mnemonic)) {
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
AVRAsmParser::parseDirectiveCodeSegment() { return true; }

bool
AVRAsmParser::parseDirectiveCodeSegmentSize() { return true; }

bool
AVRAsmParser::parseDirectiveDefineByte() { return true; }

bool
AVRAsmParser::parseDirectiveDefineSymbol() { return true; }

bool
AVRAsmParser::parseDirectiveDevice() { return true; }

bool
AVRAsmParser::parseDirectiveDataSegment() { return true; }

bool
AVRAsmParser::parseDirectiveDefineWord() { return true; }

bool
AVRAsmParser::parseDirectiveEEPromSegment() { return true; }

bool
AVRAsmParser::parseDirectiveExit() { return true; }

bool
AVRAsmParser::parseDirectiveList() { return true; }

bool
AVRAsmParser::parseDirectiveListMacro() { return true; }

bool
AVRAsmParser::parseDirectiveNoList() { return true; }

bool
AVRAsmParser::ParseDirective(llvm::AsmToken DirectiveID) {
  StringRef ID = DirectiveID.getIdentifier();

  if      (ID == ".cseg")     return parseDirectiveCodeSegment();
  else if (ID == ".csegsize") return parseDirectiveCodeSegmentSize();
  else if (ID == ".db")       return parseDirectiveDefineByte();
  else if (ID == ".def")      return parseDirectiveDefineSymbol();
  else if (ID == ".device")   return parseDirectiveDevice();
  else if (ID == ".dseg")     return parseDirectiveDataSegment();
  else if (ID == ".dw")       return parseDirectiveDefineWord();
  else if (ID == ".eseg")     return parseDirectiveEEPromSegment();
  else if (ID == ".exit")     return parseDirectiveExit();
  else if (ID == ".list")     return parseDirectiveList();
  else if (ID == ".listmac")  return parseDirectiveListMacro();
  else if (ID == ".nolist")   return parseDirectiveNoList();

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
AVRAsmParser::validateTargetOperandClass(MCParsedAsmOperand &AsmOp, unsigned ExpectedKind) {
  AVROperand & Op = static_cast<AVROperand&>(AsmOp);
  if (Op.isReg()) {
    MatchClassKind Expected(static_cast<MatchClassKind>(ExpectedKind));

    // If the instruction uses a register pair but we got a single, lower
    // register we perform a "class cast".
    if (isSubclass(Expected, MCK_DREGS)) {
      unsigned correspondingDREG = toDREG(Op.getReg());
      if (correspondingDREG) {
        Op.Reg = correspondingDREG;
        return Match_Success;
      }
    }

    // Some instructions have hard coded values as operands.
    // For example, the `lpm Rd, Z` instruction, where the second
    // operand is always explicitly Z.
    if (isSubclass(Expected, MCK_Z) && Op.getReg() == AVR::R31R30) {
      return Match_Success;
    }
  }
  return Match_InvalidOperand;
}

