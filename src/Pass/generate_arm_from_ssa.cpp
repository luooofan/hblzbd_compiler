
#include "../../include/Pass/generate_arm_from_ssa.h"

#include <algorithm>
#include <functional>

#include "../../include/arm.h"
#include "../../include/arm_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_GENARM_FROMSSA_PROCESS

#define NEW_INST(inst) static_cast<Instruction*>(new inst)
#define ADD_NEW_INST(inst) armbb->inst_list_.push_back(static_cast<Instruction*>(new inst));

#define MUL_TO_SHIFT
#define DIV_TO_SHIFT
#define MOD_TO_AND

using namespace arm;
Cond GenerateArmFromSSA::GetCondType(BranchInst::Cond cond, bool exchange) {
  if (exchange) {
    switch (cond) {
      case BranchInst::Cond::EQ:
        return Cond::EQ;
      case BranchInst::Cond::NE:
        return Cond::NE;
      case BranchInst::Cond::GT:
        return Cond::LT;
      case BranchInst::Cond::GE:
        return Cond::LE;
      case BranchInst::Cond::LT:
        return Cond::GT;
      case BranchInst::Cond::LE:
        return Cond::GE;
      default:
        return Cond::AL;
    }
  } else {
    switch (cond) {
      case BranchInst::Cond::EQ:
        return Cond::EQ;
      case BranchInst::Cond::NE:
        return Cond::NE;
      case BranchInst::Cond::GT:
        return Cond::GT;
      case BranchInst::Cond::GE:
        return Cond::GE;
      case BranchInst::Cond::LT:
        return Cond::LT;
      case BranchInst::Cond::LE:
        return Cond::LE;
      default:
        return Cond::AL;
    }
  }
};

Reg* GenerateArmFromSSA::NewVirtualReg() { return new Reg(virtual_reg_id++); };

Reg* GenerateArmFromSSA::ResolveImm2Reg(ArmBasicBlock* armbb, int imm, bool record) {
  auto vreg = NewVirtualReg();
  Instruction* inst = nullptr;
  if (Operand2::CheckImm8m(imm)) {
    // NOTE: 寄存器分配后立即数值可能会变
    inst = NEW_INST(Move(vreg, new Operand2(imm), armbb));
  } else {
    if (imm < 0 && Operand2::CheckImm8m(-imm - 1)) {  // mvn
      inst = NEW_INST(Move(vreg, new Operand2(-imm - 1), armbb, true));
    } else {  // use ldr pseudo-inst instead of mov.
      inst = NEW_INST(LdrPseudo(vreg, imm, armbb));
    }
  }
  MyAssert(nullptr != inst);
  armbb->inst_list_.push_back(inst);
  if (record) {
    this->sp_fixup.insert(inst);
  }
  return vreg;
}

Operand2* GenerateArmFromSSA::ResolveImm2Operand2(ArmBasicBlock* armbb, int imm, bool record) {
  if (Operand2::CheckImm8m(imm) && !record) {
    return new Operand2(imm);
  }
  return new Operand2(ResolveImm2Reg(armbb, imm, record));
};

void GenerateArmFromSSA::GenImmLdrStrInst(ArmBasicBlock* armbb, LdrStr::OpKind opkind, Reg* rd, Reg* rn, int imm,
                                          bool record) {
  Instruction* inst = nullptr;
  if (record) {  // NOTE: sp_arg_fixup待补充的最后的栈空间大小+被调用者保护的寄存器的总大小
    auto vreg = NewVirtualReg();
    inst = NEW_INST(LdrPseudo(Cond::AL, vreg, imm, armbb));
    armbb->inst_list_.push_back(inst);
    ADD_NEW_INST(LdrStr(opkind, rd, rn, new Operand2(vreg), armbb));
    this->sp_arg_fixup.insert(inst);
  } else {
    if (LdrStr::CheckImm12(imm)) {
      inst = NEW_INST(LdrStr(opkind, rd, rn, imm, armbb));
      armbb->inst_list_.push_back(inst);
    } else {
      auto vreg = NewVirtualReg();
      if (Operand2::CheckImm8m(imm)) {  // mov
        inst = NEW_INST(Move(vreg, new Operand2(imm), armbb));
      } else if (imm < 0 && Operand2::CheckImm8m(-imm - 1)) {  // mvn
        inst = NEW_INST(Move(vreg, new Operand2(-imm - 1), armbb, true));
      } else {  // ldr-pseudo
        inst = NEW_INST(LdrPseudo(vreg, imm, armbb));
      }
      armbb->inst_list_.push_back(inst);
      ADD_NEW_INST(LdrStr(opkind, rd, rn, new Operand2(vreg), armbb));
    }
  }
}

