#include "../arm.h"
#include "../arm_struct.h"
#include "pass_manager.h"
using namespace arm;

class GenerateArm : public Transform {
 public:
  using VarScopeRegPtrHashTable = std::unordered_map<std::string, std::unordered_map<int, Reg*>>;
  using GloVarRegPtrHashTable = std::unordered_map<std::string, Reg*>;
  GenerateArm(Module** m) : Transform(m) {}
  ArmModule* GenCode(IRModule* module);
  // will change *m: IRModule -> ArmModule
  // remember release source *m space
  void Run();

 private:
  // these data is valid in one function.
  // they should be reset at the beginning of every function.
  int virtual_reg_id = 16;
  // 全局 局部 中间变量的占用寄存器(存放值)map
  // 局部变量不需要存在var_map中 是无意义的 每次用到的时候都要ldr str
  VarScopeRegPtrHashTable var_map;  // varname, scope_id, reg
  // 全局变量的地址寄存器map
  GloVarRegPtrHashTable glo_addr_map;
  // 保存有初始sp的vreg
  // 保存sp到vreg中  add rx, sp, 0
  // 之后要用sp的地方都找sp_vreg 除了call附近 和 epilogue中
  Reg* sp_vreg = nullptr;
  int stack_size = 0;
  int arg_num = 0;

  void ResetFuncData(ArmFunction* armfunc);

 private:
  // used for generate arm code.
  Cond GetCondType(ir::IR::OpKind opkind, bool exchange = false);
  Reg* NewVirtualReg();
  Operand2* ResolveImm2Operand2(ArmBasicBlock* armbb, int imm);
  void AddPrologue(ArmBasicBlock* first_bb);
  void AddEpilogue(ArmBasicBlock* armbb);
  void LoadGlobalOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn);
  Reg* ResolveOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn);
  Operand2* ResolveOpn2Operand2(ArmBasicBlock* armbb, ir::Opn* opn);
  void ResolveResOpn2RdRegWithBiInst(ArmBasicBlock* armbb, ir::Opn* opn, BinaryInst::OpCode opcode, Reg* rn,
                                     Operand2* op2);
  template <typename CallableObjTy>
  void ResolveResOpn2RdReg(ArmBasicBlock* armbb, ir::Opn* opn, CallableObjTy f);
};