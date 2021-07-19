#include "../../include/Pass/generate_arm.h"

#include <cassert>
#include <functional>

using namespace arm;

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

Operand2* GenerateArm::ResolveImm2Operand2(ArmBasicBlock* armbb, int imm) {
  int encoding = imm;
  for (int ror = 0; ror < 32; ror += 2) {
    if (!(encoding & ~0xFFu)) {
      return new Operand2(imm);
    }
    encoding = (encoding << 2u) | (encoding >> 30u);
  }
  // use move or ldr
  auto vreg = NewVirtualReg();
  armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(vreg, std::to_string(imm))));
  return new Operand2(vreg);
};

void GenerateArm::AddEpilogue(ArmBasicBlock* armbb) {
  // 要为每一个ret语句添加epilogue
  // add sp, sp, #stack_size
  armbb->inst_list_.push_back(
      static_cast<Instruction*>(new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, new Reg(ArmReg::sp),
                                               new Reg(ArmReg::sp), ResolveImm2Operand2(armbb, stack_size))));
  // pop {pc} NOTE: same as bx lr
  auto pop_inst = new PushPop(PushPop::OpKind::POP, Cond::AL);
  pop_inst->reg_list_.push_back(new Reg(ArmReg::pc));
  armbb->inst_list_.push_back(static_cast<Instruction*>(pop_inst));
};

void GenerateArm::LoadGlobalOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) {
  // 如果是全局变量并且没在glo_addr_map里找到就ldr
  if (opn->scope_id_ == 0) {
    Reg* rglo = nullptr;
    const auto& iter = glo_addr_map.find(opn->name_);
    if (iter != glo_addr_map.end()) {
      rglo = (*iter).second;
    }
    if (nullptr == rglo) {
      rglo = NewVirtualReg();
      glo_addr_map[opn->name_] = rglo;
      // 把全局量基址ldr到某寄存器中
      armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(rglo, opn->name_)));
    }
  }
};

Reg* GenerateArm::ResolveOpn2Reg(ArmBasicBlock* armbb, ir::Opn* opn) {
  // 如果是立即数 先move到一个vreg中 返回这个vreg
  // 如果是变量 需要判断是否为中间变量
  //  是中间变量则先查varmap 如果没有的话生成一个新的vreg 记录 返回
  //  如果不是 要先ldr到一个新的vreg中 不用记录 返回
  // 如果是数组 先找基址 要先ldr到一个新的vreg中 不记录 返回
  if (opn->type_ == ir::Opn::Type::Imm) {
    Reg* rd = NewVirtualReg();
    // mov rd(vreg) op2
    auto op2 = ResolveImm2Operand2(armbb, opn->imm_num_);
    armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
    return rd;
  } else if (opn->type_ == ir::Opn::Type::Array) {
    // Array 返回一个存着地址的寄存器 先找基址 如果存在rx中 (add, rv, rx, offset)
    // 如果没找到 则(add, rbase, vsp, offset) 记录 (add, rv, rbase, offset) 返回rv
    Reg* rbase = nullptr;
    Reg* vreg = NewVirtualReg();
    if (0 == opn->scope_id_) {
      // 全局数组 地址量即其基址量
      LoadGlobalOpn2Reg(armbb, opn);
      rbase = glo_addr_map[opn->name_];
    } else {
      const auto& iter = var_map.find(opn->name_);
      if (iter != var_map.end()) {
        const auto& iter2 = (*iter).second.find(opn->scope_id_);
        if (iter2 != (*iter).second.end()) {
          rbase = ((*iter2).second);
        }
      }
      if (nullptr == rbase) {
        rbase = NewVirtualReg();
        var_map[opn->name_][opn->scope_id_] = rbase;
        auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
        armbb->inst_list_.push_back(static_cast<Instruction*>(
            new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, rbase, sp_vreg,
                           ResolveImm2Operand2(armbb, stack_size - symbol.offset_ - symbol.width_[0]))));
      }
    }
    armbb->inst_list_.push_back(static_cast<Instruction*>(new BinaryInst(
        BinaryInst::OpCode::ADD, false, Cond::AL, vreg, rbase, ResolveOpn2Operand2(armbb, opn->offset_))));
    // offset可能并非立即数 不会导致死循环
    // armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(
    // LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL, vreg, rbase, ResolveOpn2Operand2(armbb, opn->offset_))));
    return vreg;
  } else {
    // 如果是全局变量 一定有一条ldr
    if (opn->scope_id_ == 0) {
      LoadGlobalOpn2Reg(armbb, opn);
      Reg* vreg = NewVirtualReg();
      // var_map[opn->name_][opn->scope_id_] = vreg;
      armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(
          LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL, vreg, glo_addr_map[opn->name_], new Operand2(0))));
      return vreg;
    }
    auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
    if (symbol.offset_ == -1) {
      // 如果是中间变量 找到返回 没找到生成并绑定
      const auto& iter = var_map.find(opn->name_);
      if (iter != var_map.end()) {
        const auto& iter2 = (*iter).second.find(opn->scope_id_);
        if (iter2 != (*iter).second.end()) {
          return (*iter2).second;
        }
      }
      Reg* vreg = NewVirtualReg();
      var_map[opn->name_][opn->scope_id_] = vreg;
      return vreg;
    } else {
      // 局部变量 一定生成一条ldr语句 ldr rd, [sp, #offset]
      auto vreg = NewVirtualReg();
      auto offset = ResolveImm2Operand2(armbb, stack_size - symbol.offset_ - 4);
      armbb->inst_list_.push_back({static_cast<Instruction*>(
          new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL, vreg, sp_vreg, offset))});
      return vreg;
    }
  }
};

