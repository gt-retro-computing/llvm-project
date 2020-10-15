#include "MCTargetDesc/Z80MCTargetDesc.h"
#include "Z80.h"
#include "Z80InstrInfo.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

#define Z80_EXPAND_PSEUDO_INST_PASS_NAME "Z80 Pseudo Expansion"

namespace {
class Z80ExpandPseudoInst : public MachineFunctionPass {
public:
  static char ID;

  Z80ExpandPseudoInst() : MachineFunctionPass(ID) {
    initializeZ80ExpandPseudoInstPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override {
    return Z80_EXPAND_PSEUDO_INST_PASS_NAME;
  }
};

char Z80ExpandPseudoInst::ID = 0;

bool Z80ExpandPseudoInst::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  auto const *II = MF.getTarget().getMCInstrInfo();
  assert(II);

  auto Is24BitMode =
      !MF.getTarget().getTargetTriple().isArch16Bit() &&
      MF.getTarget().getTargetTriple().getEnvironment() != Triple::CODE16;

  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      switch (MI.getOpcode()) {
      case Z80::JQ:
        MI.setDesc(II->get(Is24BitMode ? Z80::JP24 : Z80::JP16));
        Changed = true;
        break;
      case Z80::JQCC:
        MI.setDesc(II->get(Is24BitMode ? Z80::JP24CC : Z80::JP16CC));
        Changed = true;
        break;
      case Z80::LD8pg:
      {
        auto Reg = MI.getOperand(0).getReg();
        if (Reg == Z80::IX || Reg == Z80::IY) {
          MI.setDesc(II->get(Z80::LD8og));
          MachineOperand backup = MI.getOperand(1);
          MI.RemoveOperand(1);
          MI.addOperand(MachineOperand::CreateImm(0));
          MI.addOperand(backup);
        }
        break;
      }
      case Z80::LD8gp: // FIXME: This is "technically" a bug. IX/IY -> go
      {
        auto Reg = MI.getOperand(1).getReg();
        if (Reg == Z80::IX || Reg == Z80::IY) {
          MI.setDesc(II->get(Z80::LD8go));
          MI.addOperand(MachineOperand::CreateImm(0));
        }
        break;
      }
      case Z80::LD8pi:
      {
        auto Reg = MI.getOperand(0).getReg();
        if (Reg == Z80::IX || Reg == Z80::IY) {
          MI.setDesc(II->get(Z80::LD8oi));
          auto Val = MI.getOperand(1);
          MI.RemoveOperand(1);
          MI.addOperand(MachineOperand::CreateImm(0));
          MI.addOperand(Val);
        }
        break;
      }
      }
    }
  }

  return Changed;
}

} // end of anonymous namespace

INITIALIZE_PASS(Z80ExpandPseudoInst, "z80-late-pseudo-expansion",
                Z80_EXPAND_PSEUDO_INST_PASS_NAME, false, false)
namespace llvm {

FunctionPass *createZ80ExpandPseudoInstPass() {
  return new Z80ExpandPseudoInst();
}

} // namespace llvm