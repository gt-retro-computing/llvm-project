//===-- MipsMCCodeEmitter.cpp - Convert Mips Code to Machine Code ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the MipsMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#include "Z80MCCodeEmitter.h"
#include "MCTargetDesc/Z80FixupKinds.h"
#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "Z80InstrInfo.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrDesc.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include <cassert>
#include <cstdint>

#define DEBUG_TYPE "mccodeemitter"

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");
STATISTIC(MCNumFixups, "Number of MC fixups created");

using namespace llvm;

#define GET_INSTRMAP_INFO
#include "Z80GenInstrInfo.inc"
#undef GET_INSTRMAP_INFO

namespace llvm {

MCCodeEmitter *createZ80MCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx) {
  return new Z80MCCodeEmitter(MCII, Ctx);
}

} // namespace llvm

void Z80MCCodeEmitter::emitByte(unsigned char C, unsigned &CurByte,
                                raw_ostream &OS) const {
  OS << (char)C;
  CurByte++;
}

void Z80MCCodeEmitter::emitWordLE(unsigned short C, unsigned &CurByte,
                                  raw_ostream &OS) const {
  emitByte((unsigned char)((C >> 0U) & 0xFFU), CurByte, OS);
  emitByte((unsigned char)((C >> 8U) & 0xFFU), CurByte, OS);
}

uint8_t Z80MCCodeEmitter::getZ80RegisterEncoding(const MCInst &MI,
                                                 unsigned RegisterNo) const {
  switch (RegisterNo) {
  case Z80::BC:
    return 0b00;
  case Z80::DE:
    return 0b01;
  case Z80::HL:
  case Z80::IX: // IX IY uses HL encoding
  case Z80::IY:
    return 0b10;
  case Z80::AF:
    return 0b11;

  // 8 Bit Registers
  case Z80::A:
    return 0b111;
  case Z80::B:
    return 0b000;
  case Z80::C:
    return 0b001;
  case Z80::D:
    return 0b010;
  case Z80::E:
    return 0b011;
  case Z80::H:
    return 0b100;
  case Z80::L:
    return 0b101;
  }
  llvm_unreachable("Unsupported Register");
}

