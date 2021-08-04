#pragma once
#include <functional>
#include <unordered_set>

#include "../arm.h"
#include "../arm_struct.h"
#include "../ssa.h"
#include "../ssa_struct.h"
#include "pass_manager.h"
class SSAModule;
class Value;
using namespace arm;
// NO OPT VERSION
class GenerateArmFromSSA : public Transform {
 public:
  GenerateArmFromSSA(Module** m) : Transform(m) {}
  // will change *m: SSAModule -> ArmModule
  void Run();
  ArmModule* GenCode(SSAModule* module);

 protected:
  // these data is valid in one function. they should be reset at the beginning of every function.
  int virtual_reg_id = 16;
  // 全局 局部 中间变量的占用寄存器(存放值)map
  std::unordered_map<Value*, Reg*> var_map;  // varname, scope_id, reg
  // 保存有初始sp的vreg 一定是也必须是r16 maybe not used. FIXME
  Reg* sp_vreg = nullptr;
  int stack_size = 0;
  std::vector<Instruction*> sp_arg_fixup;
  std::vector<Instruction*> sp_fixup;

 protected:
  // used for generate arm code.
  Cond GetCondType(BranchInst::Cond cond, bool exchange = false);
  Reg* NewVirtualReg();
  Reg* ResolveValue2Reg(ArmBasicBlock* armbb, Value* val);
  Reg* ResolveImm2Reg(ArmBasicBlock* armbb, int imm, bool record = false);
  Operand2* ResolveValue2Operand2(ArmBasicBlock* armbb, Value* val);
  Operand2* ResolveImm2Operand2(ArmBasicBlock* armbb, int imm, bool record = false);
  void GenImmLdrStrInst(ArmBasicBlock* armbb, LdrStr::OpKind opkind, Reg* rd, Reg* rn, int imm, bool record = false);
  void AddEpilogue(ArmBasicBlock* armbb);
  void AddPrologue(ArmFunction* func, FunctionValue* func_val);
  void ResetFuncData();
  void GenerateArmBasicBlocks(ArmFunction* armfunc, SSAFunction* func,
                              std::unordered_map<SSABasicBlock*, ArmBasicBlock*>& bb_map);
};