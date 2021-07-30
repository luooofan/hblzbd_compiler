#include "../../include/Pass/generate_arm.h"

#include <algorithm>
#include <functional>

#define ASSERT_ENABLE
#include "../../include/myassert.h"

using namespace arm;
#define NEW_INST(inst) static_cast<Instruction*>(new inst)
#define ADD_NEW_INST(inst) armbb->inst_list_.push_back(static_cast<Instruction*>(new inst));
// NO_OPT GEN ARM

Cond GenerateArm::GetCondType(ir::IR::OpKind opkind, bool exchange) {
  if (exchange) {
    switch (opkind) {
      case ir::IR::OpKind::JEQ:
        return Cond::EQ;
      case ir::IR::OpKind::JNE:
        return Cond::NE;
      case ir::IR::OpKind::JLT:
        return Cond::GT;
      case ir::IR::OpKind::JLE:
        return Cond::GE;
      case ir::IR::OpKind::JGT:
        return Cond::LT;
      case ir::IR::OpKind::JGE:
        return Cond::LE;
      default:
        return Cond::AL;
    }
  } else {
    switch (opkind) {
      case ir::IR::OpKind::JEQ:
        return Cond::EQ;
      case ir::IR::OpKind::JNE:
        return Cond::NE;
      case ir::IR::OpKind::JLT:
        return Cond::LT;
      case ir::IR::OpKind::JLE:
        return Cond::LE;
      case ir::IR::OpKind::JGT:
        return Cond::GT;
      case ir::IR::OpKind::JGE:
        return Cond::GE;
      default:
        return Cond::AL;
    }
  }
  return Cond::AL;
};

Reg* GenerateArm::NewVirtualReg() { return new Reg(virtual_reg_id++); };

Operand2* GenerateArm::ResolveImm2Operand2(ArmBasicBlock* armbb, int imm, bool record) {
  if (Operand2::CheckImm8m(imm)) {
    // NOTE: 寄存器分配后立即数值可能会变
    if (record) {
      auto vreg = NewVirtualReg();
      Instruction* inst = nullptr;
      inst = NEW_INST(Move(vreg, new Operand2(imm)));
      armbb->inst_list_.push_back(inst);
      this->sp_fixup.push_back(inst);
      return new Operand2(vreg);
    } else {
      return new Operand2(imm);
    }
  } else {
    auto vreg = NewVirtualReg();
    Instruction* inst = nullptr;
    if (imm < 0 && Operand2::CheckImm8m(-imm - 1)) {  // mvn
      inst = NEW_INST(Move(vreg, new Operand2(-imm - 1), true));
    } else {  // use ldr pseudo-inst instead of mov.
      inst = NEW_INST(LdrPseudo(vreg, imm));
    }
    armbb->inst_list_.push_back(inst);
    if (record) {
      this->sp_fixup.push_back(inst);
    }
    return new Operand2(vreg);
  }
};

void GenerateArm::GenImmLdrStrInst(ArmBasicBlock* armbb, LdrStr::OpKind opkind, Reg* rd, Reg* rn, int imm) {
  Instruction* inst = nullptr;
  if (rn == this->sp_vreg && imm > this->stack_size) {  // NOTE: 寄存器分配后立即数值可能会变化
    auto vreg = NewVirtualReg();
    inst = NEW_INST(LdrPseudo(Cond::AL, vreg, imm));
    armbb->inst_list_.push_back(inst);
    ADD_NEW_INST(LdrStr(opkind, rd, rn, new Operand2(vreg)));
    this->sp_arg_fixup.push_back(inst);
  } else {
    if (LdrStr::CheckImm12(imm)) {
      inst = NEW_INST(LdrStr(opkind, rd, rn, imm));
      armbb->inst_list_.push_back(inst);
    } else {
      auto vreg = NewVirtualReg();
      if (Operand2::CheckImm8m(imm)) {  // mov
        inst = NEW_INST(Move(vreg, new Operand2(imm)));
      } else if (imm < 0 && Operand2::CheckImm8m(-imm - 1)) {  // mvn
        inst = NEW_INST(Move(vreg, new Operand2(-imm - 1), true));
      } else {  // ldr-pseudo
        inst = NEW_INST(LdrPseudo(vreg, imm));
      }
      armbb->inst_list_.push_back(inst);
      ADD_NEW_INST(LdrStr(opkind, rd, rn, new Operand2(vreg)));
    }
  }
}

