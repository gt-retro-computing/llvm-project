//
// Created by codetector on 9/25/20.
//

#include "Z80ELFObjectWriter.h"
#include "Z80FixupKinds.h"

Z80ELFObjectWriter::Z80ELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(false, OSABI, ELF::EM_Z80, false) {}

unsigned int Z80ELFObjectWriter::getRelocType(MCContext &Ctx,
                                              const MCValue &Target,
                                              const MCFixup &Fixup,
                                              bool IsPCRel) const {
  // Determine the type of the relocation
  unsigned Kind = Fixup.getKind();

  if (IsPCRel) {
    llvm_unreachable("invalid fixup kind! (PcRel)");
  }

  switch (Kind) {
  default:
    llvm_unreachable("invalid fixup kind!");
  case Z80::fixup_z80_addr16_b2:
    return ELF::R_Z80_ADDR16_B2;
  }

  return 0;
}

bool Z80ELFObjectWriter::needsRelocateWithSymbol(const MCSymbol &Sym,
                                                 unsigned int Type) const {
  return true;
}

std::unique_ptr<MCObjectTargetWriter>
llvm::createZ80ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<Z80ELFObjectWriter>(OSABI);
}
