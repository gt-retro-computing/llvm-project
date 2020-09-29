//
// Created by codetector on 12/16/19.
//
#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "Z80AsmParser.h"
#include "Z80RegisterInfo.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Support/Casting.h"

#define DEBUG_TYPE "Z80-AsmParser"

using namespace llvm;
using namespace Z80Asm;

Z80Operand::Z80Operand(const Z80Operand &O) : MCParsedAsmOperand() {
  Kind = O.Kind;
  StartLoc = O.StartLoc;
  EndLoc = O.EndLoc;
  // TODO Copy Imm / Reg / Token depending on Kind
}

void Z80Operand::addRegOperands(MCInst &Inst, unsigned N) const {
  assert(N == 1 && "Can not add more than 1 reg operand");
  Inst.addOperand(MCOperand::createReg(getReg()));
}

void Z80Operand::addImmOperands(MCInst &Inst, unsigned N) const {
  assert(N == 1 && "Invalid # of operands");
  addExpr(Inst, this->Imm.Val);
}

void Z80Operand::addExpr(MCInst &Inst, const MCExpr *Expr) const {
  if (Expr == nullptr) {
    // NULL
    LLVM_DEBUG(dbgs() << "addExpr() called with a null expr\n"
                         "adding an imm(0); \n");
    Inst.addOperand(MCOperand::createImm(0));
  } else if (const auto *CE = dyn_cast<MCConstantExpr>(Expr)) {
    LLVM_DEBUG(dbgs() << "Collapsing const expr to imm\n");
    Inst.addOperand(MCOperand::createImm(CE->getValue()));
  } else {
    Inst.addOperand(MCOperand::createExpr(Expr));
  }
}

StringRef Z80Operand::getToken() const {
  assert(Kind == KindTy::Token && "Trying to get token from a non-token type.");
  return {Tok.Data, Tok.Length};
}

bool Z80Operand::isToken() const { return this->Kind == KindTy::Token; }

bool Z80Operand::isImm() const { return this->Kind == KindTy::Immediate; }

bool Z80Operand::isReg() const { return this->Kind == KindTy::Register; }

unsigned int Z80Operand::getReg() const { return this->Reg.RegNumber; }

bool Z80Operand::isMem() const {
  // TODO Z80Operand Mem Kind
  return false;
}

SMLoc Z80Operand::getStartLoc() const { return this->StartLoc; }

SMLoc Z80Operand::getEndLoc() const { return this->EndLoc; }

void Z80Operand::print(raw_ostream &OS) const {
  OS << "<";
  switch (this->Kind) {
  case KindTy::Token:
    OS << "Token: " << StringRef(this->Tok.Data, this->Tok.Length);
    break;
  case KindTy::Register:
    OS << "Reg:" << Z80::D;
    break;
  case KindTy::Immediate:
    OS << "Imm: " << this->Imm.Val;
    break;
  case KindTy::MemOff:
    OS << "MemOff";
    break;
  case KindTy::CondCode:
    OS << "CC";
    break;
  }
  OS << ">";
}

void Z80Operand::addZ80MemAOperands(MCInst &Inst, unsigned int N) const {
  assert(N == 1);
  addRegOperands(Inst, 1);
}

void Z80Operand::addZ80MemOffOperands(MCInst &Inst, unsigned N) const {
  assert(N == 2);
  Inst.addOperand(MCOperand::createReg(this->MemOffset.Reg));
  addExpr(Inst, this->MemOffset.Off);
}

void Z80Operand::addZ80CCOperands(MCInst &Inst, unsigned int N) const {
  assert(N == 1);
  Inst.addOperand(MCOperand::createImm(static_cast<int64_t>(this->CondOp.CC)));
}

bool Z80Operand::isZ80MemA() const {
  return this->Kind == KindTy::Register && this->Reg.RegNumber == Z80::HL;
}

bool Z80Operand::isZ80MemOff() const { return this->Kind == KindTy::MemOff; }

bool Z80Operand::isZ80CC() const { return this->Kind == KindTy::CondCode; }

std::unique_ptr<Z80Operand> Z80Operand::CreateToken(StringRef Token,
                                                    SMLoc Start) {
  auto Operand = std::make_unique<Z80Operand>(KindTy::Token, Start, Start);
  Operand->Tok.Data = Token.data();
  Operand->Tok.Length = Token.size();
  return Operand;
}
std::unique_ptr<Z80Operand> Z80Operand::CreateReg(unsigned RegNo, SMLoc S,
                                                  SMLoc E) {
  auto Operand = std::make_unique<Z80Operand>(KindTy::Register, S, E);
  Operand->Reg.RegNumber = RegNo;
  return Operand;
}

std::unique_ptr<Z80Operand> Z80Operand::CreateImm(const MCExpr *Val, SMLoc S,
                                                  SMLoc E) {
  auto Operand = std::make_unique<Z80Operand>(KindTy::Immediate, S, E);
  Operand->Imm.Val = Val;
  return Operand;
}

std::unique_ptr<Z80Operand> Z80Operand::CreateMemOff(unsigned int RegNo,
                                                     const MCExpr *Off, SMLoc S,
                                                     SMLoc E) {
  auto Operand = std::make_unique<Z80Operand>(KindTy::MemOff, S, E);
  Operand->MemOffset.Reg = RegNo;
  Operand->MemOffset.Off = Off;
  return Operand;
}
std::unique_ptr<Z80Operand> Z80Operand::CreateCC(StringRef CCString,
                                                           SMLoc Start, bool &Result) {
  auto Kind = StringSwitch<Optional<Z80Operand::CCTy>>(CCString)
                  .CaseLower("nz", Optional<Z80Operand::CCTy>(CCTy::NZ))
                  .CaseLower("z", Optional<Z80Operand::CCTy>(CCTy::Z))
                  .CaseLower("nc", Optional<Z80Operand::CCTy>(CCTy::NC))
                  .CaseLower("c", Optional<Z80Operand::CCTy>(CCTy::C))
                  .CaseLower("po", Optional<Z80Operand::CCTy>(CCTy::PO))
                  .CaseLower("pe", Optional<Z80Operand::CCTy>(CCTy::PE))
                  .CaseLower("p", Optional<Z80Operand::CCTy>(CCTy::P))
                  .CaseLower("m", Optional<Z80Operand::CCTy>(CCTy::M))
                  .Default(NoneType::None);
  auto Operand = std::make_unique<Z80Operand>(KindTy::CondCode, Start, Start);
  if (Kind.hasValue()) {
    Result = true;
    Operand->CondOp.CC = Kind.getValue();
  } else {
    Result = false;
  }
  return Operand;
}