void GenerateArm::AddEpilogue(ArmBasicBlock* armbb) {
  // 要为每一个ret语句添加epilogue
  // add sp, sp, #stack_size
  auto op2 = ResolveImm2Operand2(armbb, stack_size, true);
  auto add_inst = NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2));
  armbb->inst_list_.push_back(add_inst);
  MyAssert(!op2->is_imm_);
  // pop {pc} NOTE: same as bx lr
  auto pop_inst = new PushPop(PushPop::OpKind::POP);
  pop_inst->reg_list_.push_back(new Reg(ArmReg::pc));
  armbb->inst_list_.push_back(static_cast<Instruction*>(pop_inst));
};

Reg* GenerateArm::LoadGlobalOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) {
  MyAssert(0 == opn->scope_id_);
  // 如果是全局变量就要重新ldr
  Reg* rglo = NewVirtualReg();
  // 把全局量基址ldr到某寄存器中
  ADD_NEW_INST(LdrPseudo(rglo, opn->name_));
  return rglo;
};

Reg* GenerateArm::GetRBase(ArmBasicBlock* armbb, ir::Opn* opn) {
  Reg* rbase = nullptr;
  if (0 == opn->scope_id_) {  // 全局数组 地址量即其基址量
    rbase = LoadGlobalOpn2Reg(armbb, opn);
  } else {  // 不用在varmap找 每次都要重新加载 不会有中间数组
    rbase = NewVirtualReg();
    auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
    if (!symbol.IsArg()) {
      auto op2 = ResolveImm2Operand2(armbb, symbol.stack_offset_);
      ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, rbase, sp_vreg, op2));
    } else {  // 等于-1说明是int array参数 offset中存着的是基地址 所以要ldr
      this->GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, rbase, sp_vreg, symbol.stack_offset_);
    }
  }
  return rbase;
}

Reg* GenerateArm::ResolveOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) {
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
    if (symbol.IsTemp()) {  // 如果是中间变量 找到返回 没找到生成并绑定
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
    } else {  // 局部变量 一定生成一条ldr语句 ldr rd, [sp, #offset]
      auto vreg = NewVirtualReg();
      this->GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, vreg, sp_vreg, symbol.stack_offset_);
      return vreg;
    }
  }
};

Operand2* GenerateArm::ResolveOpn2Operand2(ArmBasicBlock* armbb, ir::Opn* opn) {
  if (opn->type_ == ir::Opn::Type::Imm) {  // 如果是立即数 返回立即数类型的Operand2
    return ResolveImm2Operand2(armbb, opn->imm_num_);
  } else {
    return new Operand2(ResolveOpn2Reg(armbb, opn));
  }
};

void GenerateArm::ResolveResOpn2RdReg(ArmBasicBlock* armbb, ir::Opn* opn, std::function<void(Reg*)> f) {
  // 只能是Var类型 Assign不调用此函数
  MyAssert(opn->type_ == ir::Opn::Type::Var);
  Reg* rd = nullptr;
  // Var offset为-1或者以temp-开头表示中间变量 是中间变量则只生成一条运算指令
  auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
  if (symbol.IsTemp()) {
    const auto& iter = var_map.find(opn->name_);
    if (iter != var_map.end()) {
      const auto& iter2 = (*iter).second.find(opn->scope_id_);
      if (iter2 != (*iter).second.end()) {
        rd = (*iter2).second;
      }
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
  } else if (!symbol.IsTemp()) {
    this->GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, sp_vreg, symbol.stack_offset_);
  }
}

void GenerateArm::ChangeOffset(std::string& func_name) {
  // 把符号表里的偏移都改了: 对于所有参数和局部int量改为stack_size-offset-4
  // 对于局部int array量改为stack_size-offset-array.width[0] 中间变量仍为-1
  // 首先拿到函数的作用域id
  const auto& iter = ir::gFuncTable.find(func_name);
  MyAssert(iter != ir::gFuncTable.end());
  int func_scope_id = iter->second.scope_id_;
  // 遍历所有作用域 修改偏移 改后的偏移为针对sp的偏移
  for (auto& scope : ir::gScopes) {
    if (scope.IsSubScope(func_scope_id)) {
      for (auto& item : scope.symbol_table_) {
        auto& symbol = item.second;
        if (symbol.IsTemp()) {        // 中间变量
          symbol.stack_offset_ = -1;  // useless
        } else if (symbol.IsArg()) {  // 参数
          if (symbol.IsHighArg()) {   // 第5个及以后的参数 函数栈大小加上一个lr
            symbol.stack_offset_ = this->stack_size + 4 + (symbol.offset_ / 4 - 4) * 4;
          } else {  // 前4个参数
            symbol.stack_offset_ = this->stack_size - symbol.offset_ - 4;
          }
        } else if (symbol.is_array_) {  // 非参数局部数组量
          symbol.stack_offset_ =
              this->stack_size - (symbol.offset_ - 4 * (std::max(4, this->arg_num) - 4)) - symbol.width_[0];
        } else {  // 非参数局部非数组量
          symbol.stack_offset_ = this->stack_size - (symbol.offset_ - 4 * (std::max(4, this->arg_num) - 4)) - 4;
        }
      }
    }
  }
  // ir::PrintScopes(std::cout);
}

