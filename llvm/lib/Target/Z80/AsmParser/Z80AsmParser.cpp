//
// Created by codetector on 12/14/19.
//
#include "Z80AsmParser.h"
#include "llvm/ADT/STLExtras.h"
#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/TargetRegistry.h"

#define DEBUG_TYPE "Z80-AsmParser"

using namespace llvm;
using namespace Z80Asm;

#define GET_REGISTER_MATCHER
#define GET_MATCHER_IMPLEMENTATION

#include "Z80GenAsmMatcher.inc"

bool Z80AsmParser::ParseRegister(unsigned int &RegNo, SMLoc &StartLoc,
                                 SMLoc &EndLoc) {
  SmallVector<std::unique_ptr<MCParsedAsmOperand>, 1> Operands;
  if (ParseRegister(RegNo, Operands))
    return true;
  return false;
}

OperandMatchResultTy Z80AsmParser::tryParseRegister(unsigned int &RegNo,
                                                    SMLoc &StartLoc,
                                                    SMLoc &EndLoc) {
  return MatchOperand_NoMatch;
}

bool Z80AsmParser::ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                                    SMLoc NameLoc, OperandVector &Operands) {
  // First operand is token for instruction
  Operands.push_back(Z80Operand::CreateToken(Name, NameLoc));

  // If there are no more operands, then finish
  if (getLexer().is(AsmToken::EndOfStatement))
    return false;

  // Parse first operand
  if (ParseOperand(Operands, Name))
    return true;

  // Parse until end of statement, consuming commas between operands
  while (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Consume comma token
    auto CurrentTokenKind = getLexer().getKind();
    if (CurrentTokenKind == AsmToken::Comma) {
      getLexer().Lex();
      continue;
    }

    // Parse next operand
    if (ParseOperand(Operands, Name))
      return true;
  } // End While

  if (getLexer().isNot(AsmToken::EndOfStatement))
    return Error(getLexer().getTok().getLoc(),
                 "unexpected token in operand list");

  return false;
}

bool Z80AsmParser::ParseDirective(AsmToken DirectiveID) {
  return true;
}

bool Z80AsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned int &Opcode,
                                           OperandVector &Operands,
                                           MCStreamer &Out, uint64_t &ErrorInfo,
                                           bool MatchingInlineAsm) {
  MCInst Inst;

  switch (MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm)) {
    case Match_Success:
      Out.emitInstruction(Inst, *this->STI);
      return false; // Success
    case Match_MissingFeature:
      return Error(IDLoc, "Target has missing feature(s)");
    case Match_MnemonicFail:
      return Error(IDLoc, "Invalid Mnemonic");
    default:
      return Error(IDLoc, "Failed to match instruction");
  }
}

OperandMatchResultTy Z80AsmParser::ParseMemAOperand(OperandVector &Operands) {
  llvm_unreachable("FUCK");
}

bool Z80AsmParser::ParseRegister(unsigned &RegNo, OperandVector &Operands) {
  SMLoc S = getParser().getTok().getLoc();
  SMLoc E = SMLoc::getFromPointer(getParser().getTok().getLoc().getPointer() - 1);
  const bool Is24Bit = !STI->getTargetTriple().isArch16Bit() && STI->getTargetTriple().getEnvironment() != Triple::CODE16;

  switch (getLexer().getKind()) {
  default:
    return true;
  case AsmToken::Identifier:
    const StringRef regName = getLexer().getTok().getIdentifier();
    const StringRef regNameLowerCase = regName.lower();
    RegNo = MatchRegisterName(regNameLowerCase);
    if (RegNo != 0) {
      getLexer().Lex();
      if (regNameLowerCase == "sp") {
        if (Is24Bit) RegNo = Z80::SPL;
        else RegNo = Z80::SPS;
      }
      Operands.push_back(Z80Operand::CreateReg(RegNo, S, E));
      return false;
    }

    return true;
  }
}


bool Z80AsmParser::ParseImmediate(OperandVector &Operands) {
  SMLoc S = getParser().getTok().getLoc();
  SMLoc E = SMLoc::getFromPointer(getParser().getTok().getLoc().getPointer() - 1);

  const MCExpr *Expr = nullptr;
  AsmToken::TokenKind kind = getLexer().getKind();
  switch (kind) {
  default:
    return true;
  case AsmToken::Minus:
  case AsmToken::Integer:
    if (getParser().parseExpression(Expr))
      return true;

    Operands.push_back(Z80Operand::CreateImm(Expr, S, E));
    return false;
  }
}

