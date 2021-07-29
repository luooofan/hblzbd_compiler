#pragma once

#include "./generate_arm.h"
using namespace arm;

class GenerateArmOpt : public GenerateArm {
 public:
  GenerateArmOpt(Module** m) : GenerateArm(m) {}

 private:
  // OPT:
  std::vector<int> arg_reg;

  // used for generate arm code. different from no-opt version.
  void ResetFuncData(ArmFunction* armfunc) override;
  void ChangeOffset(std::string& func_name) override;
  void AddPrologue(ArmFunction* func) override;
  Reg* LoadGlobalOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) override;
  Reg* GetRBase(ArmBasicBlock* armbb, ir::Opn* opn) override;
  Reg* ResolveOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) override;
  void ResolveResOpn2RdReg(ArmBasicBlock* armbb, ir::Opn* opn, std::function<void(Reg*)> f) override;
};