void GenerateArm::AddPrologue(ArmFunction* func) {
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

  // str args 把r0-r3中的参数全部存到当前栈中 顺着存
  for (int i = 0; i < arg_num; ++i) {
    if (i < 4) {
      // str ri sp offset!=i*4 =(stack_size-offset-4) 或者 stack_size-i*4-4
      this->GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, new Reg(ArmReg(i)), sp_vreg, stack_size - i * 4 - 4);
    }
  }
  this->ChangeOffset(func->name_);
}

void GenerateArm::GenCallCode(ArmBasicBlock* armbb, ir::IR& ir, std::vector<ir::IR*>::iterator ir_iter) {
  MyAssert(ir.opn1_.type_ == ir::Opn::Type::Func);
  // 处理param语句
  // NOTE: 需要确保IR中函数调用表现为一系列PARAM+一条CALL
  // 不用push pop 调用前sub调用后add即可 因为都是值传递 即便callee内部对数据有所更改也无需保留更改
  // 而对于r4-r11会交给callee保护这些寄存器 也就是说调用完成后所有寄存器该定值定值 该使用使用
  int param_num = ir.opn2_.imm_num_;
  if (param_num > 4) {
    // sub sp, sp, #(param_num-4)*4
    auto op2 = ResolveImm2Operand2(armbb, (param_num - 4) * 4);
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::SUB, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2));
  }

  auto param_start = ir_iter - param_num;
  for (int i = 0; i < param_num; ++i) {  // i表示当前正在看第几条param语句 也即倒数第几个参数 start from 0
    auto& param_ir = **(param_start + i);
    int order = param_num - 1 - i;  // order表示正数第几个参数 start from 0
    if (order < 4) {
      // r0-r3. MOV rx, op2
      auto op2 = ResolveOpn2Operand2(armbb, &(param_ir.opn1_));
      auto rd = new Reg(static_cast<ArmReg>(order));
      ADD_NEW_INST(Move(rd, op2));
    } else {
      // str rd, sp, #(order-4)*4 靠后的参数放在较高的地方 第5个(order=4)放在sp指向的内存
      auto rd = ResolveOpn2Reg(armbb, &(param_ir.opn1_));
      this->GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, new Reg(ArmReg::sp), (order - 4) * 4);
    }
  }
  // BL label
  ADD_NEW_INST(Branch(true, false, Cond::AL, ir.opn1_.name_));
  arm::gAllLabel.insert(ir.opn1_.name_);
  if (ir.res_.type_ != ir::Opn::Type::Null) {
    // NOTE: 这里必须生成一条mov语句 会存在两个call ir接连出现的情况
    // 并且只能放在这个基本块中或新建一个基本块不能放到下一个基本块中
    auto vreg = NewVirtualReg();
    ADD_NEW_INST(Move(false, Cond::AL, vreg, new Operand2(new Reg(ArmReg::r0))));
    var_map[ir.res_.name_][ir.res_.scope_id_] = vreg;
  }

  if (param_num > 4) {
    // add sp, sp, #(param_num-4)*4
    auto op2 = ResolveImm2Operand2(armbb, (param_num - 4) * 4);
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2));
  }
}

void GenerateArm::ResetFuncData(ArmFunction* armfunc) {
  this->virtual_reg_id = 16;
  this->var_map.clear();
  this->sp_vreg = NewVirtualReg();
  this->arg_num = armfunc->arg_num_;
  if (this->arg_num > 4) armfunc->stack_size_ -= (this->arg_num - 4) * 4;
  this->stack_size = armfunc->stack_size_;
  this->sp_arg_fixup.clear();
  this->sp_fixup.clear();
}