bool GenerateArmFromSSA::ConvertMul2Shift(ArmBasicBlock* armbb, Reg* rd, Value* val, int imm) {
  // 如果乘数是0 生成一条mov rd 0的指令
  if (0 == imm) {
    ADD_NEW_INST(Move(rd, new Operand2(0), armbb));
    return true;
  }
  auto eval_n = [](int imm) {
    int n = 0;
    while (imm > 1) {
      imm >>= 1;
      ++n;
    }
    return n;
  };
  // 如果乘数是2的幂次方 生成一条mov rd rm LSL n的指令 LSL允许0-31位
  if (0 == (imm & (imm - 1))) {
    auto op2 = new Operand2(ResolveValue2Reg(armbb, val), new Shift(Shift::OpCode::LSL, eval_n(imm)));
    ADD_NEW_INST(Move(rd, op2, armbb));
    return true;
  }
  // 如果乘数是2的幂次方+1 生成一条add rd, rn, rn LSL n
  if (0 == ((imm - 1) & (imm - 2))) {
    // std::cout << 1 << std::endl;
    auto vreg = ResolveValue2Reg(armbb, val);
    auto op2 = new Operand2(vreg, new Shift(Shift::OpCode::LSL, eval_n(imm - 1)));
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, rd, vreg, op2, armbb));
    return true;
  }
  // 如果乘数是2的幂次方-1 生成一条rsb rd, rn, rn LSL n
  if (0 == ((imm + 1) & (imm))) {
    // std::cout << 2 << std::endl;
    auto vreg = ResolveValue2Reg(armbb, val);
    auto op2 = new Operand2(vreg, new Shift(Shift::OpCode::LSL, eval_n(imm + 1)));
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::RSB, rd, vreg, op2, armbb));
    return true;
  }
  return false;
}

bool GenerateArmFromSSA::ConvertDiv2Shift(ArmBasicBlock* armbb, Reg* rd, Value* val, int imm) {
  // 如果除数是0 应该生成报错
  if (0 == imm) {
    MyAssert(0);
  }
  auto eval_n = [](int imm) {
    int n = 0;
    while (imm > 1) {
      imm >>= 1;
      ++n;
    }
    return n;
  };
  // 如果除数是2的幂次方 生成一条mov rd rm A(L)SR n的指令 允许1-32位
  // FIXME: 这样做并不适用于被除数是负数的情况
  if (0 == (imm & (imm - 1))) {
    std::cout << "Warning Div" << std::endl;
    auto op2 = new Operand2(ResolveValue2Reg(armbb, val), new Shift(Shift::OpCode::ASR, eval_n(imm)));
    ADD_NEW_INST(Move(rd, op2, armbb));
    return true;
  }
  // 考虑负数情况
  if (0 == (imm & (imm - 1))) {
    // 1. asr vreg, rn, #31;  i.e. mov vreg, rn, ASR #31;
    auto vreg = NewVirtualReg();
    auto rn = ResolveValue2Reg(armbb, val);
    ADD_NEW_INST(Move(vreg, new Operand2(rn, new Shift(Shift::OpCode::ASR, 31)), armbb));
    // 2. add vreg2, rn, vreg, lsr #(32-n);
    auto vreg2 = NewVirtualReg();
    auto n = eval_n(imm);
    auto op2 = new Operand2(vreg, new Shift(Shift::OpCode::LSR, 32 - n));
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, vreg2, rn, op2, armbb));
    // 3. asr rd, vreg2, #n;  i.e. mov rd, vreg2, ASR #n;
    ADD_NEW_INST(Move(rd, new Operand2(vreg2, new Shift(Shift::OpCode::ASR, n)), armbb));
  }
  return false;
}

bool GenerateArmFromSSA::ConvertMod2And(ArmBasicBlock* armbb, Reg* rd, Value* val, int imm) {
  // 如果是0 应该生成报错
  if (0 == imm) {
    MyAssert(0);
  }
  auto eval_n = [](int imm) {
    int n = 0;
    while (imm > 1) {
      imm >>= 1;
      ++n;
    }
    return n;
  };
  // 如果是2的幂次方 生成一条and rd rm n-1的指令
  // FIXME: 这样做并不适用于被除数是负数的情况
  if (0 == (imm & (imm - 1))) {
    std::cout << "Warning Mod" << std::endl;
    auto rm = ResolveValue2Reg(armbb, val);
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::AND, rd, rm, ResolveImm2Operand2(armbb, imm - 1), armbb));
    return true;
  }
  if (0 == (imm & (imm - 1))) {
    // 1. asr vreg, rn, #31;  i.e. mov vreg, rn, ASR #31;
    auto vreg = NewVirtualReg();
    auto rn = ResolveValue2Reg(armbb, val);
    ADD_NEW_INST(Move(vreg, new Operand2(rn, new Shift(Shift::OpCode::ASR, 31)), armbb));
    // 2. add vreg2, rn, vreg, lsr #(32-n);
    auto vreg2 = NewVirtualReg();
    auto n = eval_n(imm);
    auto op2 = new Operand2(vreg, new Shift(Shift::OpCode::LSR, 32 - n));
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, vreg2, rn, op2, armbb));
    // 3. bic vreg2, vreg2, #(2^n-1);  i.e. bfc vreg2, #0, #n;
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::BIC, vreg2, vreg2, ResolveImm2Operand2(armbb, imm - 1), armbb));
    // 4. sub rd, rn, vreg2
    ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::SUB, rd, rn, new Operand2(vreg2), armbb));
  }
  return false;
}

