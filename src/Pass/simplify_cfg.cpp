#include "../../include/Pass/simplify_cfg.h"

#include <algorithm>

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_SIMPLIFY_CFG_PROCESS

void SimplifyCFG::Run() {
  if (auto m = dynamic_cast<IRModule*>(*(this->m_))) {
    //
    for (auto func : m->func_list_) {
      Run4IRFunc(func);
    }
  } else if (auto m = dynamic_cast<SSAModule*>(*(this->m_))) {
    //
    for (auto func : m->GetFuncList()) {
      Run4SSAFunc(func);
    }
  } else {
    MyAssert(0);
  }
}

// 1. 删除没有前驱结点的基本块
// Specifically, this:
//   * removes basic blocks with no predecessors
//   * merges a basic block into its predecessor if there is only one and the
//     predecessor only has one successor.
//   * Eliminates PHI nodes for basic blocks with a single predecessor
//   * Eliminates a basic block that only contains an unconditional branch
void SimplifyCFG::Run4IRFunc(IRFunction* func) {
#ifdef DEBUG_SIMPLIFY_CFG_PROCESS
  std::cout << "========Before Simplyfy:========" << std::endl;
  func->EmitCode(std::cout);
#endif
  bool done = false;
  while (!done) {
    done = true;
    for (auto it = func->bb_list_.begin() + 1; it != func->bb_list_.end();) {
      auto bb = *it;
      // 1
      if (0 == bb->pred_.size()) {
        done = false;
        for (auto succ : bb->succ_) {
          succ->pred_.erase(std::find(succ->pred_.begin(), succ->pred_.end(), bb));
        }
        it = func->bb_list_.erase(it);
        continue;
      }
      // 2 unimplyment
      /*
      if (1 == bb->pred_.size() && 1 == bb->pred_.front()->succ_.size()) {
        done = false;
        auto pred = bb->pred_.front();
        // 前驱基本块的最后一条ir一定是跳转语句或者是一条能顺着执行下去的语句
        // 如果是跳转 必然是goto跳向当前基本块 需要删掉 同时不可能是条件跳转
        if (!pred->ir_list_.empty()) {
          auto pred_back_ir = pred->ir_list_.back();
          // 一定不会是cond branch, ret
          if (pred_back_ir->op_ == ir::IR::OpKind::GOTO) {
            pred->ir_list_.pop_back();
          }
        }
        // 顺序遍历bb的每一条ir如果是label语句直接删掉 否则添加到pred最后
        for (auto ir : bb->ir_list_) {
          if (ir->op_ == ir::IR::OpKind::LABEL) {
            // TODO delete or not NOTE: CAN'T DELETE
            // std::cout << "will delete 1" << std::endl;
            // delete ir;
          } else {
            pred->ir_list_.push_back(ir);
          }
        }
        // 维护前驱和后继关系
        bb->pred_.clear();
        pred->succ_.clear();
        for (auto succ : bb->succ_) {
          pred->succ_.push_back(succ);
          auto iter = std::find(succ->pred_.begin(), succ->pred_.end(), bb);
          MyAssert(iter != succ->pred_.end());
          *iter = pred;
        }
        bb->succ_.clear();
        bb->ir_list_.clear();
        // std::cout << "will delete 2" << std::endl;
        delete bb;
        it = func->bb_list_.erase(it);
        continue;
      }
      */
      // 3
      // 4 一个只有无条件跳转的基本块 其前驱基本块的最后一条语句要么是跳到它的跳转语句 如果不是该如何?
      ++it;
    }
  }
}

void SimplifyCFG::Run4SSAFunc(SSAFunction* func) {}

#undef ASSERT_ENABLE