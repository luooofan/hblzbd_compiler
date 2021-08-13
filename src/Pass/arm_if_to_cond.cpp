#include "../../include/Pass/arm_if_to_cond.h"

#include "../../include/arm.h"
#include "../../include/arm_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_IFTOCOND_PROCESS

void IfToCond::Run() {
  auto m = dynamic_cast<ArmModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    Run4Func(func);
  }
}

void IfToCond::Run4Func(ArmFunction* func) {
  // 实际上也是简化arm的一种 但是simplify arm会给iftocond带来更多的优化机会 所以建议在simplify arm后跟iftocond pass
  for (auto bb_it = func->bb_list_.begin(); bb_it != func->bb_list_.end(); ++bb_it) {
    auto bb = *bb_it;
    for (auto it = bb->inst_list_.begin(); it != bb->inst_list_.end();) {
      if (auto src_inst = dynamic_cast<Branch*>(*it)) {
        ArmBasicBlock *next_bb = nullptr, *next_next_bb = nullptr;
        if (bb_it + 1 != func->bb_list_.end()) next_bb = *(bb_it + 1);
        if (bb_it + 2 != func->bb_list_.end()) next_next_bb = *(bb_it + 2);
        // 有以下限制条件:
        // 1. 该指令必须是有条件的跳转指令
        // 2. 该基本块之后至少有两个基本块 并且该跳转指令跳转到之后的第二个基本块
        // 3. 该基本块之后的第一个基本块只能有一个前驱 第二个基本块只能有两个前驱
        // 4. [HYPERPARAMETER] 下一个基本块内最多5条语句
        // 5. 下一个基本块内的指令如果有条件则不能带有跟该指令不同的条件
        if (src_inst->cond_ != Cond::AL && nullptr != next_bb && nullptr != next_next_bb &&
            src_inst->label_ == next_next_bb->label_ && next_bb->pred_.size() == 1 && next_next_bb->pred_.size() == 2 &&
            next_bb->inst_list_.size() < 6) {
          bool can_delete = true;
          for (auto next_it = next_bb->inst_list_.begin(); next_it != next_bb->inst_list_.end(); ++next_it) {
            auto inst = *next_it;
            if (inst->cond_ != Cond::AL && inst->cond_ != src_inst->cond_) {
              can_delete = false;
              break;
            }
          }
#ifdef DEBUG_IFTOCOND_PROCESS
          std::cout << "Check If to Cond BB:" << std::endl;
          next_bb->EmitCode(std::cout);
          std::cout << "  Result: " << can_delete << std::endl;
#endif
          if (can_delete) {
            auto cond = GetOppositeCond(src_inst->cond_);
            for (auto inst : next_bb->inst_list_) {
              inst->cond_ = cond;
            }
            it = bb->inst_list_.erase(it);  // pop_back
            continue;
          }
        }
        ++it;
      }
      // 其他直接跳过
      else {
        ++it;
      }
    }
  }
}