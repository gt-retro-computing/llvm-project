#include "Z80.h"
#include "Z80InstrInfo.h"
#include "MCTargetDesc/Z80MCTargetDesc.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/MC/MCInstrInfo.h"

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

  StringRef getPassName() const override { return Z80_EXPAND_PSEUDO_INST_PASS_NAME; }
};

char Z80ExpandPseudoInst::ID = 0;

bool Z80ExpandPseudoInst::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  auto const *II = MF.getTarget().getMCInstrInfo();
  assert(II);

  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      switch (MI.getOpcode()) {
      case Z80::JQ:
        MI.setDesc(II->get(Z80::JP16));
        Changed = true;
        break;
      case Z80::JQCC:
        MI.setDesc(II->get(Z80::JP16CC));
        Changed = true;
      }
    }
  }

  return Changed;
}

} // end of anonymous namespace

INITIALIZE_PASS(Z80ExpandPseudoInst, "z80-late-pseudo-expansion",
                Z80_EXPAND_PSEUDO_INST_PASS_NAME, false, false)
namespace llvm {

FunctionPass *createZ80ExpandPseudoInstPass() { return new Z80ExpandPseudoInst(); }

} // namespace llvm