bool Z80AsmParser::ParseSymbolReference(OperandVector &Operands) {
  SMLoc S = getParser().getTok().getLoc();
  StringRef Identifier;
  if (getParser().parseIdentifier(Identifier))
    return true;

  SMLoc E = SMLoc::getFromPointer(getParser().getTok().getLoc().getPointer() - 1);

  // TODO relocations?

  // Parse a symbol
  MCSymbol *Sym = getContext().getOrCreateSymbol(Identifier);
  const MCExpr *Res = MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_None,
                                              getContext());
  Operands.push_back(Z80Operand::CreateImm(Res, S, E));
  return false;
}


/*
 * return - false: parse success, true: failed to parse
 */
bool Z80AsmParser::ParseOperand(OperandVector &Operands, StringRef Mnemonic) {
  // A register operand is always alone
  auto CurrentTokenKind = getLexer().getKind();
  if (CurrentTokenKind == AsmToken::LParen) {
    AsmToken LParn = getLexer().getTok();
    auto LParnPushback = getLexer().Lex(); // Consume 1 token,
    // but unlex will use this
    Operands.push_back(
        Z80Operand::CreateToken(LParn.getString(), LParn.getLoc()));
    if (!ParseOperand(Operands, Mnemonic)) {

      // Needful for IX/Y + n
      auto PotentiallyPlusMinus = getLexer().getTok();
      if (PotentiallyPlusMinus.is(AsmToken::Minus) || PotentiallyPlusMinus.is(AsmToken::Plus)) {
        if (PotentiallyPlusMinus.is(llvm::AsmToken::Plus)) // Eat the +
          getLexer().Lex();
        if (ParseImmediate(Operands)) {
          report_fatal_error("Failed attempt at parsing IX/IY+imm");
        }
        assert(Operands.back().get()->isImm());
        auto Fuck = Operands.pop_back_val();
        const auto *Offset = static_cast<Z80Operand*>(Fuck.get())->Imm.Val;

        assert(Operands.back().get()->isReg());
        Fuck = Operands.pop_back_val();
        auto reg = static_cast<Z80Operand*>(Fuck.get())->Reg.RegNumber;

        Operands.push_back(Z80Operand::CreateMemOff(reg, Offset, LParnPushback.getLoc(), getLexer().getLoc()));
      }

      // Success
      auto RParn = getLexer().getTok();
      assert(RParn.is(AsmToken::RParen));
      getLexer().Lex();
      Operands.push_back(
          Z80Operand::CreateToken(RParn.getString(), RParn.getLoc()));
      return false;
    } else {
      // Fail & backtrack
      Operands.pop_back();
      getLexer().UnLex(LParnPushback);
      return true;
    }
  }

  // FIXME: This is very hack
  if (Mnemonic.lower() == "jp" || Mnemonic.lower() == "call" || Mnemonic.lower() == "call" || Mnemonic.lower() == "jr") {
    if (!ParseZ80CC(Operands)) {
      return false;
    }
  }

  unsigned RegNo = 0;
  if (!ParseRegister(RegNo, Operands)) {
    return false; // Register Parse Success
  }


//  if (tryCustomParseOperand(Operands, MCK_BranchCC) ==
//      OperandMatchResultTy::MatchOperand_Success) {
//    return false;
//  }

  // An immediate or expression operand can be alone
  SMLoc S = getLexer().getTok().getLoc();
  if (!ParseImmediate(Operands) || !ParseSymbolReference(Operands)) {
    return false;
  }


  return true;
}

bool Z80AsmParser::ParseZ80CC(OperandVector &Operands) {
  auto &CurrentToken = getLexer().getTok();

  // We are looking for the CC flags, which should all be strings
  if (CurrentToken.isNot(AsmToken::Identifier))
    return true;
  bool Success = false;
  std::unique_ptr<Z80Operand> CC = Z80Operand::CreateCC(CurrentToken.getString(), CurrentToken.getLoc(), Success);
  if (Success) {
    getLexer().Lex(); // Eat token
    Operands.push_back(std::move(CC));
    return false;
  }
  return true;
}

extern "C" void LLVMInitializeZ80AsmParser() {
  // TODO EZ80
  RegisterMCAsmParser<Z80AsmParser> X(getTheZ80Target());
}
