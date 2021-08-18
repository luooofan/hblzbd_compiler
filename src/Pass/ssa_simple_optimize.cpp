#include "../../include/Pass/ssa_simple_optimize.h"

#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_SIMPLE_OPTIMIZE_PROCESS
// #define DEBUG_SIMPLE_OPTIMIZE_EFFECT
static int kCount = 0;

void SimpleOptimize::Run() {
  kCount = 0;
  auto m = dynamic_cast<SSAModule*>(*(this->m_));
  MyAssert(nullptr != m);
  std::unordered_set<SSAInstruction*> worklist;
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
  std::cout << "SimpleOptimize :" << std::endl;
#endif
  for (auto func : m->GetFuncList()) {
    for (auto bb : func->GetBasicBlocks()) {
      for (auto inst : bb->GetInstList()) {
        worklist.insert(inst);
      }
    }
  }

  auto replace = [&worklist](SSAInstruction* from, SSAInstruction* to) {
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
    std::cout << "Convert :", from->Print(std::cout);
    std::cout << "      ==>", to->Print(std::cout);
#endif
    from->ReplaceAllUseWith(to);
    worklist.insert(to);
    from->Remove();
    delete from;
  };

  while (!worklist.empty()) {
    auto inst = *worklist.begin();
    MyAssert(nullptr != inst);
    worklist.erase(inst);
    // 如果phi语句的参数全是一个常数 或者phi语句只有一个参数
    // NOTE HERE!!! 应该不用考虑phi指令间的并行赋值 会打破phi指令一定是bb中最前面几条的性质
    if (auto src_inst = dynamic_cast<PhiInst*>(inst)) {
      // 考虑只有一个参数
      if (2 == src_inst->GetNumOperands()) {
        ++kCount;
        auto mov_inst = new MovInst(src_inst->GetType(), src_inst->GetName(), src_inst->GetOperand(0), src_inst);
        replace(src_inst, mov_inst);
        continue;
      }
      // 考虑参数全是常数
      auto constint = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
      if (nullptr == constint) continue;
      int i = 2;
      for (; i < src_inst->GetNumOperands(); i += 2) {
        auto operand = src_inst->GetOperand(i);
        auto constint2 = dynamic_cast<ConstantInt*>(operand);
        if (nullptr == constint2) break;
        if (constint2->GetImm() != constint->GetImm()) break;
      }
      if (i == src_inst->GetNumOperands()) {
        auto mov_inst = new MovInst(src_inst->GetType(), src_inst->GetName(), constint, src_inst);
        replace(src_inst, mov_inst);
        ++kCount;
        continue;
      }
    }
    // 如果是一条mov语句 该mov可删 用其操作数代替它自己
    else if (auto src_inst = dynamic_cast<MovInst*>(inst)) {
      ++kCount;
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
      std::cout << "Remove :", src_inst->Print(std::cout);
#endif
      // 把所有使用该value的加入工作表 实际上只加入mov和phi语句即可
      for (auto use : src_inst->GetUses()) {
        worklist.insert(dynamic_cast<SSAInstruction*>(use->GetUser()));
      }
      src_inst->ReplaceAllUseWith(src_inst->GetOperand(0));
      src_inst->Remove();  // 从基本块中移除
      delete src_inst;
    }
    // 如果是二元运算语句 并且两个操作数都是常数 则转化为一条mov语句
    else if (auto src_inst = dynamic_cast<BinaryOperator*>(inst)) {
      // TODO: 如果有一个数是常数并且为0 并且操作符是mul div mod那么直接转为一条mov语句即可
      auto const_lhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
      auto const_rhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(1));
      if (nullptr == const_lhs && nullptr == const_rhs) continue;
      MovInst* mov_inst = nullptr;
      if (nullptr != const_lhs && 0 == const_lhs->GetImm() &&
          (src_inst->op_ == BinaryOperator::DIV || src_inst->op_ == BinaryOperator::MOD ||
           src_inst->op_ == BinaryOperator::MUL)) {
        new MovInst(src_inst->GetType(), src_inst->GetName(), new ConstantInt(0), src_inst);
      } else if (nullptr != const_rhs && 0 == const_rhs->GetImm() && src_inst->op_ == BinaryOperator::MUL) {
        new MovInst(src_inst->GetType(), src_inst->GetName(), new ConstantInt(0), src_inst);
      } else if (nullptr != const_lhs && nullptr != const_rhs) {
        int res = src_inst->ComputeConstInt(const_lhs->GetImm(), const_rhs->GetImm());
        new MovInst(src_inst->GetType(), src_inst->GetName(), new ConstantInt(res), src_inst);
      }
      if (nullptr != mov_inst) {
        ++kCount;
        replace(src_inst, mov_inst);
      }
    }
    // 如果是一元运算语句 并且操作数是常数 则转化为一条mov语句
    else if (auto src_inst = dynamic_cast<UnaryOperator*>(inst)) {
      auto const_lhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
      if (nullptr == const_lhs) continue;
      ++kCount;
      int res = src_inst->ComputeConstInt(const_lhs->GetImm());
      auto mov_inst = new MovInst(src_inst->GetType(), src_inst->GetName(), new ConstantInt(res), src_inst);
      replace(src_inst, mov_inst);
    }
    // 如果是条件跳转语句 并且两个操作数都是立即数 可转换为无条件跳转或者删除
    // NOTE: 还必须修改基本块的前驱后继关系
    else if (auto src_inst = dynamic_cast<BranchInst*>(inst)) {
      if (3 == src_inst->GetNumOperands()) {
        auto const_lhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
        if (nullptr == const_lhs) continue;
        auto const_rhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(1));
        if (nullptr == const_rhs) continue;
        auto bb_val = dynamic_cast<BasicBlockValue*>(src_inst->GetOperand(2));
        MyAssert(nullptr != bb_val);
        auto bb = src_inst->GetParent(), target_bb = bb_val->GetBB();
        // continue;
        if (src_inst->ComputeConstInt(const_lhs->GetImm(), const_rhs->GetImm())) {
          ++kCount;
          // 条件判断永远为真 转换为无条件跳转
          auto new_inst = new BranchInst(bb_val, src_inst);
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
          std::cout << "Convert :", src_inst->Print(std::cout);
          std::cout << "      ==>", new_inst->Print(std::cout);
#endif
          auto next_bb = bb->GetNextBB();
          MyAssert(nullptr != next_bb);
          bb->RemoveSuccBB(next_bb);
          next_bb->RemovePredBB(bb);
          // 修复next bb中的phi指令
          for (auto inst : next_bb->GetInstList()) {
            auto phi_inst = dynamic_cast<PhiInst*>(inst);
            if (nullptr == phi_inst) break;
            for (int i = 1; i < phi_inst->GetNumOperands(); i += 2) {
              auto bb_val = dynamic_cast<BasicBlockValue*>(phi_inst->GetOperand(i));
              MyAssert(nullptr != bb_val);
              if (bb_val->GetBB() == bb) {
                // NOTE: order and break
                phi_inst->RemoveOperand(i);
                phi_inst->RemoveOperand(i - 1);
                break;
              }
            }
          }
          src_inst->Remove();
          delete src_inst;
        } else {
          ++kCount;
          // 条件判断永远为假 直接删除
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
          std::cout << "Remove :", src_inst->Print(std::cout);
#endif
          target_bb->RemovePredBB(bb);
          bb->RemoveSuccBB(target_bb);
          // 修复target bb中的phi指令
          for (auto inst : target_bb->GetInstList()) {
            auto phi_inst = dynamic_cast<PhiInst*>(inst);
            if (nullptr == phi_inst) break;
            for (int i = 1; i < phi_inst->GetNumOperands(); i += 2) {
              auto bb_val = dynamic_cast<BasicBlockValue*>(phi_inst->GetOperand(i));
              MyAssert(nullptr != bb_val);
              if (bb_val->GetBB() == bb) {
                // NOTE: order and break
                phi_inst->RemoveOperand(i);
                phi_inst->RemoveOperand(i - 1);
                break;
              }
            }
          }
          src_inst->Remove();
          delete src_inst;
        }
      }
    }
  }
#ifdef DEBUG_SIMPLE_OPTIMIZE_EFFECT
  std::cout << "SSA_SIMPLE_OPTIMIZE Pass works on " << kCount << " insts" << std::endl;
#endif
}