Operand2* GenerateArm::ResolveOpn2Operand2(ArmBasicBlock* armbb, ir::Opn* opn) {
  // 如果是立即数 返回立即数类型的Operand2
  if (opn->type_ == ir::Opn::Type::Imm) {
    return ResolveImm2Operand2(armbb, opn->imm_num_);
  } else {
    return new Operand2(ResolveOpn2Reg(armbb, opn));
  }
};

template <typename CallableObjTy>
void GenerateArm::ResolveResOpn2RdReg(ArmBasicBlock* armbb, ir::Opn* opn, CallableObjTy f) {
  // 只能是Var类型 Assign不调用此函数
  assert(opn->type_ == ir::Opn::Type::Var);
  LoadGlobalOpn2Reg(armbb, opn);
  // Var offset为-1或者以temp-开头表示中间变量
  auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
  // 是中间变量则只生成一条运算指令
  Reg* rd = nullptr;
  if (-1 == symbol.offset_) {
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
    armbb->inst_list_.push_back(static_cast<Instruction*>(
        new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd, glo_addr_map[opn->name_], new Operand2(0))));
  } else if (-1 != symbol.offset_) {
    armbb->inst_list_.push_back(
        static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd, sp_vreg,
                                             ResolveImm2Operand2(armbb, stack_size - symbol.offset_ - 4))));
  }
}

void GenerateArm::AddPrologue(ArmBasicBlock* first_bb) {
  // 对于变量来说 栈偏移应为 stack_size-offset-4
  // 对于数组来说 栈偏移应为 stack_size-offset-array.width[0] 也是-4

  // 添加prologue 先push 再sub 再str
  // push {lr}
  auto push_inst = new PushPop(PushPop::OpKind::PUSH, Cond::AL);
  push_inst->reg_list_.push_back(new Reg(ArmReg::lr));
  first_bb->inst_list_.push_back(static_cast<Instruction*>(push_inst));
  // sub sp, sp, #stack_size
  first_bb->inst_list_.push_back(
      static_cast<Instruction*>(new BinaryInst(BinaryInst::OpCode::SUB, false, Cond::AL, new Reg(ArmReg::sp),
                                               new Reg(ArmReg::sp), ResolveImm2Operand2(first_bb, stack_size))));
  // add sp_vreg, sp, #0 是否有必要？
  first_bb->inst_list_.push_back(static_cast<Instruction*>(
      new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, sp_vreg, new Reg(ArmReg::sp), new Operand2(0))));
  // str args 把r0-r3中的参数全部存到当前栈中 顺着存
  for (int i = 0; i < arg_num; ++i) {
    if (i < 4) {
      // str ri sp offset!=i*4 =(stack_size-offset-4) 或者 stack_size-i*4-4
      first_bb->inst_list_.push_back(
          static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, new Reg(ArmReg(i)),
                                               sp_vreg, ResolveImm2Operand2(first_bb, stack_size - i * 4 - 4))));
    } else {
      // 可以存到当前栈中 也可以改一下符号表中的偏移
      // ldr ri sp stack_size+4+(i-4)*4
      // str ri sp offset = i*4
      int offset = stack_size + 4 + (i - 4) * 4;
      auto vreg = NewVirtualReg();
      first_bb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(
          LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL, vreg, sp_vreg, ResolveImm2Operand2(first_bb, offset))));
      first_bb->inst_list_.push_back(
          static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, vreg, sp_vreg,
                                               ResolveImm2Operand2(first_bb, stack_size - i * 4 - 4))));
    }
  }
}