Reg* GenerateArmFromSSA::ResolveValue2Reg(ArmBasicBlock* armbb, Value* val) {
  if (auto src_val = dynamic_cast<ConstantInt*>(val)) {
    return ResolveImm2Reg(armbb, src_val->GetImm());
  } else if (auto src_val = dynamic_cast<GlobalVariable*>(val)) {
    // 如果是全局量 在函数prologue中会把其地址或基址记录在var map中 直接返回地址即可
    return var_map[val];
  }
  // else if (auto src_val = dynamic_cast<UndefVariable*>(val)) {
  // val->Print(std::cout);// MyAssert(0);
  // 测例中会出现UB 但是测例用给定的输入数据不会有UB e.g. int c; while(c...){...} return c;
  // 暂时和普通变量同处理
  //}
  else if (auto src_val = dynamic_cast<Argument*>(val)) {
    // 如果能在var map中找到 说明这是一次对参数的使用 返回即可 返回的寄存器中存的可能是参数变量值 也可能是数组参数的基址
    if (var_map.find(val) != var_map.end()) {
      return var_map[val];
    }
    // 找不到 此时执行流肯定在addprologue中 根据是第几个参数 产生mov或者ldr语句 把参数都放到寄存器里 并记录到var map中
    else {
      Reg* vreg = NewVirtualReg();
      auto arg_no = src_val->GetArgNo();
      if (arg_no < 4) {
        ADD_NEW_INST(Move(vreg, new Operand2(new Reg(arg_no)), armbb));
      } else {  // NOTE: 寄存器分配后修改的时候应该是+=最终的stack_size+push的大小
        GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, vreg, sp_vreg, /*4 lr +*/ (arg_no - 4) * 4, true);
      }
      var_map[val] = vreg;
      return vreg;
    }
  } else if (dynamic_cast<FunctionValue*>(val) || dynamic_cast<BasicBlockValue*>(val)) {
    MyAssert(0);
  } else {
    // 先在var map中找 找到直接返回 没找到(意味着定值)就记录映射并返回
    if (var_map.find(val) != var_map.end()) {
      return var_map[val];
    } else {
      Reg* vreg = NewVirtualReg();
      var_map[val] = vreg;
      return vreg;
    }
  }
}

Operand2* GenerateArmFromSSA::ResolveValue2Operand2(ArmBasicBlock* armbb, Value* val) {
  if (auto src_val = dynamic_cast<ConstantInt*>(val)) {
    return ResolveImm2Operand2(armbb, src_val->GetImm());
  } else {
    return new Operand2(ResolveValue2Reg(armbb, val));
  }
}

void GenerateArmFromSSA::AddPrologue(ArmFunction* func, FunctionValue* func_val) {
  // push {lr}
  auto armbb = func->bb_list_.front();
  auto push_inst = new PushPop(PushPop::OpKind::PUSH, armbb);
  push_inst->reg_list_.push_back(new Reg(ArmReg::lr));
  armbb->inst_list_.push_back(static_cast<Instruction*>(push_inst));

  // sub sp,sp,#0    放进sp_fixup中 等待寄存器分配完成后修改该值
  auto op2 = ResolveImm2Operand2(armbb, 0, true);
  MyAssert(!op2->is_imm_);
  ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::SUB, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2, armbb));

  // TODO:
  // 实际上可以把prologue单独当作一个基本块 把原来第一个bb的idom指向prologue 并维护其他关系
  // 取参数和全局变量到一个vreg中的操作可以放到在该函数内其所有使用的最小公共祖先基本块的末尾(dom tree)
  // 这样应该能在一定程度上优化寄存器分配
  for (auto arg : func_val->GetArgList()) {
    ResolveValue2Reg(armbb, arg);
  }

  // load global var address to vreg NOTE: 全局量都是地址 全局变量的偏移都是0 全局数组的偏移可变
  for (auto glob_var : func_val->GetFunction()->GetUsedGlobVarList()) {
    Reg* rglo = NewVirtualReg();
    var_map[glob_var] = rglo;
    ADD_NEW_INST(LdrPseudo(rglo, glob_var->GetName(), armbb));
  }
}

