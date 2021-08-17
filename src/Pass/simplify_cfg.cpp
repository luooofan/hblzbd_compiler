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

// 删除没有前驱结点的基本块
void SimplifyCFG::Run4IRFunc(IRFunction* func) {
#ifdef DEBUG_SIMPLIFY_CFG_PROCESS
  std::cout << "========Before Simplyfy:========" << std::endl;
  func->EmitCode(std::cout);
#endif
  std::unordered_set<IRBasicBlock*> worklist;
  std::unordered_set<IRBasicBlock*> wait;
  for (auto bb : func->bb_list_) {
    if (bb->pred_.size() == 0) worklist.insert(bb);
  }
  if (func->bb_list_.size() > 0) worklist.erase(func->bb_list_[0]);
  while (!worklist.empty()) {
    auto bb = *worklist.begin();
    worklist.erase(bb);
    wait.insert(bb);
    for (auto succ : bb->succ_) {
      succ->pred_.erase(std::find(succ->pred_.begin(), succ->pred_.end(), bb));
      if (succ->pred_.size() == 0) worklist.insert(succ);
    }
  }
  for (auto it = func->bb_list_.begin(); it != func->bb_list_.end();) {
    if (wait.count(*it))
      it = func->bb_list_.erase(it);
    else
      ++it;
  }
}

void SimplifyCFG::Run4SSAFunc(SSAFunction* func) {}

#undef ASSERT_ENABLE