void GenerateArm::GenCallCode(ArmBasicBlock* armbb, ir::IR& ir, int loc) {
  assert(ir.opn1_.type_ == ir::Opn::Type::Func);
  // 处理param语句
  // ir[start-1] is call
  // NOTE: 需要确保IR中函数调用表现为一系列PARAM+一条CALL
  // 不用push pop 调用前sub调用后add即可 因为都是值传递 即便callee内部对数据有所更改也无需保留更改
  // 而对于r4-r11会交给callee保护这些寄存器 也就是说调用完成后所有寄存器该定值定值 该使用使用
  int param_num = ir.opn2_.imm_num_;
  if (param_num > 4) {
    // sub sp, sp, #(param_num-4)*4
    armbb->inst_list_.push_back(static_cast<Instruction*>(
        new BinaryInst(BinaryInst::OpCode::SUB, false, Cond::AL, new Reg(ArmReg::sp), new Reg(ArmReg::sp),
                       ResolveImm2Operand2(armbb, (param_num - 4) * 4))));
  }

  int param_start = loc - 1 - param_num;
  for (int i = 0; i < param_num; ++i) {  // i表示当前正在看第几条param语句 也即倒数第几个参数 start from 0
    auto& param_ir = ir::gIRList[param_start + i];
    int order = param_num - 1 - i;  // order表示正数第几个参数 start from 0
    if (order < 4) {
      // r0-r3. MOV rx, op2
      auto op2 = ResolveOpn2Operand2(armbb, &(param_ir.opn1_));
      auto rd = new Reg(static_cast<ArmReg>(order));
      armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
    } else {
      // str rd, sp, #(order-4)*4 靠后的参数放在较高的地方 第5个(order=4)放在sp指向的内存
      auto rd = ResolveOpn2Reg(armbb, &(param_ir.opn1_));
      armbb->inst_list_.push_back(
          static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
                                               new Reg(ArmReg::sp), ResolveImm2Operand2(armbb, (order - 4) * 4))));
    }
  }
  // for (int i = param_num - 1; i >= 0; --i) {  // i表示当前的参数是正着数的第几个参数 start from 0
  //   auto& param_ir = ir::gIRList[param_start + param_num - i - 1];
  //   if (i < 4) {
  //     // r0-r3. MOV rx, op2
  //     auto op2 = ResolveOpn2Operand2(armbb, &(param_ir.opn1_));
  //     auto rd = new Reg(static_cast<ArmReg>(i));
  //     armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
  //   } else {
  //     // str rd, sp, #(i-4)*4 靠后的参数放在较高的地方 第5个(i=4)放在sp指向的内存
  //     auto rd = ResolveOpn2Reg(armbb, &(param_ir.opn1_));
  //     armbb->inst_list_.push_back(
  //         static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
  //                                              new Reg(ArmReg::sp), ResolveImm2Operand2(armbb, (i - 4) * 4))));
  //   }
  // }
  // BL label
  armbb->inst_list_.push_back(static_cast<Instruction*>(new Branch(true, false, Cond::AL, ir.opn1_.name_)));
  if (ir.res_.type_ != ir::Opn::Type::Null) {
    // NOTE: 这里不必生成一条mov语句
    var_map[ir.res_.name_][ir.res_.scope_id_] = new Reg(ArmReg::r0);
    // dbg_print_var_map();
  }

  if (param_num > 4) {
    // add sp, sp, #(param_num-4)*4
    armbb->inst_list_.push_back(static_cast<Instruction*>(
        new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, new Reg(ArmReg::sp), new Reg(ArmReg::sp),
                       ResolveImm2Operand2(armbb, (param_num - 4) * 4))));
  }
}

void GenerateArm::ResetFuncData(ArmFunction* armfunc) {
  this->virtual_reg_id = 16;
  this->var_map.clear();
  this->glo_addr_map.clear();
  this->sp_vreg = NewVirtualReg();
  this->stack_size = armfunc->stack_size_;
  this->arg_num = armfunc->arg_num_;
}

