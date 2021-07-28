#pragma once
#include <functional>
#include <unordered_set>

#include "../arm.h"
#include "../arm_struct.h"
#include "pass_manager.h"
using namespace arm;

class GenerateArm : public Transform {
 public:
  using VarScopeRegPtrHashTable = std::unordered_map<std::string, std::unordered_map<int, Reg*>>;
  GenerateArm(Module** m) : Transform(m) {}
  virtual ArmModule* GenCode(IRModule* module);
  // will change *m: IRModule -> ArmModule
  // remember release source *m space
  virtual void Run() override;

 protected:
  // these data is valid in one function.
  // they should be reset at the beginning of every function.
  int virtual_reg_id = 16;
  // 全局 局部 中间变量的占用寄存器(存放值)map
  // 局部变量不需要存在var_map中 是无意义的 每次用到的时候都要ldr str
  VarScopeRegPtrHashTable var_map;  // varname, scope_id, reg
  // 保存有初始sp的vreg
  // 保存sp到vreg中  add rx, sp, 0
  // 之后要用sp的地方都找sp_vreg 除了call附近 和 epilogue中
  Reg* sp_vreg = nullptr;
  int stack_size = 0;
  int arg_num = 0;
  std::vector<Instruction*> sp_arg_fixup;
  std::vector<Instruction*> sp_fixup;

 protected:
  // used for generate arm code.
  Cond GetCondType(ir::IR::OpKind opkind, bool exchange = false);
  Reg* NewVirtualReg();
  Operand2* ResolveImm2Operand2(ArmBasicBlock* armbb, int imm, bool record = false);
  Operand2* ResolveOpn2Operand2(ArmBasicBlock* armbb, ir::Opn* opn);
  void GenImmLdrStrInst(ArmBasicBlock* armbb, LdrStr::OpKind opkind, Reg* rd, Reg* rn, int imm);
  void GenCallCode(ArmBasicBlock* armbb, ir::IR& ir, std::vector<ir::IR*>::iterator ir_iter);
  void AddEpilogue(ArmBasicBlock* armbb);

  virtual void ResetFuncData(ArmFunction* armfunc);
  virtual void ChangeOffset(std::string& func_name);
  virtual void AddPrologue(ArmFunction* func);
  virtual Reg* LoadGlobalOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn);
  virtual Reg* GetRBase(ArmBasicBlock* armbb, ir::Opn* opn);
  virtual Reg* ResolveOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn);
  virtual void ResolveResOpn2RdReg(ArmBasicBlock* armbb, ir::Opn* opn, std::function<void(Reg*)> f);
};