void Z80MCCodeEmitter::emitZ80Prefix(const MCInst &MI,
                                     const MCInstrDesc &MIDesc,
                                     unsigned &CurByte, raw_ostream &OS) const {

  auto OpCode = MI.getOpcode();
  auto TS = MIDesc.TSFlags;

  // DD Prefix
  bool EmitCBPrefix = false;
  bool EmitDDPrefix = false;
  bool EmitEDPrefix = false;
  bool EmitFDPrefix = false;

  if (TS & Z80II::EDPrefix) {
    EmitEDPrefix = true;
  }

  switch (OpCode) {
  case Z80::BIT8bg:
  case Z80::BIT8bo:
  case Z80::SRL8r:
  case Z80::SRA8r:
  case Z80::SLA8r:
  case Z80::SLL8r:
  case Z80::RR8r:
  case Z80::RL8r:
  case Z80::RLC8r:
  case Z80::RRC8r:
    EmitCBPrefix = true;
    break;
  default:
    break;
  }

  if (TS & (Z80II::AnyIndexPrefix | Z80II::IndexedIndexPrefix)) {
    unsigned int Reg = Z80::NoRegister;
    switch (OpCode) {
      // PUSH/POP IX/IY
    case Z80::PUSH16r:
    case Z80::POP16r:
    case Z80::LD16ri:
    case Z80::ADD16SP: // translates into ADD HL/IX/IY,ss where ss is coded SP
    case Z80::ADD16ao:
    case Z80::DEC16r:
    case Z80::EX16SP: // EX (sp), HL/IX/IY Operands: [SP, rp, SP, rp]
    case Z80::LD8og:  // LD (IX/IY + n), r Operands: [IX/IY, n, r]
    case Z80::LD8ri:  // LD r, n   Operands: [IXhl/IYhl/r8, n]
    case Z80::LD16am:
    case Z80::LD16om:
    case Z80::INC16r:
    case Z80::ADD16aa:
    case Z80::LD8oi:
      Reg = MI.getOperand(0).getReg();
      break;
    case Z80::BIT8bo: // BIT b, IX/IY, Offset
    case Z80::LD8go:
    case Z80::LD16SP: // LD sp, IX/IY/HL Operands: [SP, rp]
    case Z80::LD16ma:
      Reg = MI.getOperand(1).getReg();
      break;
    // Not IX IY Cases
    case Z80::SBC16ao:
    case Z80::BIT8bg:
    case Z80::SRL8r:
    case Z80::SRA8r:
    case Z80::SLA8r:
    case Z80::SLL8r:
    case Z80::RR8r:
    case Z80::RL8r:
    case Z80::OUTcr:
    case Z80::INrc:
    case Z80::INI16:
    case Z80::RLC8r:
    case Z80::RRC8r:
    case Z80::OUTI16:
    case Z80::ADC16ao:
    case Z80::ADC16aa:
    case Z80::SBC16aa:
    case Z80::ADC16SP:
    case Z80::SBC16SP:
      break;
    default:
      MI.dump();
      llvm_unreachable("MI with Prefix Flag but no prefix rules applied.");
      break;
    }

    if (Reg == Z80::IX)
      EmitDDPrefix = true;
    else if (Reg == Z80::IY)
      EmitFDPrefix = true;
  }

  if (EmitDDPrefix)
    emitByte(0xDD, CurByte, OS);

  if (EmitFDPrefix)
    emitByte(0xFD, CurByte, OS);

  if (EmitEDPrefix)
    emitByte(0xED, CurByte, OS);

  if (EmitCBPrefix)
    emitByte(0xCB, CurByte, OS);
}

void Z80MCCodeEmitter::patchCC(const MCInst &MI, uint8_t &PrimaryOpcode) const {
  auto Opcode = MI.getOpcode();
  if (Opcode == Z80::JP16CC || Opcode == Z80::CALL16CC) {
    assert(MI.getOperand(1).isImm());
    auto CC = MI.getOperand(1).getImm();
    assert(CC <= 0b111 && "CC is in range");
    PrimaryOpcode |= CC << 3;
    return;
  }
}

