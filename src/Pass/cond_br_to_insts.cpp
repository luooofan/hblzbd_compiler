#include "../../include/Pass/cond_br_to_insts.h"

#include "../../include/arm.h"
#include "../../include/arm_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_CONDBRTOINSTS_PROCESS
// #define OUTPUT_CONDBRTOINSTS_EFFECT

// NOTE: 限制最大指令数 不过因为mov32汇编宏的存在可能实际上不止4条
static const int kMaxInstNumInBB = 4;  // [HYPERPARAMETER]
static int kCount = 0;

void CondBrToInsts::Run() {
  kCount = 0;
  auto m = dynamic_cast<ArmModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    Run4Func(func);
  }
#ifdef OUTPUT_CONDBRTOINSTS_EFFECT
  std::cout << "CondBrToInsts Pass works on: " << kCount << " ArmBBs" << std::endl;
#endif
}

void CondBrToInsts::Run4Func(ArmFunction* func) {
#ifdef DEBUG_CONDBRTOINSTS_PROCESS
  std::cout << "Run CondBrToInsts In Func: " << func->name_ << std::endl;
  // if (func->name_ != "reduce") return; // debug for single function in single testcase
#endif
  // 实际上也是简化arm的一种 但是simplify arm会给CondBrToInsts带来更多的优化机会 所以建议在simplify arm后跟CondBrToInsts
  // pass
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
        // 3. 该基本块之后的第一个基本块只能有一个前驱 即该基本块
        // 4. [HYPERPARAMETER] 下一个基本块内最多5条语句
        // 5. 下一个基本块内的指令如果有条件则不能带有跟该指令不同的条件 除最后一条语句外不能有函数调用 即bl(x)语句
        if (src_inst->cond_ != Cond::AL && nullptr != next_bb && nullptr != next_next_bb &&
            src_inst->label_ == next_next_bb->label_ && next_bb->pred_.size() == 1 && next_bb->pred_.front() == bb &&
            next_bb->inst_list_.size() <= kMaxInstNumInBB) {
          bool can_delete = true;
          for (auto next_it = next_bb->inst_list_.begin(); next_it != next_bb->inst_list_.end(); ++next_it) {
            auto inst = *next_it;
            if (inst->cond_ != Cond::AL && inst->cond_ != src_inst->cond_) {
              can_delete = false;
              break;
            }
            if (next_it + 1 == next_bb->inst_list_.end()) break;
            if (auto src_inst = dynamic_cast<Branch*>(inst)) {
              if (src_inst->has_l_) {
                can_delete = false;
                break;
              }
            }
          }
#ifdef DEBUG_CONDBRTOINSTS_PROCESS
          std::cout << "Check If to Cond BB:" << std::endl;
          next_bb->EmitCode(std::cout);
          std::cout << "  Result: " << can_delete << " " << CondToString(GetOppositeCond(src_inst->cond_)) << std::endl;
#endif
          if (can_delete) {
            ++kCount;
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