void GenerateArmFromSSA::AddEpilogue(ArmBasicBlock* armbb) {
  // 目前的策略是每个return前都要加epilogue 也可以在函数最后放一个epilogue基本块 然后return翻译成跳转
  // 要为每一个ret语句添加epilogue
  // add sp, sp, #stack_size 之后应该修改为栈大小 记录在sp_fixup里
  auto op2 = ResolveImm2Operand2(armbb, 0, true);
  auto add_inst = NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2, armbb));
  armbb->inst_list_.push_back(add_inst);
  MyAssert(!op2->is_imm_);

  // pop {pc} NOTE: same as bx lr
  auto pop_inst = new PushPop(PushPop::OpKind::POP, armbb);
  pop_inst->reg_list_.push_back(new Reg(ArmReg::pc));
  armbb->inst_list_.push_back(static_cast<Instruction*>(pop_inst));
};

void GenerateArmFromSSA::ResetFuncData() {
  this->virtual_reg_id = 16;
  this->var_map.clear();
  this->sp_vreg = new Reg(ArmReg::sp);
  this->stack_size = 0;
  this->sp_arg_fixup.clear();
  this->sp_fixup.clear();
}

void GenerateArmFromSSA::GenerateArmBasicBlocks(ArmFunction* armfunc, SSAFunction* func,
                                                std::unordered_map<SSABasicBlock*, ArmBasicBlock*>& bb_map) {
  // create armbb according to irbb
  for (auto bb : func->GetBBList()) {
    // ArmBasicBlock* armbb = new ArmBasicBlock(&(bb->GetLabel()));  // FIXME
    ArmBasicBlock* armbb = new ArmBasicBlock();
    bool set_label = false;
    for (auto use : bb->GetValue()->GetUses()) {
      if (nullptr == dynamic_cast<PhiInst*>(use->GetUser())) set_label = true;
    }
    if (set_label) armbb->label_ = bb->GetLabel();
    bb_map.insert({bb, armbb});
    armfunc->bb_list_.push_back(armbb);
    armbb->func_ = armfunc;
  }
  // maintain pred and succ
  for (auto bb : func->GetBBList()) {
    auto armbb = bb_map[bb];
    for (auto pred : bb->GetPredBB()) {
      armbb->pred_.push_back(bb_map[pred]);
    }
    for (auto succ : bb->GetSuccBB()) {
      armbb->succ_.push_back(bb_map[succ]);
    }
  }
}

