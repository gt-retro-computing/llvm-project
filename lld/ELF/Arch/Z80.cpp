//===- Z80.cpp ---------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "InputFiles.h"
#include "Symbols.h"
#include "Target.h"
#include "lld/Common/ErrorHandler.h"
#include "llvm/Object/ELF.h"
#include "llvm/Support/Endian.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {
class Z80 final : public TargetInfo {
public:
  Z80();
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
};
} // namespace

Z80::Z80() {

  // FIXME add trap instruction
  trapInstr = {0x00, 0x00, 0x00, 0x00};
  //  execFirst = true;
  config->imageBase = Optional<uint64_t>(0);
}

RelExpr Z80::getRelExpr(RelType type, const Symbol &s,
                        const uint8_t *loc) const {
  switch (type) {
  case R_Z80_PCREL8_B2:
    return R_PC;
  case R_Z80_8:
  case R_Z80_16:
  case R_Z80_ADDR16_B2:
  case R_Z80_ADDR16_B3:
    return R_ABS;
  default:
    llvm_unreachable("Undefined reloc type");
  }
}

void Z80::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  auto type = rel.type;
  switch (type) {
  case R_Z80_ADDR16_B2:
    val += read16le(loc + 1);
    checkIntUInt(loc, val, 16, rel);
    write16le(loc + 1, val & 0xFFFF);
    break;
  case R_Z80_ADDR16_B3:
    val += read16le(loc + 2);
    checkIntUInt(loc, val, 16, rel);
    write16le(loc + 2, val & 0xFFFF);
    break;
  case R_Z80_8:
    val += *loc;
    checkIntUInt(loc, val, 8, rel);
    *loc = (val & 0xFF);
    break;
  case R_Z80_16:
    val += read16le(loc);
    checkIntUInt(loc, val, 16, rel);
    write16le(loc, val & 0xFFFF);
    break;
  case R_Z80_PCREL8_B2: {
    val += *(loc + 1);
    int64_t fixed_val = ((int64_t)val - 2);
    checkInt(loc, fixed_val, 8, rel);
    *(loc + 1) = (uint8_t)fixed_val;
    break;
  }
  default:
    error(getErrorLocation(loc) + "unrecognized relocation " + toString(type));
  }
}

TargetInfo *elf::getZ80TargetInfo() {
  static Z80 target;
  return &target;
}