ArmModule* GenerateArm::GenCode(IRModule* module) {
  // 由ir module直接构建arm module
  // 构建的过程中把ir中的label语句删掉 转化成了基本块的name
  // NOTE: 可能会出现一些空的bb 有些基本块是unnamed
  ArmModule* armmodule = new ArmModule(module->name_, module->global_scope_);
  std::unordered_map<IRFunction*, ArmFunction*> func_map;

  auto div_func = new ArmFunction("__aeabi_idiv", 2, 0);
  auto mod_func = new ArmFunction("__aeabi_idivmod", 2, 0);

  // for every ir func
  for (auto func : module->func_list_) {
    ArmFunction* armfunc = new ArmFunction(func->name_, func->arg_num_, func->stack_size_);
    func_map.insert({func, armfunc});
    armmodule->func_list_.push_back(armfunc);

    // create armbb according to irbb
    std::unordered_map<IRBasicBlock*, ArmBasicBlock*> bb_map;
    for (auto bb : func->bb_list_) {
      ArmBasicBlock* armbb = new ArmBasicBlock();
      bb_map.insert({bb, armbb});
      armfunc->bb_list_.push_back(armbb);
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

    ResetFuncData(armfunc);
    AddPrologue(bb_map[func->bb_list_.front()]);

    // for every ir basicblock
    for (auto bb : func->bb_list_) {
      auto armbb = bb_map[bb];

      // 如果第一条ir是label label 就转成armbb的label 从而过滤所有label语句 这里假设bb的start一定有效(start<end)
      int start = bb->start_;
      if (ir::gIRList[start].op_ == ir::IR::OpKind::LABEL) {
        auto& opn1 = ir::gIRList[start].opn1_;
        if (opn1.type_ == ir::Opn::Type::Label) {
          armbb->label_ = new std::string(opn1.name_);
        }
        ++start;
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
        armbb->inst_list_.push_back(static_cast<Instruction*>(new BinaryInst(opcode, false, Cond::AL, rd, rn, op2)));
      };

      // valid ir: gIRList[start,end_)
      // for every ir
      while (start < bb->end_) {
        // 对于每条有原始变量(非中间变量)的四元式
        // 为了保证正确性 必须生成对使用变量的ldr指令
        // 因为无法确认程序执行流 无法确认当前使用的变量的上一个定值点
        // 也就无法确认到底在哪个寄存器中 所以需要ldr到一个新的虚拟寄存器中
        // 为了保证逻辑与存储的一致性 必须生成对定值变量的str指令
        // 寄存器分配期间也无法删除这些ldr和str指令
        // 这些虚拟寄存器对应的活跃区间也非常短
        auto& ir = ir::gIRList[start++];
        switch (ir.op_) {
          case ir::IR::OpKind::ADD: {
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            gen_rn_op2(&(ir.opn1_), &(ir.opn2_), rn, op2);
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::ADD, rd, rn, op2);
            // auto f = std::bind(gen_bi_inst, BinaryInst::OpCode::ADD, std::placeholders::_1, rn, op2);
            // this->ResolveResOpn2RdReg(armbb, &(ir.res_), f);
            break;
          }
          case ir::IR::OpKind::SUB: {
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            bool is_opn1_imm = gen_rn_op2(&(ir.opn1_), &(ir.opn2_), rn, op2);
            BinaryInst::OpCode opcode = is_opn1_imm ? BinaryInst::OpCode::RSB : BinaryInst::OpCode::SUB;
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(opcode, rd, rn, op2);
            // auto f = std::bind(gen_bi_inst, opcode, std::placeholders::_1, rn, op2);
            // this->ResolveResOpn2RdReg(armbb, &(ir.res_), f);
            break;
          }
          case ir::IR::OpKind::MUL: {
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            // NOTE: MUL的两个操作数必须全为寄存器 不能是立即数
            auto op2 = new Operand2(ResolveOpn2Reg(armbb, &(ir.opn2_)));
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::MUL, rd, rn, op2);
            // auto f = std::bind(gen_bi_inst, BinaryInst::OpCode::MUL, std::placeholders::_1, rn, op2);
            // this->ResolveResOpn2RdReg(armbb, &(ir.res_), f);
            break;
          }
          case ir::IR::OpKind::DIV: {
            auto rm = ResolveOpn2Operand2(armbb, &(ir.opn1_));
            auto rn = ResolveOpn2Operand2(armbb, &(ir.opn2_));
            // mov r1, rn
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, new Reg(ArmReg::r1), rn)));
            // mov r0, rm
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, new Reg(ArmReg::r0), rm)));
            // call __aeabi_idiv
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Branch(true, false, Cond::AL, "__aeabi_idiv")));
            //
            auto rd = ResolveOpn2Reg(armbb, &(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new Move(false, Cond::AL, rd, new Operand2(new Reg(ArmReg::r0)))));
            break;
          }
          case ir::IR::OpKind::MOD: {
            auto rm = ResolveOpn2Operand2(armbb, &(ir.opn1_));
            auto rn = ResolveOpn2Operand2(armbb, &(ir.opn2_));
            // mov r1, rn
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, new Reg(ArmReg::r1), rn)));
            // mov r0, rm
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, new Reg(ArmReg::r0), rm)));
            // call __aeabi_idivmod
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new Branch(true, false, Cond::AL, "__aeabi_idivmod")));
            // TODO: 维护call_list
            // if (armfunc->call_func_list_.find(mod_func) ==
            //     armfunc->call_func_list_.end()) {
            //   armfunc->call_func_list_.push_back(mod_func);
            // }
            // mov rd, r1
            auto rd = ResolveOpn2Reg(armbb, &(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new Move(false, Cond::AL, rd, new Operand2(new Reg(ArmReg::r1)))));
            break;
          }
          case ir::IR::OpKind::NOT: {
            // cmp rn 0
            // moveq rd 1
            // movne rd 0
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            auto gen_not_armcode = [&armbb, &rn](Reg* rd) {
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new BinaryInst(BinaryInst::OpCode::CMP, Cond::AL, rn, new Operand2(0))));
              armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::EQ, rd, new Operand2(1))));
              armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::NE, rd, new Operand2(0))));
            };
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_not_armcode(rd);
            // this->ResolveResOpn2RdReg(armbb, &(ir.res_), gen_not_armcode);
            break;
          }
          case ir::IR::OpKind::NEG: {
            // impl by RSB res, opn1, #0
            auto rn = ResolveOpn2Reg(armbb, &(ir.opn1_));
            auto op2 = new Operand2(0);
            Reg* rd = ResolveOpn2Reg(armbb, &(ir.res_));
            gen_bi_inst(BinaryInst::OpCode::RSB, rd, rn, op2);
            // auto f = std::bind(gen_bi_inst, BinaryInst::OpCode::RSB, std::placeholders::_1, rn, op2);
            // this->ResolveResOpn2RdReg(armbb, &(ir.res_), f);
            break;
          }
          case ir::IR::OpKind::LABEL: {  // 所有label语句都应该已经被跳过
            assert(0);
            break;
          }
          case ir::IR::OpKind::PARAM: {  // 所有param语句放到call的时候处理
            break;
          }
          case ir::IR::OpKind::CALL: {
            this->GenCallCode(armbb, ir, start);
            break;
          }
          case ir::IR::OpKind::RET: {
            // NOTE:
            //  在ir中 ret语句的下一句一定是新的bb的首指令
            //  翻译时 ret int; -> mov + add sp + pop {pc};  ret void; -> add sp + pop {pc}
            //  mov语句视作对r0的一次定值 紧接着的pop语句应该视作对r0的一次使用
            //  但两条语句一定紧挨着在一个bb末尾所以不必如此 不会影响bb的def use livein liveout
            //  带虚拟寄存器的arm code中只有对r0的def没有use
            if (ir.opn1_.type_ != ir::Opn::Type::Null) {
              Operand2* op2 = ResolveOpn2Operand2(armbb, &(ir.opn1_));
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new Move(false, Cond::AL, new Reg(ArmReg::r0), op2)));
            }
            AddEpilogue(armbb);
            break;
          }
          case ir::IR::OpKind::GOTO: {  // B label
            assert(ir.opn1_.type_ == ir::Opn::Type::Label);
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Branch(false, false, Cond::AL, ir.opn1_.name_)));
            break;
          }
          case ir::IR::OpKind::ASSIGN: {
            // maybe imm or var or array
            //
            // NOTE:
            // 很多四元式类型的op1和op2都可以是Array类型
            // 但是只有assign语句的rd可能是Array类型
            assert(ir.opn2_.type_ == ir::Opn::Type::Null);
            Operand2* op2 = ResolveOpn2Operand2(armbb, &(ir.opn1_));
            // 使res如果是全局变量的话 一定有基址寄存器
            if (ir.res_.type_ == ir::Opn::Type::Array) {
              Reg* rd = NewVirtualReg();
              // mov,rd(new vreg),op2
              armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
              // 先找有没有rbase 没有就用一条ldr指令加载到rbase里并记录
              // 有了rbase之后 str rd, rbase, offset
              Reg* rbase = nullptr;
              if (0 == ir.res_.scope_id_) {
                LoadGlobalOpn2Reg(armbb, &(ir.res_));
                rbase = glo_addr_map[ir.res_.name_];
              } else {
                const auto& iter = var_map.find(ir.res_.name_);
                if (iter != var_map.end()) {
                  const auto& iter2 = (*iter).second.find(ir.res_.scope_id_);
                  if (iter2 != (*iter).second.end()) {
                    rbase = ((*iter2).second);
                  }
                }
                if (nullptr == rbase) {
                  rbase = NewVirtualReg();
                  var_map[ir.res_.name_][ir.res_.scope_id_] = rbase;
                  auto& symbol = ir::gScopes[ir.res_.scope_id_].symbol_table_[ir.res_.name_];
                  armbb->inst_list_.push_back(static_cast<Instruction*>(
                      new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, rbase, sp_vreg,
                                     ResolveImm2Operand2(armbb, stack_size - symbol.offset_ - symbol.width_[0]))));
                }
              }
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd, rbase,
                                                       ResolveImm2Operand2(armbb, ir.res_.offset_->imm_num_))));
            } else {
              // 把一个op2 mov到某变量中
              // 如果是中间变量 直接mov到这个变量所在的寄存器中即可
              // 如果是局部变量 mov到这个变量所在的寄存器 然后str回内存
              // 如果是全局变量 mov到这个变量所在的寄存器 然后str回内存
              auto gen_mov_inst = [&armbb, &op2](Reg* rd) {
                armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
              };
              this->ResolveResOpn2RdReg(armbb, &(ir.res_), gen_mov_inst);
            }
            break;
          }
          case ir::IR::OpKind::ASSIGN_OFFSET: {
            // (=[], array, -, temp-res) res一定是中间变量
            assert(ir.opn2_.type_ == ir::Opn::Type::Null);
            Reg* rbase = nullptr;
            Reg* vreg = NewVirtualReg();
            var_map[ir.res_.name_][ir.res_.scope_id_] = vreg;
            auto opn = &(ir.opn1_);
            if (0 == opn->scope_id_) {
              // 全局数组 地址量即其基址量
              LoadGlobalOpn2Reg(armbb, opn);
              rbase = glo_addr_map[opn->name_];
            } else {
              const auto& iter = var_map.find(opn->name_);
              if (iter != var_map.end()) {
                const auto& iter2 = (*iter).second.find(opn->scope_id_);
                if (iter2 != (*iter).second.end()) {
                  rbase = ((*iter2).second);
                }
              }
              if (nullptr == rbase) {
                rbase = NewVirtualReg();
                var_map[opn->name_][opn->scope_id_] = rbase;
                auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
                armbb->inst_list_.push_back(static_cast<Instruction*>(
                    new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, rbase, sp_vreg,
                                   ResolveImm2Operand2(armbb, stack_size - symbol.offset_ - symbol.width_[0]))));
              }
            }
            // 找到rbase后 来一条ldr语句
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL, vreg, rbase,
                                                     ResolveOpn2Operand2(armbb, opn->offset_))));
            break;
          }
          case ir::IR::OpKind::JEQ:
          case ir::IR::OpKind::JNE:
          case ir::IR::OpKind::JLT:
          case ir::IR::OpKind::JLE:
          case ir::IR::OpKind::JGT:
          case ir::IR::OpKind::JGE: {
            assert(ir.res_.type_ == ir::Opn::Type::Label);
            // CMP rn op2; BEQ label;
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;

            bool is_opn1_imm = gen_rn_op2(&(ir.opn1_), &(ir.opn2_), rn, op2);

            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(BinaryInst::OpCode::CMP, Cond::AL, rn, op2)));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new Branch(false, false, GetCondType(ir.op_, is_opn1_imm), ir.res_.name_)));
            break;
          }
          case ir::IR::OpKind::VOID: {
            assert(0);
            break;
          }
          default: {
            assert(0);
            break;
          }
        }
      }
    }  // end of ir loop

    // NOTE: 中间代码保证了函数执行流最后一定有一条return语句 所以不必在最后再加一个epilogue

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
  assert(nullptr != m);
  auto arm_m = this->GenCode(m);
  *(this->m_) = static_cast<Module*>(arm_m);
  delete m;
}
