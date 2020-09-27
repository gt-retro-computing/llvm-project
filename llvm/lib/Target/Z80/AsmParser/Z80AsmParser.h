//
// Created by codetector on 12/16/19.
//

#ifndef LLVM_Z80ASMPARSER_H
#define LLVM_Z80ASMPARSER_H

#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCSubtargetInfo.h"

using namespace llvm;
namespace Z80Asm {

class Z80AsmParser : public MCTargetAsmParser {
#define GET_ASSEMBLER_HEADER
#include "Z80GenAsmMatcher.inc"

public:
  Z80AsmParser(const MCSubtargetInfo &STI, MCAsmParser &P,
               const MCInstrInfo &MII, const MCTargetOptions &Options)
      : MCTargetAsmParser(Options, STI, MII) {
    setAvailableFeatures(STI.getFeatureBits());
  }
  bool ParseRegister(unsigned int &RegNo, SMLoc &StartLoc,
                     SMLoc &EndLoc) override;
  OperandMatchResultTy tryParseRegister(unsigned int &RegNo, SMLoc &StartLoc,
                                        SMLoc &EndLoc) override;
  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool ParseDirective(AsmToken DirectiveID) override;
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned int &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               uint64_t &ErrorInfo,
                               bool MatchingInlineAsm) override;
  OperandMatchResultTy ParseMemAOperand(OperandVector &Operands);
  bool ParseOperand(OperandVector &Operands, StringRef Mnemonic);
  bool ParseRegister(unsigned &RegNo, OperandVector &Operands);

  bool ParseImmediate(OperandVector &Operands);

  bool ParseSymbolReference(OperandVector &Operands);

  //  void convertToMapAndConstraints(unsigned int Kind,
  //                                  const OperandVector &Operands) override;
  bool ParseZ80CC(OperandVector &pVector);
};

struct Z80Operand final : public MCParsedAsmOperand {
  enum class KindTy { Token, Register, Immediate, MemOff, CondCode } Kind;
  enum class CCTy {
    NZ = 0b000,
    Z = 0b001,
    NC = 0b010,
    C = 0b011,
    PO = 0b100,
    PE = 0b101,
    P = 0b110,
    M = 0b111
  };

  SMLoc StartLoc, EndLoc;
  SMLoc OffsetOfLoc;
  StringRef SymName;

  struct TokOp {
    const char *Data;
    unsigned Length;
  };

  struct RegOp {
    unsigned RegNumber;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  struct MemOff {
    unsigned Reg;
    const MCExpr *Off;
  };

  struct CondCode {
    CCTy CC;
  };

  union {
    struct TokOp Tok;
    struct RegOp Reg;
    struct ImmOp Imm;
    struct CondCode CondOp;
    struct MemOff MemOffset;
  };

  void addExpr(MCInst &, const MCExpr *Expr) const;

  void addRegOperands(MCInst &Inst, unsigned N) const;

  void addImmOperands(MCInst &Inst, unsigned N) const;

  void addZ80MemAOperands(MCInst &Inst, unsigned N) const;

  void addZ80MemOffOperands(MCInst &Inst, unsigned int N) const;

  void addZ80CCOperands(MCInst &Inst, unsigned int N) const;

  bool isZ80MemA() const;

  bool isToken() const override;

  bool isImm() const override;

  bool isReg() const override;

  unsigned int getReg() const override;

  StringRef getToken() const;

  bool isMem() const override;

  SMLoc getStartLoc() const override;

  SMLoc getEndLoc() const override;

  void print(raw_ostream &OS) const override;

  Z80Operand(KindTy K) : MCParsedAsmOperand(), Kind(K) {}

  Z80Operand(KindTy K, SMLoc Start, SMLoc End)
      : MCParsedAsmOperand(), Kind(K), StartLoc(Start), EndLoc(End) {}
  Z80Operand(const Z80Operand &);

  static std::unique_ptr<Z80Operand> CreateToken(StringRef Name, SMLoc Start);

  static std::unique_ptr<Z80Operand> CreateReg(unsigned RegNo, SMLoc S,
                                               SMLoc E);

  static std::unique_ptr<Z80Operand> CreateImm(const MCExpr *Val, SMLoc S,
                                               SMLoc E);

  static std::unique_ptr<Z80Operand> CreateCC(StringRef CCString, SMLoc Start, bool& Result);

  static std::unique_ptr<Z80Operand>
  CreateMemOff(unsigned RegNo, const MCExpr *Off, SMLoc S, SMLoc E);

  bool isZ80MemOff() const;
  bool isZ80CC() const;
};

} // namespace Z80Asm
#endif // LLVM_Z80ASMPARSER_H