ArmModule* GenerateArmFromSSA::GenCode(SSAModule* module) {
  // FIXME
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
  std::cout << "Gen ArmCode From SSA Start:" << std::endl;
#endif
  ArmModule* armmodule = new ArmModule(module->name_, module->global_scope_);  // FIXME: terrible design
  std::unordered_map<SSAFunction*, ArmFunction*> func_map;

  // for every ir func
  for (auto func : module->GetFuncList()) {
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
    std::cout << "Gen ArmCode From Every Function Start:" << std::endl;
#endif
    ArmFunction* armfunc = new ArmFunction(func->GetFuncName(), func->GetValue()->GetArgNum(), 0);
    func_map.insert({func, armfunc});
    armmodule->func_list_.push_back(armfunc);

    std::unordered_map<SSABasicBlock*, ArmBasicBlock*> bb_map;
    GenerateArmBasicBlocks(armfunc, func, bb_map);
    ResetFuncData();
    AddPrologue(armfunc, func->GetValue());

    // for every ssa basicblock
    for (auto bb : func->GetBBList()) {
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
      std::cout << "Gen ArmCode From Every BB Start:" << std::endl;
#endif
      auto armbb = bb_map[bb];

      auto gen_rn_op2 = [this, &armbb](Value* lhs, Value* rhs, Reg*& rn, Operand2*& op2) {
        bool is_lhs_imm = nullptr != dynamic_cast<ConstantInt*>(lhs);
        if (is_lhs_imm) {
          // exchange order: lhs->op2 rhs->rn res->rd
          rn = ResolveValue2Reg(armbb, rhs);
          op2 = ResolveValue2Operand2(armbb, lhs);
        } else {
          // lhs->rn rhs->op2 res->rd
          rn = ResolveValue2Reg(armbb, lhs);
          op2 = ResolveValue2Operand2(armbb, rhs);
        }
        return is_lhs_imm;
      };

      auto gen_bi_inst = [&armbb](BinaryInst::OpCode opcode, Reg* rd, Reg* rn, Operand2* op2) {
        ADD_NEW_INST(BinaryInst(opcode, rd, rn, op2, armbb));
      };

      // for every ssa inst
      for (auto inst_iter = bb->GetInstList().begin(); inst_iter != bb->GetInstList().end(); ++inst_iter) {
        // 对全局或局部数组某个元素的取值和赋值只会出现在load和store指令中 对全局变量的操作也会出现在load和store指令中
        // 地址(可能不是数组基址 而是数组某个位置的地址)只会出现在call指令的参数中
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
        std::cout << "\tnow process this inst:" << std::endl;
        (*inst_iter)->Print(std::cout);
#endif
        auto inst = *inst_iter;
        if (auto src_inst = dynamic_cast<BinaryOperator*>(inst)) {
          auto lhs = src_inst->GetOperand(0);
          auto rhs = src_inst->GetOperand(1);
          auto res = src_inst;
          Reg* rn = nullptr;
          Operand2* op2 = nullptr;
          switch (src_inst->op_) {
            case BinaryOperator::OpKind::ADD: {
              gen_rn_op2(lhs, rhs, rn, op2);
              Reg* rd = ResolveValue2Reg(armbb, res);
              gen_bi_inst(BinaryInst::OpCode::ADD, rd, rn, op2);
              break;
            }
            case BinaryOperator::OpKind::SUB: {
              bool is_opn1_imm = gen_rn_op2(lhs, rhs, rn, op2);
              BinaryInst::OpCode opcode = is_opn1_imm ? BinaryInst::OpCode::RSB : BinaryInst::OpCode::SUB;
              Reg* rd = ResolveValue2Reg(armbb, res);
              gen_bi_inst(opcode, rd, rn, op2);
              break;
            }
            case BinaryOperator::OpKind::MUL: {
              // 应该不会出现同时为立即数的情况
              Reg* rd = ResolveValue2Reg(armbb, res);
#ifdef MUL_TO_SHIFT
              if (auto const_int = dynamic_cast<ConstantInt*>(lhs))
                if (ConvertMul2Shift(armbb, rd, rhs, const_int->GetImm())) break;
              if (auto const_int = dynamic_cast<ConstantInt*>(rhs))
                if (ConvertMul2Shift(armbb, rd, lhs, const_int->GetImm())) break;
#endif
              rn = ResolveValue2Reg(armbb, lhs);
              // NOTE: MUL的两个操作数必须全为寄存器 不能是立即数
              op2 = new Operand2(ResolveValue2Reg(armbb, rhs));
              gen_bi_inst(BinaryInst::OpCode::MUL, rd, rn, op2);
              break;
            }
            case BinaryOperator::OpKind::DIV: {
              Reg* rd = ResolveValue2Reg(armbb, res);
#ifdef DIV_TO_SHIFT
              if (auto const_int = dynamic_cast<ConstantInt*>(lhs))
                if (ConvertDiv2Shift(armbb, rd, rhs, const_int->GetImm())) break;
              if (auto const_int = dynamic_cast<ConstantInt*>(rhs))
                if (ConvertDiv2Shift(armbb, rd, lhs, const_int->GetImm())) break;
#endif
              rn = ResolveValue2Reg(armbb, lhs);
              op2 = new Operand2(ResolveValue2Reg(armbb, rhs));
              gen_bi_inst(BinaryInst::OpCode::SDIV, rd, rn, op2);
              break;
            }
            case BinaryOperator::OpKind::MOD: {
              Reg* rd = ResolveValue2Reg(armbb, res);
#ifdef MOD_TO_AND
              if (auto const_int = dynamic_cast<ConstantInt*>(lhs))
                if (ConvertMod2And(armbb, rd, rhs, const_int->GetImm())) break;
              if (auto const_int = dynamic_cast<ConstantInt*>(rhs))
                if (ConvertMod2And(armbb, rd, lhs, const_int->GetImm())) break;
#endif
              // 取余 in c/c++ not 取模
              // sdiv rd1, rn, op2
              rn = ResolveValue2Reg(armbb, lhs);
              op2 = new Operand2(ResolveValue2Reg(armbb, rhs));
              Reg* rd1 = NewVirtualReg();
              gen_bi_inst(BinaryInst::OpCode::SDIV, rd1, rn, op2);
              // mul rd2, rd1, op2
              Reg* rd2 = NewVirtualReg();
              gen_bi_inst(BinaryInst::OpCode::MUL, rd2, rd1, op2);
              // sub rd, rd1, rd2
              gen_bi_inst(BinaryInst::OpCode::SUB, rd, rn, new Operand2(rd2));
              break;
            }
            default: {
              MyAssert(0);
              break;
            }
          }
        } else if (auto src_inst = dynamic_cast<UnaryOperator*>(inst)) {
          auto lhs = src_inst->GetOperand(0);
          auto res = src_inst;
          switch (src_inst->op_) {
            case UnaryOperator::NEG: {
              // impl by RSB res, opn1, #0
              auto rn = ResolveValue2Reg(armbb, lhs);
              auto op2 = ResolveImm2Operand2(armbb, 0);
              Reg* rd = ResolveValue2Reg(armbb, res);
              gen_bi_inst(BinaryInst::OpCode::RSB, rd, rn, op2);
              break;
            }
            case UnaryOperator::NOT: {
              // cmp rn 0
              // moveq rd 1
              // movne rd 0
              auto rn = ResolveValue2Reg(armbb, lhs);
              Reg* rd = ResolveValue2Reg(armbb, res);
              ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::CMP, rn, ResolveImm2Operand2(armbb, 0), armbb));
              ADD_NEW_INST(Move(false, Cond::EQ, rd, ResolveImm2Operand2(armbb, 1), armbb));
              ADD_NEW_INST(Move(false, Cond::NE, rd, ResolveImm2Operand2(armbb, 0), armbb));
              break;
            }
            default: {
              MyAssert(0);
              break;
            }
          }
        } else if (auto src_inst = dynamic_cast<BranchInst*>(inst)) {
          // 最后一个操作数是labeltype 一定是BasicBlockValue
          if (src_inst->cond_ == BranchInst::Cond::AL) {  // 无条件跳转
            ADD_NEW_INST(Branch(false, false, src_inst->GetOperand(0)->GetName(), armbb));
          } else {  // 有条件跳转
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            bool is_opn1_imm = gen_rn_op2(src_inst->GetOperand(0), src_inst->GetOperand(1), rn, op2);
            ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::CMP, rn, op2, armbb));
            ADD_NEW_INST(Branch(false, false, GetCondType(src_inst->cond_, is_opn1_imm),
                                src_inst->GetOperand(2)->GetName(), armbb));
          }
        } else if (auto src_inst = dynamic_cast<CallInst*>(inst)) {
          // NOTE: 这里对调用语句的处理并不规范 但方便目标代码的生成和寄存器溢出情况的处理

          // 第一个操作数一定是FunctionValue
          int param_num = src_inst->GetNumOperands() - 1;

          // mov r0-r3 op2 | str rd sp+x
          for (int i = 0; i < param_num; ++i) {
            // 这个val可能是i32 也可能是i32 pointer 不过都是值传递没区别
            auto val = src_inst->GetOperand(i + 1);
            if (i < 4) {
              // r0-r3. MOV rx, op2
              auto op2 = ResolveValue2Operand2(armbb, val);
              MyAssert(nullptr != op2);
              // std::cout << std::string(*op2) << std::endl;
              // armbb->EmitCode(std::cout);
              ADD_NEW_INST(Move(new Reg(i), op2, armbb));
            } else {
              // str rd, sp, #(order-4)*4 靠后的参数放在较高的地方 第5个(order=4)放在sp指向的内存
              auto rd = ResolveValue2Reg(armbb, val);
              // GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, new Reg(ArmReg::sp), (i - 4) * 4);
              // NOTE: 这将导致str语句中出现负立即数
              GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, sp_vreg, -(param_num - i) * 4);
            }
          }
          // armbb->EmitCode(std::cout);

          // sub sp, sp, #(param_num-4)*4 NOTE: 一定要放在bl前一句
          if (param_num > 4) {
            auto op2 = ResolveImm2Operand2(armbb, (param_num - 4) * 4);
            ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::SUB, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2, armbb));
          }
          // BL label
          ADD_NEW_INST(Branch(true, false, Cond::AL, src_inst->GetOperand(0)->GetName(), armbb));

          // add sp, sp, #(param_num-4)*4  NOTE: 一定要放在bl后一句
          if (param_num > 4) {
            auto op2 = ResolveImm2Operand2(armbb, (param_num - 4) * 4);
            ADD_NEW_INST(BinaryInst(BinaryInst::OpCode::ADD, new Reg(ArmReg::sp), new Reg(ArmReg::sp), op2, armbb));
          }

          // mov rd r0
          if (!src_inst->GetType()->IsVoid()) {
            // NOTE: 这里必须生成一条mov语句 会存在两个call ir接连出现的情况
            auto vreg = ResolveValue2Reg(armbb, src_inst);
            ADD_NEW_INST(Move(false, Cond::AL, vreg, new Operand2(new Reg(ArmReg::r0)), armbb));
          }

        } else if (auto src_inst = dynamic_cast<ReturnInst*>(inst)) {
          // NOTE: 无论有没有返回值都要mov到r0中 为了之后活跃分析时认为是对r0的一次定值
          Operand2* op2 = nullptr;
          if (src_inst->GetNumOperands() > 0) {
            op2 = ResolveValue2Operand2(armbb, src_inst->GetOperand(0));
          } else {
            op2 = ResolveImm2Operand2(armbb, 0);
          }
          ADD_NEW_INST(Move(new Reg(ArmReg::r0), op2, armbb));
          AddEpilogue(armbb);
          break;
        } else if (auto src_inst = dynamic_cast<AllocaInst*>(inst)) {
          // 把局部数组基址add到vreg中
          // NOTE: 应该也可以记录一个数组到栈中基址偏移的映射 每次操作把对基址的偏移计算为相对sp的偏移
          auto rbase = ResolveValue2Reg(armbb, src_inst->GetOperand(1));
          MyAssert(nullptr != rbase);
          // 此时只是在var map中记录了该数组基址对应的vreg 还需要生成一条add指令
          ADD_NEW_INST(
              BinaryInst(BinaryInst::OpCode::ADD, rbase, sp_vreg, ResolveImm2Operand2(armbb, stack_size), armbb));

          // maintain stack size
          auto constint_value = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
          MyAssert(nullptr != constint_value);
          stack_size += constint_value->GetImm();

        } else if (auto src_inst = dynamic_cast<LoadInst*>(inst)) {
          // NOTE: 凡是ldr和str命令都要注意立即数
          // 有offset 一定是对数组操作 全局变量和局部数组基址都已经放入var map中
          auto rd = ResolveValue2Reg(armbb, src_inst);
          auto rn = ResolveValue2Reg(armbb, src_inst->GetOperand(0));
          // 有偏移的时候要看偏移是否是常数 没偏移认为偏移为0
          if (2 == src_inst->GetNumOperands()) {
            auto offset_val = src_inst->GetOperand(1);
            if (auto constint_val = dynamic_cast<ConstantInt*>(offset_val)) {
              GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, rd, rn, constint_val->GetImm());
            } else {
              Operand2* offset = ResolveValue2Operand2(armbb, offset_val);
              ADD_NEW_INST(LdrStr(LdrStr::OpKind::LDR, rd, rn, offset, armbb));
            }
          } else {
            GenImmLdrStrInst(armbb, LdrStr::OpKind::LDR, rd, rn, 0);
          }

        } else if (auto src_inst = dynamic_cast<StoreInst*>(inst)) {
          // NOTE: 凡是ldr和str命令都要注意立即数
          // 有offset 一定是对数组操作 全局变量和局部数组基址都已经放入var map中
          auto rd = ResolveValue2Reg(armbb, src_inst->GetOperand(0));
          auto rn = ResolveValue2Reg(armbb, src_inst->GetOperand(1));
          // 有偏移的时候要看偏移是否是常数 没偏移认为偏移为0
          if (src_inst->GetNumOperands() == 3) {
            auto offset_val = src_inst->GetOperand(2);
            if (auto constint_val = dynamic_cast<ConstantInt*>(offset_val)) {
              GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, rn, constint_val->GetImm());
            } else {
              Operand2* offset = ResolveValue2Operand2(armbb, offset_val);
              ADD_NEW_INST(LdrStr(LdrStr::OpKind::STR, rd, rn, offset, armbb));
            }
          } else {
            GenImmLdrStrInst(armbb, LdrStr::OpKind::STR, rd, rn, 0);
          }

        } else if (auto src_inst = dynamic_cast<MovInst*>(inst)) {
          auto op2 = ResolveValue2Operand2(armbb, src_inst->GetOperand(0));
          auto rd = ResolveValue2Reg(armbb, src_inst);
          ADD_NEW_INST(Move(rd, op2, armbb));
        } else if (auto src_inst = dynamic_cast<PhiInst*>(inst)) {
          // TODO
        } else {
          MyAssert(0);
        }
      }
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
      std::cout << "Gen ArmCode For Every BB End." << std::endl;
      // armbb->EmitCode(std::cout);
