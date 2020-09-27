//===- MipsMCCodeEmitter.h - Convert Mips Code to Machine Code --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file defines the MipsMCCodeEmitter class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MIPS_MCTARGETDESC_MIPSMCCODEEMITTER_H
#define LLVM_LIB_TARGET_MIPS_MCTARGETDESC_MIPSMCCODEEMITTER_H

#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/SubtargetFeature.h"
#include <cstdint>
#include <llvm/MC/MCInstrDesc.h>
#include <llvm/MC/MCRegisterInfo.h>

using namespace llvm;

namespace llvm {

class MCContext;
class MCExpr;
class MCFixup;
class MCInst;
class MCInstrInfo;
class MCOperand;
class MCSubtargetInfo;
class raw_ostream;

class Z80MCCodeEmitter : public MCCodeEmitter {
  const MCInstrInfo &MCII;
  MCContext &Ctx;

public:
  Z80MCCodeEmitter(const MCInstrInfo &mcii, MCContext &Ctx_)
      : MCII(mcii), Ctx(Ctx_) {}
  Z80MCCodeEmitter(const Z80MCCodeEmitter &) = delete;
  Z80MCCodeEmitter &operator=(const Z80MCCodeEmitter &) = delete;
  ~Z80MCCodeEmitter() override = default;

  void emitByte(unsigned char C, unsigned int &CurByte, raw_ostream &OS) const;

  void encodeInstruction(const MCInst &MI, raw_ostream &OS,
                         SmallVectorImpl<MCFixup> &Fixups,
                         const MCSubtargetInfo &STI) const override;
  void emitZ80Prefix(const MCInst &MI, const MCInstrDesc &MIDesc,
                     unsigned int &CurByte, raw_ostream &OS) const;
  void emitImmediate(const MCInst &MI, unsigned &CurByte, raw_ostream &OS) const;

  void patchCC(const MCInst &MI, uint8_t &PrimaryOpcode) const;
  void patchRegister(const MCInst &MI, uint8_t &PrimaryOpcode) const;

  uint8_t getZ80RegisterEncoding(const MCInst &MI,
                                 unsigned int RegisterNo) const;
  void emitWordLE(unsigned short C, unsigned int &CurByte,
                  raw_ostream &OS) const;
};

MCCodeEmitter *createZ80MCCodeEmitter(const MCInstrInfo &MCII,
                                      const MCRegisterInfo &MRI,
                                      MCContext &Ctx);
} // end namespace llvm
#endif // LLVM_LIB_TARGET_MIPS_MCTARGETDESC_MIPSMCCODEEMITTER_H