ArmModule* GenerateArm::GenCode(IRModule* module) {
  // 由ir module直接构建arm module
  // 构建的过程中把ir中的label语句删掉 转化成了基本块的name
  // NOTE: 可能会出现一些空的bb 有些基本块是unnamed
  ArmModule* armmodule = new ArmModule(module->name_, module->global_scope_);
  std::unordered_map<IRFunction*, ArmFunction*> func_map;

  auto mod_func = new ArmFunction("__aeabi_idivmod", 2, 0);
  arm::gAllLabel.insert("__aeabi_idivmod");

  // for every ir func
  for (auto func : module->func_list_) {
    arm::gAllLabel.insert(func->name_);

    ArmFunction* armfunc = new ArmFunction(func->name_, func->arg_num_, func->stack_size_);
    func_map.insert({func, armfunc});
    armmodule->func_list_.push_back(armfunc);

    ResetFuncData(armfunc);
    // OPT: ChangeOffset(armfunc->name_);

    // create armbb according to irbb
    std::unordered_map<IRBasicBlock*, ArmBasicBlock*> bb_map;
    for (auto bb : func->bb_list_) {
      ArmBasicBlock* armbb = new ArmBasicBlock();
      bb_map.insert({bb, armbb});
      armfunc->bb_list_.push_back(armbb);
      armbb->func_ = armfunc;
    }
    // maintain pred and succ
    for (auto bb : func->bb_list_) {
      auto armbb = bb_map[bb];
      for (auto pred : bb->pred_) {
        armbb->pred_.push_back(bb_map[pred]);
      }
      for (auto succ : bb->succ_) {
        armbb->succ_.push_back(bb_map[succ]);
      }
    }

    AddPrologue(armfunc);

    // for every ir basicblock
    for (auto bb : func->bb_list_) {
      auto armbb = bb_map[bb];

      // 处理所有label语句
      auto first_ir = bb->ir_list_.front();
      if (first_ir->op_ == ir::IR::OpKind::LABEL) {
        // TODO:
        arm::gAllLabel.insert(first_ir->opn1_.name_);
        if (first_ir->opn1_.type_ == ir::Opn::Type::Label) {  // 如果是label就把label赋给这个bb的label
          armbb->label_ = new std::string(first_ir->opn1_.name_);
        }  // 如果是func label不用管 直接删了就行
        bb->ir_list_.erase(bb->ir_list_.begin());
      }

      auto dbg_print_var_map = [this]() {
        std::clog << "DBG_PRINT_VAR_MAP BEGIN." << std::endl;
        for (auto& outer : this->var_map) {
          std::clog << "varname: " << outer.first << std::endl;
          for (auto& inner : outer.second) {
            std::clog << "\tscope_id: " << inner.first << " " << std::string(*(inner.second)) << std::endl;
          }
        }
        std::clog << "DBG_PRINT_VAR_MAP END." << std::endl;
      };

      auto gen_rn_op2 = [this, &armbb](ir::Opn* opn1, ir::Opn* opn2, Reg*& rn, Operand2*& op2) {
        bool is_opn1_imm = opn1->type_ == ir::Opn::Type::Imm;
        if (is_opn1_imm) {
          // exchange order: opn1->op2 opn2->rn res->rd
          rn = ResolveOpn2Reg(armbb, opn2);
          op2 = ResolveOpn2Operand2(armbb, opn1);
        } else {
          // opn1->rn opn2->op2 res->rd
          rn = ResolveOpn2Reg(armbb, opn1);
          op2 = ResolveOpn2Operand2(armbb, opn2);
        }
        return is_opn1_imm;
      };

      auto gen_bi_inst = [&armbb](BinaryInst::OpCode opcode, Reg* rd, Reg* rn, Operand2* op2) {
        ADD_NEW_INST(BinaryInst(opcode, rd, rn, op2));
      };

      // valid ir: gIRList[start,end_)
      // for every ir
      for (auto ir_iter = bb->ir_list_.begin(); ir_iter != bb->ir_list_.end(); ++ir_iter) {
        // 对于每条有原始变量(非中间变量)的四元式 为了保证正确性 必须生成对使用变量的ldr指令
        // 因为无法确认程序执行流 无法确认当前使用的变量的上一个定值点 也就无法确认到底在哪个寄存器中
        // 所以需要ldr到一个新的虚拟寄存器中 为了保证逻辑与存储的一致性 必须生成对定值变量的str指令
        // 函数参数和局部变量被视为中间变量 局部数组量每次使用都需ldr 每次定值都要str
        auto& ir = **ir_iter;
        switch (ir.op_) {
          // NOTE: 只有assign类型ir的res可能是局部变量 全局变量 也可能是数组类型 其他情况的res都是中间变量
          case ir::IR::OpKind::ADD: {
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            gen_rn_op2(&(ir.opn1_), &(ir.opn2_), rn, op2);
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::ADD, rd, rn, op2);
            break;
          }
          case ir::IR::OpKind::SUB: {
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            bool is_opn1_imm = gen_rn_op2(&(ir.opn1_), &(ir.opn2_), rn, op2);
            BinaryInst::OpCode opcode = is_opn1_imm ? BinaryInst::OpCode::RSB : BinaryInst::OpCode::SUB;
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(opcode, rd, rn, op2);
            break;
          }
          case ir::IR::OpKind::MUL: {
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            // NOTE: MUL的两个操作数必须全为寄存器 不能是立即数
            auto op2 = new Operand2(ResolveOpn2Reg(armbb, &(ir.opn2_)));
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::MUL, rd, rn, op2);
            break;
          }
          case ir::IR::OpKind::DIV: {
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            auto op2 = new Operand2(ResolveOpn2Reg(armbb, &(ir.opn2_)));
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::SDIV, rd, rn, op2);
            break;
          }
          case ir::IR::OpKind::MOD: {
            auto rm = ResolveOpn2Operand2(armbb, &(ir.opn1_));
            auto rn = ResolveOpn2Operand2(armbb, &(ir.opn2_));
            // mov r1, rn
            ADD_NEW_INST(Move(new Reg(ArmReg::r1), rn));
            // mov r0, rm
            ADD_NEW_INST(Move(new Reg(ArmReg::r0), rm));
            // bl __aeabi_idivmod
            ADD_NEW_INST(Branch(true, false, "__aeabi_idivmod"));
            // 维护call_list
            auto& call_funcs = armfunc->call_func_list_;
            if (std::find(call_funcs.begin(), call_funcs.end(), mod_func) == call_funcs.end()) {
              call_funcs.push_back(mod_func);
            }
            // mov rd, r1
            auto rd = ResolveOpn2Reg(armbb, &(ir.res_));
            ADD_NEW_INST(Move(rd, new Operand2(new Reg(ArmReg::r1))));
            break;
          }
          case ir::IR::OpKind::NOT: {
            // cmp rn 0
            // moveq rd 1
            // movne rd 0
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::CMP, rn, ResolveImm2Operand2(armbb, 0)));
            ADD_NEW_INST(Move(false, Cond::EQ, rd, ResolveImm2Operand2(armbb, 1)));
            ADD_NEW_INST(Move(false, Cond::NE, rd, ResolveImm2Operand2(armbb, 0)));
            break;
          }
          case ir::IR::OpKind::NEG: {
            // impl by RSB res, opn1, #0
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            auto op2 = ResolveImm2Operand2(armbb, 0);
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::RSB, rd, rn, op2);
            break;
          }
          case ir::IR::OpKind::LABEL: {  // 所有label语句都应该已经被跳过
            MyAssert(0);
            break;
          }
          case ir::IR::OpKind::PARAM: {  // 所有param语句放到call的时候处理
            break;
          }
          case ir::IR::OpKind::CALL: {  // 在GenCallCode中完成处理
            this->GenCallCode(armbb, ir, ir_iter);
            break;
          }
          case ir::IR::OpKind::RET: {  // mov + epilogue
            // NOTE:
            //  在ir中 ret语句的下一句一定是新的bb的首指令
            //  翻译时 ret int; -> mov + add sp + pop {pc};  ret void; -> add sp + pop {pc}
            //  mov语句视作对r0的一次定值 紧接着的pop语句应该视作对r0的一次使用
            //  但两条语句一定紧挨着在一个bb末尾所以不必如此 不会影响bb的def use livein liveout
            Operand2* op2 = nullptr;
            if (ir.opn1_.type_ != ir::Opn::Type::Null) {
              op2 = ResolveOpn2Operand2(armbb, &(ir.opn1_));

            } else {
              op2 = ResolveImm2Operand2(armbb, 0);
            }
            ADD_NEW_INST(Move(new Reg(ArmReg::r0), op2));
            AddEpilogue(armbb);
            break;
          }
          case ir::IR::OpKind::GOTO: {  // B label
            MyAssert(ir.opn1_.type_ == ir::Opn::Type::Label);
            arm::gAllLabel.insert(ir.opn1_.name_);
            ADD_NEW_INST(Branch(false, false, ir.opn1_.name_));
            break;
          }
          case ir::IR::OpKind::ASSIGN: {
            // NOTE: 很多四元式类型的op1和op2都可以是Array类型 但是只有assign语句的rd可能是Array类型
            MyAssert(ir.opn2_.type_ == ir::Opn::Type::Null);
            Operand2* op2 = ResolveOpn2Operand2(armbb, &(ir.opn1_));
            // 使res如果是全局变量的话 一定有基址寄存器
            if (ir.res_.type_ == ir::Opn::Type::Array) {
              Reg* rd = NewVirtualReg();
              // mov,rd(new vreg),op2
              ADD_NEW_INST(Move(rd, op2));
              Reg* rbase = GetRBase(armbb, &(ir.res_));
              if (ir.res_.offset_->type_ == ir::Opn::Type::Imm) {
                this->GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, rbase, ir.res_.offset_->imm_num_);
              } else {
                ADD_NEW_INST(LdrStr(LdrStr::OpKind::STR, rd, rbase, ResolveOpn2Operand2(armbb, ir.res_.offset_)));
              }
            } else {
              // 把一个op2 mov到某变量中
              auto gen_mov_inst = [&armbb, &op2](Reg* rd) { ADD_NEW_INST(Move(false, Cond::AL, rd, op2)); };
              this->ResolveResOpn2RdReg(armbb, &(ir.res_), gen_mov_inst);
            }
            break;
          }
          case ir::IR::OpKind::ASSIGN_OFFSET: {
            // (=[], array, -, temp-res) res一定是中间变量
            MyAssert(ir.opn2_.type_ == ir::Opn::Type::Null);
            Reg* vreg = NewVirtualReg();
            var_map[ir.res_.name_][ir.res_.scope_id_] = vreg;
            auto opn = &(ir.opn1_);
            Reg* rbase = GetRBase(armbb, opn);
            if (opn->offset_->type_ == ir::Opn::Type::Imm) {
              this->GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, vreg, rbase, opn->offset_->imm_num_);
            } else {
              ADD_NEW_INST(LdrStr(LdrStr::OpKind::LDR, vreg, rbase, ResolveOpn2Operand2(armbb, opn->offset_)));
            }

            break;
          }
          case ir::IR::OpKind::JEQ:
          case ir::IR::OpKind::JNE:
          case ir::IR::OpKind::JLT:
          case ir::IR::OpKind::JLE:
          case ir::IR::OpKind::JGT:
          case ir::IR::OpKind::JGE: {
            MyAssert(ir.res_.type_ == ir::Opn::Type::Label);
            arm::gAllLabel.insert(ir.res_.name_);
            // CMP rn op2; BEQ label;
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;

            bool is_opn1_imm = gen_rn_op2(&(ir.opn1_), &(ir.opn2_), rn, op2);

            ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::CMP, rn, op2));
            ADD_NEW_INST(Branch(false, false, GetCondType(ir.op_, is_opn1_imm), ir.res_.name_));
            break;
          }
          case ir::IR::OpKind::VOID: {
            MyAssert(0);
            break;
          }
          default: {
            MyAssert(0);
            break;
          }
        }
      }
    }  // end of ir loop

    // NOTE: 中间代码保证了函数执行流最后一定有一条return语句 所以不必在最后再加一个epilogue

    armfunc->sp_fixup_ = this->sp_fixup;
    armfunc->sp_arg_fixup_ = this->sp_arg_fixup;
    armfunc->virtual_reg_max = virtual_reg_id;
  }  // end of func loop

  // maintain call_func_list
  for (auto func : module->func_list_) {
    auto armfunc = func_map[func];
    for (auto callee : func->call_func_list_) {
      auto iter = func_map.find(callee);
      if (iter == func_map.end()) {
        // buildin system library function
        armfunc->call_func_list_.push_back(new ArmFunction(callee->name_, callee->arg_num_, callee->stack_size_));
      } else {
        armfunc->call_func_list_.push_back((*iter).second);
      }
    }
  }

  return armmodule;
}

void GenerateArm::Run() {
  auto m = dynamic_cast<IRModule*>(*(this->m_));
  MyAssert(nullptr != m);
  auto arm_m = this->GenCode(m);
  *(this->m_) = static_cast<Module*>(arm_m);
  delete m;
}

#undef ASSERT_ENABLE