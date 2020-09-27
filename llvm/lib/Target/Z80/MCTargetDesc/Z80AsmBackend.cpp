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
#include "Z80MCTargetDesc.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDirectives.h"
#include "llvm/MC/MCELFObjectWriter.h"
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

  return 0;
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

}

Optional<MCFixupKind> Z80AsmBackend::getFixupKind(StringRef Name) const {
  return MCAsmBackend::getFixupKind(Name);
}

const MCFixupKindInfo &Z80AsmBackend::
getFixupKindInfo(MCFixupKind Kind) const {
    llvm_unreachable("unknown fixup");
}

/// WriteNopData - Write an (optimal) nop sequence of Count bytes
/// to the given output. If the target cannot generate such a sequence,
/// it should return an error.
///
/// \return - True on success.
bool Z80AsmBackend::writeNopData(raw_ostream &OS, uint64_t Count) const {
  // Check for a less than instruction size number of bytes
  // FIXME: 16 bit instructions are not handled yet here.
  // We shouldn't be using a hard coded number for instruction size.

  // If the count is not 4-byte aligned, we must be writing data into the text
  // section (otherwise we have unaligned instructions, and thus have far
  // bigger problems), so just write zeros instead.
  OS.write_zeros(Count);
  return true;
}

bool Z80AsmBackend::shouldForceRelocation(const MCAssembler &Asm,
                                           const MCFixup &Fixup,
                                           const MCValue &Target) {
  return false;
}

MCAsmBackend *llvm::createZ80AsmBackend(const Target &T,
                                         const MCSubtargetInfo &STI,
                                         const MCRegisterInfo &MRI,
                                         const MCTargetOptions &Options) {
  return new Z80AsmBackend(T, MRI, STI.getTargetTriple(), STI.getCPU());
}
