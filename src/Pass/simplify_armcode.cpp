#include "../../include/Pass/simplify_armcode.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"

void SimplifyArm::Run() {
  auto m = dynamic_cast<ArmModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    for (auto bb : func->bb_list_) {
      for (auto it = bb->inst_list_.begin(); it != bb->inst_list_.end();) {
        if (auto src_inst = dynamic_cast<Move*>(*it)) {  // 删除自己到自己的mov语句
          auto op2 = src_inst->op2_;
          if (!op2->is_imm_ && nullptr == op2->shift_ && src_inst->rd_->reg_id_ == op2->reg_->reg_id_) {
            it = bb->inst_list_.erase(it);
          } else {
            ++it;
          }
        } else {
          ++it;
        }
        // NOTE: 寄存器分配前不能删add vreg, sp, #0 不能删sub/add sp,sp,#0
      }
    }
  }
}