void Z80MCCodeEmitter::patchRegister(const MCInst &MI,
                                     uint8_t &PrimaryOpcode) const {
  auto Opcode = MI.getOpcode();

  enum class RegPatch {
    R8_DST,
    R8_SRC,
    R16,
    R8_8,
    NoPatch
  } PatchType = RegPatch::NoPatch;
  unsigned Reg = 0;
  unsigned Reg2 = 0;

  switch (Opcode) {
  case Z80::PUSH16r:
  case Z80::POP16r:
  case Z80::LD16ri:
  case Z80::DEC16r:
  case Z80::INC16r:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(0).getReg());
    PatchType = RegPatch::R16;
    break;
  case Z80::SBC16ao:
  case Z80::ADC16ao:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(0).getReg());
    PatchType = RegPatch::R16;
    break;
  case Z80::ADD16ao:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(2).getReg());
    PatchType = RegPatch::R16;
    break;
    // 8 Bit Register Patches
  case Z80::LD8pg:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(1).getReg());
    PatchType = RegPatch::R8_SRC;
    break;
  case Z80::LD8go:
  case Z80::LD8ri:
  case Z80::LD8gp:
  case Z80::INrc:
  case Z80::OUTcr:
  case Z80::DEC8r:
  case Z80::INC8r:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(0).getReg());
    PatchType = RegPatch::R8_DST;
    break;
  case Z80::OR8ar:
  case Z80::XOR8ar:
  case Z80::AND8ar:
  case Z80::ADD8ar:
  case Z80::SUB8ar:
  case Z80::SBC8ar:
  case Z80::ADC8ar:
  case Z80::SRL8r:
  case Z80::SRA8r:
  case Z80::SLA8r:
  case Z80::SLL8r:
  case Z80::RR8r:
  case Z80::RL8r:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(0).getReg());
    PatchType = RegPatch::R8_SRC;
    break;
  case Z80::LD8gg:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(0).getReg());
    Reg2 = getZ80RegisterEncoding(MI, MI.getOperand(1).getReg());
    PatchType = RegPatch::R8_8;
    break;
  case Z80::LD8og:
    Reg = getZ80RegisterEncoding(MI, MI.getOperand(2).getReg());
    PatchType = RegPatch::R8_SRC;
    break;
  // Bit Operations
  case Z80::BIT8bg: // bit imm, G8
    Reg = MI.getOperand(0).getImm();
    Reg2 = getZ80RegisterEncoding(MI, MI.getOperand(1).getReg());
    PatchType = RegPatch::R8_8;
    break;
  case Z80::BIT8bo:
    Reg = MI.getOperand(0).getImm();
    PatchType = RegPatch::R8_DST;
    break;
  }

  switch (PatchType) {
  case RegPatch::R16:
    assert(Reg <= 0b11);
    PrimaryOpcode |= Reg << 4;
    break;
  case RegPatch::R8_DST:
    assert(Reg <= 0b111);
    PrimaryOpcode |= Reg << 3;
    break;
  case RegPatch::R8_SRC:
    assert(Reg <= 0b111);
    PrimaryOpcode |= Reg;
    break;
  case RegPatch::R8_8:
    if (!(Reg <= 0b111 && Reg2 <= 0b111)) {
      MI.dump();
    }
    assert(Reg <= 0b111 && Reg2 <= 0b111);
    PrimaryOpcode |= (Reg << 3) | Reg2;
    break;
  case RegPatch::NoPatch:
    break;
  default:
    llvm_unreachable("Unsupported");
  }
}

void Z80MCCodeEmitter::emitImmVal(unsigned Value, bool Is16Bit,
                                  unsigned &CurByte, raw_ostream &OS) const {
  if (Is16Bit)
    emitWordLE(Value, CurByte, OS);
  else
    emitByte(Value, CurByte, OS);
}

