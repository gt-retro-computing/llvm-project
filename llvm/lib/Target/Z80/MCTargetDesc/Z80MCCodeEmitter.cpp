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
#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "Z80InstrInfo.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/SmallVector.h"
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

using namespace llvm;

#define DEBUG_TYPE "mccodeemitter"

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

void Z80MCCodeEmitter::emitZ80Prefix(const MCInst &MI, MCInstrDesc &MIDesc,
                                  unsigned &CurByte, raw_ostream &OS) const {
  auto TS = MIDesc.TSFlags;
  auto Prefix = (TS >> Z80II::PrefixShift) & Z80II::PrefixMask;
  bool IndexedIndexPrefix =
      (TS >> Z80II::PrefixShift) & Z80II::IndexedIndexPrefix;

  assert(!IndexedIndexPrefix);
}

void Z80MCCodeEmitter::patchCC(const MCInst &MI, uint8_t &PrimaryOpcode) const {
  auto Opcode = MI.getOpcode();
  if (Opcode == Z80::JP16CC) {
    assert(MI.getOperand(1).isImm());
    auto CC = MI.getOperand(1).getImm();
    assert(CC <= 0b111 && "CC is in range");
    PrimaryOpcode |= CC << 3;
    return;
  } else {
    llvm_unreachable("Invalid Opcode for patchCC");
  }
}

/// encodeInstruction - Emit the instruction.
/// Size the instruction with Desc.getSize().
void Z80MCCodeEmitter::encodeInstruction(const MCInst &MI, raw_ostream &OS,
                                         SmallVectorImpl<MCFixup> &Fixups,
                                         const MCSubtargetInfo &STI) const {
  // Non-pseudo instructions that get changed for direct object
  // only based on operand values.
  // If this list of instructions get much longer we will move
  // the check to a function call. Until then, this is more efficient.
  MCInst TmpInst = MI;
  auto &MCInstDesc = MCII.get(MI.getOpcode());
  auto TS = MCInstDesc.TSFlags;

  unsigned CurrentByte = 0;

  uint8_t PrimaryOpcode = (TS & Z80II::OpcodeMask) >> Z80II::OpcodeShift;

  switch (MI.getOpcode()) {
  case Z80::JP16CC:
    patchCC(MI, PrimaryOpcode);
    break;
  default:
    break;
  }

  emitByte(PrimaryOpcode, CurrentByte, OS);

  // TODO Needfuls
}

MCCodeEmitter *createZ80MCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx) {
  return new Z80MCCodeEmitter(MCII, Ctx);
}