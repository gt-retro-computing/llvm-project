//
// Created by codetector on 9/25/20.
//

#ifndef LLVM_Z80ELFOBJECTWRITER_H
#define LLVM_Z80ELFOBJECTWRITER_H

#include "Z80MCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/MC/MCValue.h"

namespace {
using namespace llvm;
class Z80ELFObjectWriter : public MCELFObjectTargetWriter {

public:
  Z80ELFObjectWriter(uint8_t OSABI);

  virtual ~Z80ELFObjectWriter() = default;

  bool needsRelocateWithSymbol(const MCSymbol &Sym,
                               unsigned int Type) const override;

protected:
  unsigned int getRelocType(MCContext &Ctx, const MCValue &Target,
                            const MCFixup &Fixup, bool IsPCRel) const override;
};
}
#endif // LLVM_Z80ELFOBJECTWRITER_H
