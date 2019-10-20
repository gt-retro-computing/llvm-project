//===-- Z80TargetInfo.h - Z80 Target Implementation -------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80_TARGETINFO_Z80TARGETINFO_H
#define LLVM_LIB_TARGET_Z80_TARGETINFO_Z80TARGETINFO_H

namespace llvm {

class Target;

Target &getTheZ80Target();
Target &getTheEZ80Target();

}

#endif // LLVM_LIB_TARGET_Z80_TARGETINFO_Z80TARGETINFO_H
