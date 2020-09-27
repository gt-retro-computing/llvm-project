//
// Created by codetector on 9/25/20.
//

#include "Z80ELFObjectWriter.h"

Z80ELFObjectWriter::Z80ELFObjectWriter(uint8_t OSABI) : MCELFObjectTargetWriter(false, OSABI, 654, false) {}


unsigned int Z80ELFObjectWriter::getRelocType(MCContext &Ctx,
                                              const MCValue &Target,
                                              const MCFixup &Fixup,
                                              bool IsPCRel) const {

  return 0;
}

bool Z80ELFObjectWriter::needsRelocateWithSymbol(const MCSymbol &Sym,
                                                 unsigned int Type) const {
  return true;
}

std::unique_ptr<MCObjectTargetWriter> llvm::createZ80ELFObjectWriter(uint8_t OSABI) {
  return std::make_unique<Z80ELFObjectWriter>(OSABI);
}


