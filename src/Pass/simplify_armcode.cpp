#include "../../include/Pass/simplify_armcode.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"

void SimplifyArm::Run() {
  auto m = dynamic_cast<ArmModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    for (auto bb : func->bb_list_) {
      for (auto it = bb->inst_list_.begin(); it != bb->inst_list_.end();) {
        // 1 删除自己到自己的mov语句
        // 2 带偏移的 rd只在下一条语句中作为op2被使用的 mov语句 可以和下一条语句合为一句
        // e.g. mov rd, rn LSL #2; add rd2, rn2, rd; -> add rd2, rn2, rn LSL #2
        //      mov rd, rn LSL #2; ldr/str rd2, [rn2, rd]; -> ldr/str rd2, [rn2, rn LSL #2]
        if (auto src_inst = dynamic_cast<Move*>(*it)) {
          // 1
          auto mov_op2 = src_inst->op2_;
          if (!mov_op2->is_imm_ && nullptr == mov_op2->shift_ && src_inst->rd_->reg_id_ == mov_op2->reg_->reg_id_) {
            // std::cout << "mov to self." << std::endl;
            // 删了之后检查下一条指令
            it = bb->inst_list_.erase(it);
            continue;
          }
          // 2
          // 没有下一条语句直接break出去
          if (it + 1 == bb->inst_list_.end()) break;
          // 如果结果寄存器的使用只有一个并且是下一条语句的op2才可以
          auto next_inst = *(it + 1);
          auto mov_rd = src_inst->rd_;
          if (mov_op2->HasShift() && mov_rd->GetUsedInstNum() == 1 &&
              (*(mov_rd->GetUsedInsts().begin())) == next_inst) {
            // src_inst->EmitCode(std::cout);
            // next_inst->EmitCode(std::cout);
            // 下一条语句是binaryinst
            if (auto next_bi_inst = dynamic_cast<BinaryInst*>(next_inst)) {
              auto op2 = next_bi_inst->op2_;
              if (nullptr != op2 && !op2->HasShift() && !op2->is_imm_ && op2->reg_ == mov_rd) {
                next_bi_inst->op2_ = mov_op2;
                // TODO delete mov指令 delete mov_rd 从instlist中删除
                it = bb->inst_list_.erase(it);
                // 此时it指向被修改的binary inst 之后it++不对该指令检查
              }
            }
            // 下一条语句是moveinst
            else if (auto next_move_inst = dynamic_cast<Move*>(next_inst)) {
              auto op2 = next_move_inst->op2_;
              if (nullptr != op2 && !op2->HasShift() && !op2->is_imm_ && op2->reg_ == mov_rd) {
                next_move_inst->op2_ = mov_op2;
                // TODO delete mov指令 delete mov_rd 从instlist中删除
                it = bb->inst_list_.erase(it);
                // 此时it指向被修改的binary inst 之后it++不对该指令检查
              }
            }
            // 下一条语句是ldrstr inst
            else if (auto next_ldrstr_inst = dynamic_cast<LdrStr*>(next_inst)) {
              auto op2 = next_ldrstr_inst->offset_;
              if (nullptr != op2 && !op2->HasShift() && !op2->is_imm_ && op2->reg_ == mov_rd) {
                next_ldrstr_inst->offset_ = mov_op2;
                // TODO delete mov指令 delete mov_rd 从instlist中删除
                it = bb->inst_list_.erase(it);
                // 此时it指向被修改的binary inst 之后it++不对该指令检查
              }
            }
          }
          ++it;
        }
        // 3 代数化简
        else if (auto src_inst = dynamic_cast<BinaryInst*>(*it)) {
          // 2.1 add 0 sub 0 转换成mov指令 但是不能删除rd和rn都是sp的指令
          if (src_inst->opcode_ == BinaryInst::OpCode::ADD || src_inst->opcode_ == BinaryInst::OpCode::SUB) {
            if (src_inst->op2_->is_imm_ && 0 == src_inst->op2_->imm_num_ &&
                (src_inst->rd_->reg_id_ != (int)ArmReg::sp || src_inst->rn_->reg_id_ != (int)ArmReg::sp)) {
              it = bb->inst_list_.erase(it);
              it = bb->inst_list_.insert(
                  it, static_cast<Instruction*>(new Move(false, Cond::AL, src_inst->rd_, new Operand2(src_inst->rn_))));
              // 继续检查新加的这条mov指令
              continue;
            }
          }
          ++it;
        }

        else if (auto src_inst = dynamic_cast<Move*>(*it)) {
        }
        // 其他无法简化直接跳过
        else {
          ++it;
        }
      }
    }
  }
}