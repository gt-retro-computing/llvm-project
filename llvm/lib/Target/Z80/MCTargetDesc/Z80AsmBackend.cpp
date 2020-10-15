//===-- Z80AsmBackend.cpp - Mips Asm Backend  ----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements the Z80AsmBackend class.
//
//===----------------------------------------------------------------------===//
//

#include "Z80AsmBackend.h"
#include "Z80FixupKinds.h"
#include "Z80MCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCFixup.h"
#include "llvm/MC/MCFixupKindInfo.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

// Prepare value for the target space for it
static unsigned adjustFixupValue(const MCFixup &Fixup, uint64_t Value,
                                 MCContext &Ctx) {
  switch (Fixup.getKind()) {
  default:
    llvm_unreachable("Unsupported Fixup");
  case Z80::fixup_z80_pcrel8_b2:
//    if (!isInt<8>(((int64_t)Value) - 1))
//      Ctx.reportError(Fixup.getLoc(), "PC Rel8 out of range");
    return Value;
  case FK_Data_1:
    if (!isUInt<8>(Value))
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range of u16");
    return Value & 0xFF;
  case Z80::fixup_z80_addr16_b2:
  case Z80::fixup_z80_addr16_b3:
  case FK_Data_2:
    if (!isUInt<16>(Value))
      Ctx.reportError(Fixup.getLoc(), "fixup value out of range of u16");
    return Value & 0xFFFF;
  }
  return Value;
}

std::unique_ptr<MCObjectTargetWriter>
Z80AsmBackend::createObjectTargetWriter() const {
  return createZ80ELFObjectWriter(0);
}

/// ApplyFixup - Apply the \p Value for given \p Fixup into the provided
/// data fragment, at the offset specified by the fixup and following the
/// fixup kind as appropriate.
void Z80AsmBackend::applyFixup(const MCAssembler &Asm, const MCFixup &Fixup,
                               const MCValue &Target,
                               MutableArrayRef<char> Data, uint64_t Value,
                               bool IsResolved,
                               const MCSubtargetInfo *STI) const {
  auto &Ctx = Asm.getContext();
  auto Info = getFixupKindInfo(Fixup.getKind());

  if (Value == 0)
    return; // Since we substitute zero anyways.

  Value = adjustFixupValue(Fixup, Value, Ctx);


  Value <<= Info.TargetOffset;

  auto OffsetInInstruction = Info.TargetOffset;
  auto FixupSize = alignTo(Info.TargetSize + Info.TargetOffset, 8) / 8;

  auto InstructionOffset = Fixup.getOffset();

  assert(InstructionOffset + FixupSize <= Data.size() &&
         "Invalid fixup offset!");

  for (unsigned i = 0; i != FixupSize; ++i) {
    Data[InstructionOffset + i] |= uint8_t((Value >> (i * 8)) & 0xff);
  }
}

const MCFixupKindInfo &Z80AsmBackend::getFixupKindInfo(MCFixupKind Kind) const {
  const static MCFixupKindInfo Infos[] = {
      // This table *must* be in the order that the fixup_* kinds are defined in
      // Z80FixupKinds.h.
      //
      // name                      offset  bits  flags
      {"fixup_z80_addr16_b2", 8, 16, 0},
      {"fixup_z80_addr16_b3", 16, 16, 0},
      {"fixup_z80_pcrel8_b2", 8, 8, 0},
  };
  static_assert((array_lengthof(Infos)) == Z80::NumTargetFixupKinds,
                "Not all fixup kinds added to Infos array");

  if (Kind < FirstTargetFixupKind)
    return MCAsmBackend::getFixupKindInfo(Kind);

  assert(unsigned(Kind - FirstTargetFixupKind) < getNumFixupKinds() &&
         "Invalid kind!");
  return Infos[Kind - FirstTargetFixupKind];
}

/// WriteNopData - Write an (optimal) nop sequence of Count bytes
/// to the given output. If the target cannot generate such a sequence,
/// it should return an error.
///
/// \return - True on success.
bool Z80AsmBackend::writeNopData(raw_ostream &OS, uint64_t Count) const {
  OS.write_zeros(Count);
  return true;
}

unsigned Z80AsmBackend::getNumFixupKinds() const {
  return Z80::Fixups::NumTargetFixupKinds;
}

MCAsmBackend *llvm::createZ80AsmBackend(const Target &T,
                                        const MCSubtargetInfo &STI,
                                        const MCRegisterInfo &MRI,
                                        const MCTargetOptions &Options) {
  return new Z80AsmBackend(T, MRI, STI.getTargetTriple(), STI.getCPU());
}