void Z80MCCodeEmitter::emitImmediate(const MCInst &MI, unsigned &CurByte,
                                     raw_ostream &OS,
                                     SmallVectorImpl<MCFixup> &Fixups) const {
  auto &MIDesc = MCII.get(MI.getOpcode());
  auto TS = MIDesc.TSFlags;

  if ((TS & (Z80II::HasOff | Z80II::HasImm)) == 0)
    return;

  auto Opcode = MI.getOpcode();

  assert(MI.getNumOperands() > 0);
  auto ImmOp = MI.getOperand(0);
  bool Is16Bit = false;
  bool isPCRel = false;

  if (TS & Z80II::HasImm || TS & Z80II::HasOff) {
    switch (Opcode) {
    // Special Case: both offset and imm.
    case Z80::LD8oi:
      ImmOp = MI.getOperand(2);
      {
        auto Offset = MI.getOperand(1);
        assert(Offset.isImm() && "LD8oi currently requires a imm as Offset not expr");
        emitImmVal(Offset.getImm(), false, CurByte, OS);
      }
      break;
    case Z80::DJNZ:
      isPCRel = true;
      Is16Bit = false;
      break;
    case Z80::JP16:
    case Z80::JP16CC:
    case Z80::CALL16:
    case Z80::CALL16CC:
    case Z80::LD8ma:
    case Z80::LD8am:
    case Z80::LD16ma:
      Is16Bit = true;
      // Default Operand 0
      break;
    case Z80::LD16ri:
    case Z80::LD16am:
    case Z80::LD16om:
      Is16Bit = true;
      ImmOp = MI.getOperand(1);
      break;

    // 8 Bit Offsets
    case Z80::OUTia:
    case Z80::INai:
    case Z80::AND8ai:
    case Z80::OR8ai:
    case Z80::XOR8ai:
    case Z80::SUB8ai:
    case Z80::ADC8ai:
    case Z80::ADD8ai:
    case Z80::SBC8ai:
    case Z80::CP8ai:
      // Default Operand 0
      break;
    case Z80::LD8go:
      ImmOp = MI.getOperand(2);
      break;
    case Z80::LD8og:
    case Z80::LD8ri:
    case Z80::LD8pi:
      ImmOp = MI.getOperand(1);
      break;
    // Prefix Imm, No action needed here. Return immediately
    case Z80::BIT8bo:
      return;
    default:
      MI.dump();
      llvm_unreachable("Unpatched HasImm");
    }

    if (ImmOp.isImm()) {
      emitImmVal(ImmOp.getImm(), Is16Bit, CurByte, OS);
    } else if (ImmOp.isExpr()) {
      // Emit Fixups for Expr
      const MCExpr *Expr = ImmOp.getExpr();
      MCExpr::ExprKind Kind = Expr->getKind();
      Z80::Fixups FixupKind = Z80::fixup_z80_invalid;

      if (Kind == MCExpr::Binary) {
        auto BinExp = cast<MCBinaryExpr>(Expr);
        assert(BinExp->getRHS()->getKind() == MCExpr::Constant && "Only support Symbol + Constant BinExp");
        assert(BinExp->getOpcode() == MCBinaryExpr::Add && "Only add BinExp is supported RN");
        assert(cast<MCConstantExpr>(BinExp->getRHS())->getValue() != 0 );
        emitImmVal(cast<MCConstantExpr>(BinExp->getRHS())->getValue(), Is16Bit, CurByte, OS);
      } else {
        emitImmVal(0, Is16Bit, CurByte, OS);
      }

      if (Kind == MCExpr::Target) {
        llvm_unreachable("todo, support MCExpr::Target");
      } else if ((Kind == MCExpr::SymbolRef &&
                 cast<MCSymbolRefExpr>(Expr)->getKind() ==
                     MCSymbolRefExpr::VK_None) ||
                 (Kind == MCExpr::Binary &&
                  cast<MCBinaryExpr>(Expr)->getLHS()->getKind() == MCExpr::SymbolRef)) {
        if (Opcode == Z80::DJNZ) {
          FixupKind = Z80::fixup_z80_pcrel8_b2;
        } else if (Opcode == Z80::JP16CC || Opcode == Z80::JP16 ||
                   Opcode == Z80::CALL16 || Opcode == Z80::CALL16CC ||
                   Opcode == Z80::LD8ma || Opcode == Z80::LD8am) {
          FixupKind = Z80::fixup_z80_addr16_b2;
        } else if (Opcode == Z80::LD16ma ) {
          if (MI.getOperand(1).getReg() != Z80::IY &&
              MI.getOperand(1).getReg() != Z80::IX) {
            FixupKind = Z80::fixup_z80_addr16_b2;
          } else {
            FixupKind = Z80::fixup_z80_addr16_b3;
          }
        } else if (Opcode == Z80::LD16am || Opcode == Z80::LD16om ||
                   Opcode == Z80::LD16ri) {
          if (MI.getOperand(0).getReg() != Z80::IY &&
              MI.getOperand(0).getReg() != Z80::IX) {
            FixupKind = Z80::fixup_z80_addr16_b2;
          } else {
            FixupKind = Z80::fixup_z80_addr16_b3;
          }
        }
      }

      if (FixupKind == Z80::fixup_z80_invalid) {
        MI.dump();
      }
      assert(FixupKind != Z80::fixup_z80_invalid && "Unhandled expression!");

      Fixups.push_back(
          MCFixup::create(0, Expr, MCFixupKind(FixupKind), MI.getLoc()));
      ++MCNumFixups;

    } else {
      MI.dump();
      llvm_unreachable("HasImm but got neither imm nor expr");
    }
  }
}

