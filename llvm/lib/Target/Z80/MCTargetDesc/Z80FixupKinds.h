//
// Created by codetector on 9/27/20.
//

#ifndef LLVM_Z80FIXUPKINDS_H
#define LLVM_Z80FIXUPKINDS_H

#include "llvm/MC/MCFixup.h"

namespace llvm {
namespace Z80 {
enum Fixups {
  fixup_z80_addr16_b2 = FirstTargetFixupKind,
  // fixup_tl45_invalid - used as a sentinel and a marker, must be last fixup
  fixup_z80_invalid,
  NumTargetFixupKinds = fixup_z80_invalid - FirstTargetFixupKind
};
} // end namespace Z80
} // end namespace llvm

#endif // LLVM_Z80FIXUPKINDS_H
