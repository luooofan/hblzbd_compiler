#include "../../include/Pass/generate_arm_opt.h"

#include <algorithm>
#include <functional>

#define ASSERT_ENABLE
#include "../../include/myassert.h"

using namespace arm;
#define NEW_INST(inst) static_cast<Instruction*>(new inst)
#define ADD_NEW_INST(inst) armbb->inst_list_.push_back(static_cast<Instruction*>(new inst));
// OPT GEN ARM

Reg* GenerateArmOpt::LoadGlobalOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) {
  MyAssert(0 == opn->scope_id_);
  // 如果是全局变量就要重新ldr
  Reg* rglo = NewVirtualReg();
  // 把全局量基址ldr到某寄存器中
  ADD_NEW_INST(LdrPseudo(rglo, opn->name_));
  return rglo;
};

Reg* GenerateArmOpt::GetRBase(ArmBasicBlock* armbb, ir::Opn* opn) {
  Reg* rbase = nullptr;
  if (0 == opn->scope_id_) {
    rbase = LoadGlobalOpn2Reg(armbb, opn);
  } else {
    auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
    if (!symbol.IsArg()) {
      rbase = NewVirtualReg();
      auto op2 = ResolveImm2Operand2(armbb, symbol.stack_offset_);
      ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, rbase, sp_vreg, op2));
    } else {  // OPT: 等于-1说明是int array参数 已经存在某个vreg中
      rbase = var_map[opn->name_][opn->scope_id_];
    }
  }
  return rbase;
}

Reg* GenerateArmOpt::ResolveOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) {
  if (opn->type_ == ir::Opn::Type::Imm) {  // mov rd(vreg) op2
    Reg* rd = NewVirtualReg();
    auto op2 = ResolveImm2Operand2(armbb, opn->imm_num_);
    ADD_NEW_INST(Move(false, Cond::AL, rd, op2));
    return rd;
  } else if (opn->type_ == ir::Opn::Type::Array) {
    // Array 返回一个存着地址的寄存器 先把基址放到rbase中 再(add, rv, rbase, offset) 返回rv
    Reg* rbase = GetRBase(armbb, opn);
    Reg* vreg = NewVirtualReg();
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, vreg, rbase, ResolveOpn2Operand2(armbb, opn->offset_)));
    return vreg;
  } else {
    if (0 == opn->scope_id_) {  // 如果是全局变量 一定有一条ldr
      Reg* vadr = LoadGlobalOpn2Reg(armbb, opn);
      Reg* vreg = NewVirtualReg();
      this->GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, vreg, vadr, 0);
      return vreg;
    }
    auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
    // OPT: 局部变量被视为中间变量 找到直接返回 没找到新建一个并绑定
    const auto& iter = var_map.find(opn->name_);
    if (iter != var_map.end()) {
      const auto& iter2 = (*iter).second.find(opn->scope_id_);
      if (iter2 != (*iter).second.end()) {
        return (*iter2).second;
      }
    }
    // NOTE: 如果没找到的话可能是未定义先使用
    Reg* vreg = NewVirtualReg();
    var_map[opn->name_][opn->scope_id_] = vreg;
    return vreg;
  }
};

void GenerateArmOpt::ResolveResOpn2RdReg(ArmBasicBlock* armbb, ir::Opn* opn, std::function<void(Reg*)> f) {
  // 只能是Var类型 Assign不调用此函数
  MyAssert(opn->type_ == ir::Opn::Type::Var);
  Reg* rd = nullptr;
  // OPT: 局部变量和中间变量行为一致
  auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
  const auto& iter = var_map.find(opn->name_);
  if (iter != var_map.end()) {
    const auto& iter2 = (*iter).second.find(opn->scope_id_);
    if (iter2 != (*iter).second.end()) {
      rd = (*iter2).second;
    }
  }
  if (nullptr == rd) {
    rd = NewVirtualReg();
    var_map[opn->name_][opn->scope_id_] = rd;
  }
  // call func
  // std::clog << typeid(f).name() << std::endl;
  f(rd);
  // 否则还要生成一条str指令
  if (opn->scope_id_ == 0) {
    auto vadr = LoadGlobalOpn2Reg(armbb, opn);
    this->GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, vadr, 0);
  }
  // OPT: 局部变量和中间变量行为一致 不用再生成str指令
}

