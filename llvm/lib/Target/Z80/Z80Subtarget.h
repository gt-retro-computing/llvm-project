//===-- Z80Subtarget.h - Define Subtarget for the Z80 ----------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the Z80 specific subclass of TargetSubtargetInfo.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_Z80_Z80SUBTARGET_H
#define LLVM_LIB_TARGET_Z80_Z80SUBTARGET_H

#include "Z80FrameLowering.h"
#include "Z80InstrInfo.h"
#include "Z80ISelLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"

#define GET_SUBTARGETINFO_HEADER
#include "Z80GenSubtargetInfo.inc"

namespace llvm {
class Z80TargetMachine;

class Z80Subtarget final : public Z80GenSubtargetInfo {
  /// What processor and OS we're targeting.
  Triple TargetTriple;

  /// True if compiling for 16-bit, false for 24-bit.
  bool In16BitMode;

  /// True if compiling for 24-bit, false for 16-bit.
  bool In24BitMode;

  /// True if target has undocumented z80 instructions.
  bool HasUndocOps = false;

  /// True if target has z180 instructions.
  bool HasZ180Ops = false;

  /// True if target has ez80 instructions.
  bool HasEZ80Ops = false;

  /// True if target has index half registers (HasUndocOps || HasEZ80Ops).
  bool HasIdxHalfRegs = false;

  // Ordering here is important. Z80InstrInfo initializes Z80RegisterInfo which
  // Z80TargetLowering needs.
  Z80InstrInfo InstrInfo;
  Z80TargetLowering TLInfo;
  Z80FrameLowering FrameLowering;

public:
  /// This constructor initializes the data members to match that
  /// of the specified triple.
  Z80Subtarget(const Triple &TT, StringRef CPU, StringRef TuneCPU, StringRef FS,
               const Z80TargetMachine &TM);

  const Z80TargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }
  const Z80InstrInfo *getInstrInfo() const override { return &InstrInfo; }
  const Z80FrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const Z80RegisterInfo *getRegisterInfo() const override {
    return &getInstrInfo()->getRegisterInfo();
  }

  /// ParseSubtargetFeatures - Parses features string setting specified
  /// subtarget options.  Definition of function is auto generated by tblgen.
  void ParseSubtargetFeatures(StringRef CPU, StringRef TuneCPU, StringRef FS);

private:
  Z80Subtarget &initializeSubtargetDependencies(StringRef CPU,
                                                StringRef TuneCPU,
                                                StringRef FS);
  void initializeEnvironment();

public:
  const Triple &getTargetTriple() const { return TargetTriple; }
  /// Is this ez80 (disregarding specific ABI / programming model)
  bool is24Bit() const { return In24BitMode; }
  bool is16Bit() const { return In16BitMode; }
  bool hasUndocOps() const { return HasUndocOps; }
  bool hasZ180Ops() const { return HasZ180Ops; }
  bool hasEZ80Ops() const { return HasEZ80Ops; }
  bool hasIndexHalfRegs() const { return HasIdxHalfRegs; }
  bool has24BitEZ80Ops() const { return is24Bit() && hasEZ80Ops(); }
  bool has16BitEZ80Ops() const { return is16Bit() && hasEZ80Ops(); }
};
} // namespace llvm

#endif
