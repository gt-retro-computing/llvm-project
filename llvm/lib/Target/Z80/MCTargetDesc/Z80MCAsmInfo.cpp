//===-- Z80MCAsmInfo.cpp - Z80 asm properties -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of the Z80MCAsmInfo properties.
//
//===----------------------------------------------------------------------===//

#include "Z80MCAsmInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"
using namespace llvm;

void Z80MCAsmInfo::anchor() {}

Z80MCAsmInfo::Z80MCAsmInfo(const Triple &T) {
  bool Is16Bit = T.isArch16Bit() || T.getEnvironment() == Triple::CODE16;
  CodePointerSize = CalleeSaveStackSlotSize = Is16Bit ? 2 : 4;
  MaxInstLength = 6;
  DollarIsPC = true;
  CommentString = ";";

  PrivateGlobalPrefix = "_g";
  PrivateLabelPrefix = "_p";
  LinkerPrivateGlobalPrefix = "lp_";
  IsLittleEndian = true;
  AssemblerDialect = !Is16Bit;
  SupportsQuotedNames = false;
  HasFunctionAlignment = false;
  HasDotTypeDotSizeDirective = false;
  WeakDirective = "\tweak\t";
  UseIntegratedAssembler = true;
  UseLogicalShr = false;
  HasSingleParameterDotFile = false;
  DwarfFileDirective = "\tfile\t";
  DwarfLocDirective = "\tloc\t";
  NeedsLocalForSize = false;
  HasIdentDirective = false;

  // Debug Information
  SupportsDebugInformation = true;

  // Exceptions handling
  ExceptionsType = ExceptionHandling::SjLj;
}

const char *Z80MCAsmInfo::getBlockDirective(int64_t Size) const {
  switch (Size) {
  default:
    return nullptr;
  case 1:
    return "\tdb\t";
  case 2:
    return "\tdw\t";
  case 3:
    return "\tdl\t";
  case 4:
    return "\tdd\t";
  }
}
