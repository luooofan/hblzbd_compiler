#include "../../include/Pass/simplify_armcode.h"

#include <algorithm>
#define ASSERT_ENABLE
#include "../../include/myassert.h"

void SimplifyArm::Run() {
  auto m = dynamic_cast<ArmModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    for (auto bb_it = func->bb_list_.begin(); bb_it != func->bb_list_.end(); ++bb_it) {
      // for (auto bb : func->bb_list_) {
      auto bb = *bb_it;
      for (auto it = bb->inst_list_.begin(); it != bb->inst_list_.end();) {
        // (*it)->EmitCode(std::cout);
        // Move语句 两种简化可能
        if (auto src_inst = dynamic_cast<Move*>(*it)) {
          // 1 删除自己到自己的mov语句
          auto mov_op2 = src_inst->op2_;
          if (!mov_op2->is_imm_ && nullptr == mov_op2->shift_ && src_inst->rd_->reg_id_ == mov_op2->reg_->reg_id_) {
            // 删了之后检查下一条指令
            it = bb->inst_list_.erase(it);
            continue;
          }
          // 2 带偏移的 rd只作为一个无偏移的op2被使用的 所有使用和定值都在一个块内的 mov语句 可以和其所有使用合为一句
          // NOTE: 可以在一定程度上更利于寄存器分配 TODO: 需要确保使用中间没有新的定值
          // e.g. mov rd, rn LSL #2; add rd2, rn2, rd; -> add rd2, rn2, rn LSL #2
          //      mov rd, rn LSL #2; ldr/str rd2, [rn2, rd]; -> ldr/str rd2, [rn2, rn LSL #2]
          auto mov_rd = src_inst->rd_;
          if (mov_op2->HasShift() && mov_rd->reg_id_ > 15 /*4?*/ &&
              src_inst->cond_ == Cond::AL) {  // NOTE HERE! 预着色的寄存器可能有其他作用
            bool can_delete = true;
            for (auto used_inst : mov_rd->GetUsedInsts()) {
              // 如果有一条语句不是把它用作一个没有偏移的op2 那么就不执行替换
              if (!used_inst->HasUsedAsOp2WithoutShift(mov_rd) || used_inst->parent_ != bb) {
                can_delete = false;
                break;
              }
            }
            if (can_delete) {
              // bb->EmitCode(std::cout);
              // 替换所有使用
              for (auto used_inst : mov_rd->GetUsedInsts()) {
                used_inst->ReplaceOp2With(mov_op2);
              }
              // 删除move语句
              // std::cout << "will delete:", src_inst->EmitCode(std::cout);
              it = bb->inst_list_.erase(it);
              continue;
            }
          }
          ++it;
        }
        // 3 代数化简
        else if (auto src_inst = dynamic_cast<BinaryInst*>(*it)) {
          // 2.1 add 0 sub 0 转换成mov指令 但是不能在sp_fixup中的指令
          if (src_inst->opcode_ == BinaryInst::OpCode::ADD || src_inst->opcode_ == BinaryInst::OpCode::SUB) {
            if (src_inst->op2_->is_imm_ && 0 == src_inst->op2_->imm_num_ && !func->sp_fixup_.count(src_inst)) {
              it = bb->inst_list_.erase(it);
              it = bb->inst_list_.insert(
                  it, new Move(false, Cond::AL, src_inst->rd_, new Operand2(src_inst->rn_), bb, false, false));
              // new Move(src_inst->rd_, new Operand2(src_inst->rn_), src_inst);
              // it = std::find(bb->inst_list_.begin(), bb->inst_list_.end(), src_inst);
              // MyAssert(it != bb->inst_list_.end());
              // it = bb->inst_list_.erase(it);
              // 继续检查新加的这条mov指令
              continue;
            }
          }
          ++it;
        }
        // 4 无条件跳转到下一个基本的跳转语句化简
        else if (auto src_inst = dynamic_cast<Branch*>(*it)) {
          if (src_inst->cond_ == Cond::AL && bb_it + 1 != func->bb_list_.end() &&
              src_inst->label_ == (*(bb_it + 1))->label_) {
            it = bb->inst_list_.erase(it);
            continue;
          }
          ++it;
        }
        // 其他无法简化直接跳过
        else {
          ++it;
        }
      }
      // std::cout << "BB End" << std::endl;
    }
    // std::cout << "Function End" << std::endl;
  }
}