#endif
    }  // end of bb loop

    // NOTE: 中间代码保证了函数执行流最后一定有一条return语句 所以不必在最后再加一个epilogue

    // handle phi nodes
    // NOTE: 暂时用一些冗余的临时变量避免lost copy problem和swap problem 切割关键边待定
    for (auto bb : func->GetBBList()) {
      auto armbb = bb_map[bb];
      // 处理当前bb的phi node和所有前驱bb
      for (auto inst : bb->GetInstList()) {
        if (auto src_inst = dynamic_cast<PhiInst*>(inst)) {
          // 每次都新建个vreg 在前驱的最后一条语句或者跳转语句前加一条原value对应的vreg到该vreg的mov
          // 在该基本块前面加一条vreg到原dest value的mov指令
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
          src_inst->Print(std::cout);
#endif
          auto vreg = NewVirtualReg();
          armbb->inst_list_.insert(armbb->inst_list_.begin(),
                                   new Move(ResolveValue2Reg(armbb, src_inst), new Operand2(vreg), armbb));

          for (int i = 0; i * 2 < src_inst->GetNumOperands(); ++i) {
            // std::cout << i << std::endl;
            // 第i个前驱对应的value是第2i个操作数 对应的bb是第i个前驱bb 或者第2i+1个操作数(拿到的是bbvalue)
            auto val = src_inst->GetOperand(2 * i);
            auto ssa_pred_bb = dynamic_cast<BasicBlockValue*>(src_inst->GetOperand(2 * i + 1))->GetBB();
            auto arm_pred_bb = bb_map[ssa_pred_bb];
            MyAssert(nullptr != ssa_pred_bb && nullptr != arm_pred_bb);
            // arm_pred_bb如果为空 直接加在最后面
            if (arm_pred_bb->inst_list_.empty()) {
              auto new_inst = new Move(vreg, ResolveValue2Operand2(arm_pred_bb, val), arm_pred_bb);
              arm_pred_bb->inst_list_.push_back(new_inst);
              continue;
            }
            auto last_inst = arm_pred_bb->inst_list_.back();
            auto last_branch_inst = dynamic_cast<Branch*>(last_inst);
            // std::cout << i << std::endl;
            // 最后一条如果是跳转语句
            if (nullptr != last_branch_inst) {
              auto pred_last_inst1 = arm_pred_bb->inst_list_.back();
              arm_pred_bb->inst_list_.pop_back();
              // 条件跳转语句 前一句一定是cmp 在cmp前加入new_inst
              if (last_branch_inst->cond_ != Cond::AL) {
                auto pred_last_inst2 = arm_pred_bb->inst_list_.back();
                arm_pred_bb->inst_list_.pop_back();
                auto new_inst = new Move(vreg, ResolveValue2Operand2(arm_pred_bb, val), arm_pred_bb);
                // arm_pred_bb->inst_list_.insert(arm_pred_bb->inst_list_.end() - 2, new_inst);
                arm_pred_bb->inst_list_.push_back(new_inst);
                arm_pred_bb->inst_list_.push_back(pred_last_inst2);

              }
              // 无条件跳转语句 在跳转语句前插入
              else {
                auto new_inst = new Move(vreg, ResolveValue2Operand2(arm_pred_bb, val), arm_pred_bb);
                // arm_pred_bb->inst_list_.insert(arm_pred_bb->inst_list_.end() - 1, new_inst);
                arm_pred_bb->inst_list_.push_back(new_inst);
              }
              arm_pred_bb->inst_list_.push_back(pred_last_inst1);
            }
            // 其他语句 直接加在最后面
            else {
              auto new_inst = new Move(vreg, ResolveValue2Operand2(arm_pred_bb, val), arm_pred_bb);
              arm_pred_bb->inst_list_.push_back(new_inst);
            }
            // std::cout << i << std::endl;
          }
        }
        // NOTE: 可能把一条phi指令优化为mov 所以并非phi指令都在最前面
        // else {
        // break;
        //}
      }
    }

    armfunc->sp_fixup_ = this->sp_fixup;
    armfunc->sp_arg_fixup_ = this->sp_arg_fixup;
    armfunc->virtual_reg_max = virtual_reg_id;
    armfunc->stack_size_ = stack_size;
#ifdef DEBUG_GENARM_FROMSSA_PROCESS
    std::cout << "Gen ArmCode From Every Function End." << std::endl;
#endif
  }  // end of func loop

  // maintain call_func_list
  for (auto func : module->GetFuncList()) {
    auto armfunc = func_map[func];
    for (auto callee : func->GetCallFuncList()) {
      auto iter = func_map.find(callee);
      if (iter == func_map.end()) {
        // buildin system library function
        armfunc->call_func_list_.push_back(new ArmFunction(callee->GetFuncName(), callee->GetValue()->GetArgNum(), 0));
      } else {
        armfunc->call_func_list_.push_back((*iter).second);
      }
    }
  }

  return armmodule;
}

void GenerateArmFromSSA::Run() {
  auto m = dynamic_cast<SSAModule*>(*(this->m_));
  MyAssert(nullptr != m);
  auto arm_m = this->GenCode(m);
  *(this->m_) = static_cast<Module*>(arm_m);
  delete m;
}

#undef ASSERT_ENABLE