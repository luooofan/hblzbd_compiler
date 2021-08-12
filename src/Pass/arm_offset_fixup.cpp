#include "../../include/Pass/arm_offset_fixup.h"

#include "../../include/arm.h"
#include "../../include/arm_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_FIXUP_PROCESS

void SPOffsetFixup::Run() {
  auto m = dynamic_cast<ArmModule *>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    Fixup4Func(func);
  }
}

void SPOffsetFixup::Fixup4Func(ArmFunction *func) {
#ifdef DEBUG_FIXUP_PROCESS
  std::cout << "Prune Instruction Start:" << std::endl;
#endif
  // 每一个函数确定了之后 更改push/pop指令 更改add/sub sp sp stack_size指令
  int offset_fixup_diff = func->used_callee_saved_regs.size() * 4 + func->stack_size_;
  bool is_lr_used = func->used_callee_saved_regs.find((int)ArmReg::lr) != func->used_callee_saved_regs.end();

#ifdef DEBUG_FIXUP_PROCESS
  std::cout << "Modify Push Pop Start:" << std::endl;
#endif
  for (auto bb : func->bb_list_) {
    for (auto iter = bb->inst_list_.begin(); iter != bb->inst_list_.end();) {
      if (auto pushpop_inst = dynamic_cast<PushPop *>(*iter)) {
        // pushpop_inst->EmitCode(outfile);
        pushpop_inst->reg_list_.clear();
        for (auto reg : func->used_callee_saved_regs) {
          pushpop_inst->reg_list_.push_back(new Reg(reg));
        }
        if (pushpop_inst->opkind_ == PushPop::OpKind::PUSH) {
          if (pushpop_inst->reg_list_.empty()) {
            iter = bb->inst_list_.erase(iter);
            continue;
          }
        } else {
          if (is_lr_used) {  // 把pop lr 改为pop pc
            MyAssert(pushpop_inst->reg_list_.back()->reg_id_ == (int)ArmReg::lr);
            pushpop_inst->reg_list_.back()->reg_id_ = (int)ArmReg::pc;
          }
          if (pushpop_inst->reg_list_.empty()) {
            iter = bb->inst_list_.erase(iter);
          } else {
            ++iter;
          }
          // 此时iter指向原pop指令的下一条指令
          if (!is_lr_used) {  // 不在push中 插入一条 bx lr
            iter = bb->inst_list_.insert(iter, static_cast<Instruction *>(new Branch(false, true, Cond::AL, "lr")));
          }
          continue;
        }
        ++iter;
      } else {
        ++iter;
      }
    }
  }
#ifdef DEBUG_FIXUP_PROCESS
  std::cout << "Modify Push Pop End." << std::endl;
#endif

#ifdef DEBUG_FIXUP_PROCESS
  std::cout << "Fixup sp_arg Start:" << std::endl;
#endif
  // std::cout << offset_fixup_diff << " " << stack_size_diff << std::endl;
  // 针对sp_arg的修复 需要修复栈中高处实参的位置 一定是一条ldr-pseudo指令
  for (auto it = func->sp_arg_fixup_.begin(); it != func->sp_arg_fixup_.end(); ++it) {
    auto src_inst = dynamic_cast<LdrPseudo *>(*it);
    MyAssert(nullptr != src_inst && src_inst->IsImm());
    src_inst->imm_ += offset_fixup_diff;
  }
#ifdef DEBUG_FIXUP_PROCESS
  std::cout << "Fixup sp_arg End." << std::endl;
  std::cout << "Fixup sp Start:" << std::endl;
#endif
  auto convert_imm_inst = [&func](std::vector<Instruction *>::iterator it) {
    // 把mov/mvn转换成ldrpseudo
    for (auto bb : func->bb_list_) {
      for (auto inst_it = bb->inst_list_.begin(); inst_it != bb->inst_list_.end(); ++inst_it) {
        if (*inst_it == *it) {
          auto src_inst = dynamic_cast<Move *>(*it);
          MyAssert(nullptr != src_inst && src_inst->op2_->is_imm_);
          inst_it = bb->inst_list_.erase(inst_it);
          int imm = 0;
          if (src_inst->is_mvn_) {
            imm = -src_inst->op2_->imm_num_ - 1;
          } else {
            imm = src_inst->op2_->imm_num_;
          }
          auto pseudo_ldr_inst = static_cast<Instruction *>(new LdrPseudo(Cond::AL, src_inst->rd_, imm));
          inst_it = bb->inst_list_.insert(inst_it, pseudo_ldr_inst);
          // TODO: delete
          *it = pseudo_ldr_inst;
          return;
        }
      }
    }
    MyAssert(0);
  };

  auto merge_with_addsub = [&func](std::vector<Instruction *>::iterator it) {
    // 把mov/ldrpseudo和后面的addsub sp sp 合起来
    for (auto bb : func->bb_list_) {
      for (auto inst_it = bb->inst_list_.begin(); inst_it != bb->inst_list_.end(); ++inst_it) {
        if (*inst_it == *it) {
          int imm = 0;
          if (auto src_inst = dynamic_cast<Move *>(*it)) {
            MyAssert(src_inst->op2_->is_imm_);
            if (src_inst->is_mvn_) return;
            imm = src_inst->op2_->imm_num_;
          } else if (auto src_inst = dynamic_cast<LdrPseudo *>(*it)) {
            MyAssert(src_inst->IsImm());
            imm = src_inst->imm_;
          } else {
            MyAssert(0);
          }
          inst_it = bb->inst_list_.erase(inst_it);
          // inst_it现在指向了add或sub sp sp op2指令 要把op2改为立即数类型
          auto addsub_inst = dynamic_cast<BinaryInst *>(*inst_it);
          MyAssert(nullptr != addsub_inst && addsub_inst->rd_->reg_id_ == (int)ArmReg::sp &&
                   addsub_inst->rn_->reg_id_ == (int)ArmReg::sp);
          // TODO delete src op2
          addsub_inst->op2_ = new Operand2(imm);
          *it = addsub_inst;  // 之后sp_fixup中会出现addsub指令
          return;
        }
      }
    }
    MyAssert(0);
  };

  // 针对function prologue和epilogue中sp的修复 原来都是0 可能是mov mvn或者pseudo ldr
  for (auto it = func->sp_fixup_.begin(); it != func->sp_fixup_.end(); ++it) {
    if (auto src_inst = dynamic_cast<Move *>(*it)) {
      MyAssert(src_inst->op2_->is_imm_);
      if (src_inst->is_mvn_) {
        MyAssert(0);
        src_inst->op2_->imm_num_ = -func->stack_size_;
      } else {
        src_inst->op2_->imm_num_ = func->stack_size_;
      }
      if (!Operand2::CheckImm8m(src_inst->op2_->imm_num_)) {
        convert_imm_inst(it);
      } else {
        // 满足op2 可以和后面的add/sub合为一条语句
        merge_with_addsub(it);
      }
    } else if (auto src_inst = dynamic_cast<LdrPseudo *>(*it)) {
      MyAssert(src_inst->IsImm());
      src_inst->imm_ = func->stack_size_;
      if (Operand2::CheckImm8m(src_inst->imm_)) {
        // 满足op2 可以和后面的add/sub合为一条语句
        merge_with_addsub(it);
      }
    } else {
      MyAssert(0);
    }
  }
#ifdef DEBUG_FIXUP_PROCESS
  std::cout << "Fixup sp End." << std::endl;
#endif
}

#undef ASSERT_ENABLE