// OPT: 必须在add prologue之后调用 NOTE: different from noopt
void GenerateArmOpt::ChangeOffset(std::string& func_name) {
  // 把符号表里的偏移都改了
  // 对于局部int array量改为stack_size-offset-array.width[0]
  // 首先拿到函数的作用域id
  const auto& iter = ir::gFuncTable.find(func_name);
  MyAssert(iter != ir::gFuncTable.end());
  int func_scope_id = iter->second.scope_id_;
  // 遍历所有作用域 修改偏移 只保留局部数组 重新计算偏移
  // OPT: 把所有参数放到寄存器里 把所有局部变量视为中间变量
  for (auto& scope : ir::gScopes) {
    if (scope.IsSubScope(func_scope_id)) {
      for (auto& item : scope.symbol_table_) {
        auto& symbol = item.second;
        if (symbol.IsTemp()) {        // 中间变量
          symbol.stack_offset_ = -1;  // useless
        } else if (symbol.IsArg()) {  // 参数
          // OPT: 维护varmap args...
          MyAssert(scope.scope_id_ == func_scope_id);
          var_map[item.first][scope.scope_id_] = new Reg(this->arg_reg[symbol.offset_ / 4]);
          symbol.offset_ = -1;
        } else if (symbol.is_array_) {  // 非参数局部数组量
          symbol.stack_offset_ = this->stack_size - (symbol.offset_ - 4 * this->arg_num) - symbol.width_[0];
        } else {  // 非参数局部非数组量
          symbol.stack_offset_ = -1;
        }
      }
    }
  }
  // ir::PrintScopes(std::cout);
}

void GenerateArmOpt::AddPrologue(ArmFunction* func) {
  // push {lr}
  auto armbb = func->bb_list_.front();
  auto push_inst = new PushPop(PushPop::OpKind::PUSH);
  push_inst->reg_list_.push_back(new Reg(ArmReg::lr));
  armbb->inst_list_.push_back(static_cast<Instruction*>(push_inst));
  // sub sp,sp,#stack_size
  auto op2 = ResolveImm2Operand2(armbb, stack_size, true);
  MyAssert(!op2->is_imm_);
  ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::SUB, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2));

  // mov sp_vreg(r16),sp
  ADD_NEW_INST(Move(sp_vreg, new Operand2(new Reg(ArmReg::sp))));

  // OPT: mov vreg r0-r3 把所有参数全部存到虚拟寄存器中 并维护varmap  OPT: 填充arg reg
  for (int i = 0; i < arg_num; ++i) {
    auto vreg = NewVirtualReg();
    this->arg_reg.push_back(vreg->reg_id_);
    if (i < 4) {
      ADD_NEW_INST(Move(vreg, new Operand2(new Reg(ArmReg(i)))));
    } else {
      this->GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, vreg, sp_vreg, this->stack_size + 4 + (i - 4) * 4);
    }
  }
  this->ChangeOffset(func->name_);
}

void GenerateArmOpt::ResetFuncData(ArmFunction* armfunc) {
  this->virtual_reg_id = 16;
  // NOTE: need to guarantee sp_vreg is r16.
  this->sp_vreg = NewVirtualReg();

  this->var_map.clear();
  this->arg_num = armfunc->arg_num_;
  // OPT:
  armfunc->stack_size_ -= this->arg_num * 4;
  this->stack_size = armfunc->stack_size_;
  this->sp_arg_fixup.clear();
  this->sp_fixup.clear();
  // OPT:
  this->arg_reg.clear();
}

#undef ASSERT_ENABLE