void Z80MCCodeEmitter::emitZ80PrefixImmediate(const MCInst &MI,
                                              const MCInstrDesc &Desc,
                                              unsigned int CurByte,
                                              raw_ostream &OS) const {
  if (MI.getNumOperands() == 0)
    return;

  auto ImmOp = MI.getOperand(0);
  auto Is16Bit = false;

  auto Opcode = MI.getOpcode();
  switch (Opcode) {
  case Z80::BIT8bo:
    ImmOp = MI.getOperand(2);
    break;
  default:
    return;
  }

  // Emit
  if (ImmOp.isImm()) {
    if (Is16Bit)
      emitWordLE(ImmOp.getImm(), CurByte, OS);
    else
      emitByte(ImmOp.getImm(), CurByte, OS);
  } else if (ImmOp.isExpr()) {
    llvm_unreachable("expr not supported as prefix right now");
    // TODO: emit fixups
    if (Is16Bit)
      emitWordLE(0, CurByte, OS);
    else
      emitByte(0, CurByte, OS);

    // Emit Fixups for Expr
    const MCExpr *Expr = ImmOp.getExpr();
    MCExpr::ExprKind Kind = Expr->getKind();
    Z80::Fixups FixupKind = Z80::fixup_z80_invalid;
    bool RelaxCandidate = false;
    if (Kind == MCExpr::Target) {
      llvm_unreachable("todo, support MCExpr::Target");
    } else if (Kind == MCExpr::SymbolRef &&
               cast<MCSymbolRefExpr>(Expr)->getKind() ==
                   MCSymbolRefExpr::VK_None) {
      if (Opcode == Z80::DJNZ) {
        FixupKind = Z80::fixup_z80_pcrel8_b2;
      } else if (Opcode == Z80::JP16CC || Opcode == Z80::JP16 ||
                 Opcode == Z80::CALL16 || Opcode == Z80::CALL16CC) {
        FixupKind = Z80::fixup_z80_addr16_b2;
      } else if (Opcode == Z80::LD16am || Opcode == Z80::LD16om ||
                 Opcode == Z80::LD16ri) {
        if (MI.getOperand(0).getReg() != Z80::IY &&
            MI.getOperand(0).getReg() != Z80::IX) {
          FixupKind = Z80::fixup_z80_addr16_b2;
        } else {
          FixupKind = Z80::fixup_z80_addr16_b3;
        }
      }
    }

    if (FixupKind == Z80::fixup_z80_invalid) {
      MI.dump();
    }
    assert(FixupKind != Z80::fixup_z80_invalid && "Unhandled expression!");
    //
    //    Fixups.push_back(
    //        MCFixup::create(0, Expr, MCFixupKind(FixupKind), MI.getLoc()));
    //    ++MCNumFixups;

  } else {
    llvm_unreachable("HasImm but got neither imm nor expr");
  }
}

/// encodeInstruction - Emit the instruction.
/// Size the instruction with Desc.getSize().
void Z80MCCodeEmitter::encodeInstruction(const MCInst &MI, raw_ostream &OS,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  MCInst TmpInst = MI;
  auto &MIDesc = MCII.get(MI.getOpcode());
  auto TS = MIDesc.TSFlags;

  unsigned CurrentByte = 0;

  emitZ80Prefix(MI, MIDesc, CurrentByte, OS);

  // Prefix immediate for Bit test and Set group
  emitZ80PrefixImmediate(MI, MIDesc, CurrentByte, OS);

  // Primary OpCode Area
  uint8_t PrimaryOpcode = (TS & Z80II::OpcodeMask) >> Z80II::OpcodeShift;
  patchCC(MI, PrimaryOpcode);
  patchRegister(MI, PrimaryOpcode);
  emitByte(PrimaryOpcode, CurrentByte, OS);

  // Immediate Suffix
  emitImmediate(MI, CurrentByte, OS, Fixups);

  // TODO Needfuls
}

MCCodeEmitter *createZ80MCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx) {
  return new Z80MCCodeEmitter(MCII, Ctx);
}