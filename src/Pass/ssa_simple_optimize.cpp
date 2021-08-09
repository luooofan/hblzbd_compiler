#include "../../include/Pass/ssa_simple_optimize.h"

#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_SIMPLE_OPTIMIZE_PROCESS

void SimpleOptimize::Run() {
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
  while (!worklist.empty()) {
    auto inst = *worklist.begin();
    MyAssert(nullptr != inst);
    worklist.erase(inst);
    // 如果phi语句的参数全是一个常数 或者phi语句只有一个参数
    if (auto src_inst = dynamic_cast<PhiInst*>(inst)) {
      // 到目前为之 没有能删除phi语句参数的操作 所以只考虑参数全是常数即可
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
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
        std::cout << "Remove :";
        src_inst->Print(std::cout);
#endif
        auto mov_inst = new MovInst(src_inst->GetType(), src_inst->GetName(), constint, src_inst);
        src_inst->ReplaceAllUseWith(mov_inst);
        worklist.insert(mov_inst);
        src_inst->Remove();
        delete src_inst;
      }
    }
    // 如果是一条mov语句 该mov可删 用其操作数代替它自己
    else if (auto src_inst = dynamic_cast<MovInst*>(inst)) {
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
      std::cout << "Remove :";
      src_inst->Print(std::cout);
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
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
      std::cout << "Remove :";
      src_inst->Print(std::cout);
#endif
      auto const_lhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
      if (nullptr == const_lhs) continue;
      auto const_rhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(1));
      if (nullptr == const_rhs) continue;
      int res = src_inst->ComputeConstInt(const_lhs->GetImm(), const_rhs->GetImm());
      auto mov_inst = new MovInst(src_inst->GetType(), src_inst->GetName(), new ConstantInt(res), src_inst);
      src_inst->ReplaceAllUseWith(mov_inst);
      worklist.insert(mov_inst);
      src_inst->Remove();
      delete src_inst;
    }
    // 如果是一元运算语句 并且操作数是常数 则转化为一条mov语句
    else if (auto src_inst = dynamic_cast<UnaryOperator*>(inst)) {
#ifdef DEBUG_SIMPLE_OPTIMIZE_PROCESS
      std::cout << "Remove :";
      src_inst->Print(std::cout);
#endif
      auto const_lhs = dynamic_cast<ConstantInt*>(src_inst->GetOperand(0));
      if (nullptr == const_lhs) continue;
      int res = src_inst->ComputeConstInt(const_lhs->GetImm());
      auto mov_inst = new MovInst(src_inst->GetType(), src_inst->GetName(), new ConstantInt(res), src_inst);
      src_inst->ReplaceAllUseWith(mov_inst);
      worklist.insert(mov_inst);
      src_inst->Remove();
      delete src_inst;
    }
  }
}