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
            // std::cout << "mov to self." << std::endl;
            it = bb->inst_list_.erase(it);
          } else {
            ++it;
          }
        } else if (auto src_inst = dynamic_cast<BinaryInst*>(*it)) {
          // add 0 sub 0
          if (src_inst->opcode_ == BinaryInst::OpCode::ADD || src_inst->opcode_ == BinaryInst::OpCode::SUB) {
            if (src_inst->op2_->is_imm_ && 0 == src_inst->op2_->imm_num_) {
              it = bb->inst_list_.erase(it);
              it = bb->inst_list_.insert(
                  it, static_cast<Instruction*>(new Move(false, Cond::AL, src_inst->rd_, new Operand2(src_inst->rn_))));
              continue;  // 继续检查新加的这条mov指令
            }
          }
          ++it;
        } else {
          ++it;
        }
        // NOTE: 寄存器分配前不能删add vreg, sp, #0 不能删sub/add sp,sp,#0
      }
    }
  }
}