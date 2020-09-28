//===------ Z80Disassembler.cpp - Disassembler for Z80 --------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the RISCVDisassembler class.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "TargetInfo/Z80TargetInfo.h"
#include "llvm/CodeGen/Register.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

#define DEBUG_TYPE "z80-disassembler"

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {
class Z80Disassembler : public MCDisassembler {
  std::unique_ptr<MCInstrInfo const> const MCII;

public:
  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &CStream) const override;

  Z80Disassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                  MCInstrInfo const *MCII)
      : MCDisassembler(STI, Ctx), MCII(MCII) {}
  DecodeStatus decodeOpcode(MCInst &Instr, uint64_t &Size,
                            ArrayRef<uint8_t> Bytes, uint64_t Address) const;
};

enum class Z80DisAsmPrefixState {
  NoPrefix,
  DDPrefix, // IX
  FDPrefix, // IY
};

} // end anonymous namespace

static MCDisassembler *createZ80Disassembler(const Target &T,
                                             const MCSubtargetInfo &STI,
                                             MCContext &Ctx) {
  return new Z80Disassembler(STI, Ctx, T.createMCInstrInfo());
}

extern "C" LLVM_EXTERNAL_VISIBILITY void LLVMInitializeZ80Disassembler() {
  // Register the disassembler for each target.
  TargetRegistry::RegisterMCDisassembler(getTheZ80Target(),
                                         createZ80Disassembler);
//  TargetRegistry::RegisterMCDisassembler(getTheRISCV64Target(),
//                                         createRISCVDisassembler);
}

DecodeStatus Z80Disassembler::decodeOpcode(MCInst &Instr, uint64_t &Size, ArrayRef<uint8_t> Bytes, uint64_t Address) const {

  bool Parsed = true;

  switch(Bytes[Size]) {
  default:
    Parsed = false;
    break;
  }
  return Parsed ? Success : Fail;
}

DecodeStatus Z80Disassembler::getInstruction(MCInst &Instr, uint64_t &Size,
                                             ArrayRef<uint8_t> Bytes,
                                             uint64_t Address,
                                             raw_ostream &CStream) const {
  Size = 0;
  Z80DisAsmPrefixState PrefixState = Z80DisAsmPrefixState::NoPrefix;

  switch (Bytes[0]) { // Look at prefix
  case 0xDD:
    PrefixState = Z80DisAsmPrefixState::DDPrefix; Size += 1; break;
  case 0xFD:
    PrefixState = Z80DisAsmPrefixState::FDPrefix; Size += 1; break;
  default:
    break;
  }

  return Fail;
}
