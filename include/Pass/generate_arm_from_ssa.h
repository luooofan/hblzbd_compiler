#ifndef __GENERATE_ARM_FROM_SSA_H__
#define __GENERATE_ARM_FROM_SSA_H__

#include <functional>
#include <unordered_set>
#include <vector>

#include "../arm.h"
#include "../ssa.h"
#include "../ssa_struct.h"
#include "pass_manager.h"
class SSAModule;
class Value;
class ArmBasicBlock;
class ArmFunction;
class ArmModule;
using namespace arm;

class GenerateArmFromSSA : public Transform {
 public:
  GenerateArmFromSSA(Module** m);
  // will change *m: SSAModule -> ArmModule
  void Run();
  ArmModule* GenCode(SSAModule* module);

 protected:
  // these data is valid in one function. they should be reset at the beginning of every function.
  std::vector<Reg*> machine_regs;
  int virtual_reg_id = 16;
  std::unordered_map<Value*, Reg*> var_map;
  int stack_size = 0;
  std::unordered_set<Instruction*> sp_arg_fixup;
  std::unordered_set<Instruction*> sp_fixup;
  ArmBasicBlock* prologue = nullptr;
  ArmBasicBlock* epilogue = nullptr;

 protected:
  // used for generate arm code.
  Cond GetCondType(BranchInst::Cond cond, bool exchange = false);
  Reg* NewVirtualReg();
  Reg* ResolveValue2Reg(ArmBasicBlock* armbb, Value* val);
  Reg* ResolveImm2Reg(ArmBasicBlock* armbb, int imm, bool record = false);
  Operand2* ResolveValue2Operand2(ArmBasicBlock* armbb, Value* val);
  Operand2* ResolveImm2Operand2(ArmBasicBlock* armbb, int imm, bool record = false);
  void GenImmLdrStrInst(ArmBasicBlock* armbb, LdrStr::OpKind opkind, Reg* rd, Reg* rn, int imm, bool record = false);
  bool ConvertMul2Shift(ArmBasicBlock* armbb, Reg* rd, Value* val, int imm);
  bool ConvertDiv2Shift(ArmBasicBlock* armbb, Reg* rd, Value* val, int imm);
  bool ConvertMod2And(ArmBasicBlock* armbb, Reg* rd, Value* val, int imm);
  void AddEpilogue(ArmBasicBlock* armbb);
  void AddPrologue(ArmFunction* func, FunctionValue* func_val);
  void AddEpilogue(ArmFunction* func);
  void ResetFuncData();
  void GenerateArmBasicBlocks(ArmFunction* armfunc, SSAFunction* func,
                              std::unordered_map<SSABasicBlock*, ArmBasicBlock*>& bb_map);
};

#endif