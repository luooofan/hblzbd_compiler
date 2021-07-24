#include "../../include/Pass/arm_liveness_analysis.h"

#include <cassert>

std::pair<std::vector<RegId>, std::vector<RegId>> GetDefUse(Instruction *inst) {
  std::vector<RegId> def;
  std::vector<RegId> use;

  auto process_op2 = [&use](Operand2 *op2) {
    if (nullptr == op2) return;
    if (op2->is_imm_) return;
    use.push_back(op2->reg_->reg_id_);
    if (nullptr != op2->shift_ && !op2->shift_->is_imm_) {
      use.push_back(op2->shift_->shift_reg_->reg_id_);
    }
    return;
  };

  if (auto src_inst = dynamic_cast<BinaryInst *>(inst)) {
    if (nullptr != src_inst->rd_) {
      def.push_back(src_inst->rd_->reg_id_);
    }
    use.push_back(src_inst->rn_->reg_id_);
    process_op2(src_inst->op2_);
  } else if (auto src_inst = dynamic_cast<Move *>(inst)) {
    def.push_back(src_inst->rd_->reg_id_);
    process_op2(src_inst->op2_);
  } else if (auto src_inst = dynamic_cast<Branch *>(inst)) {
    // call可先视为对前面已经定义的参数寄存器的使用 再视作对调用者保护的寄存器的定义 不管之后这些寄存器有没有被使用
    // caller save regs: r0-r3, lr, ip
    // ret可视为对r0和lr的使用 但需要保证armcode是正确的 i.e. 如果前面有bl xxx指令的话 后面不能用bx lr指令返回
    // 如果b后面是个函数label就是call 如果b后面是lr就是ret
    auto &label = src_inst->label_;
    if (src_inst->IsRet()) {  // bx lr
      use.push_back(static_cast<RegId>(ArmReg::r0));
      use.push_back(static_cast<RegId>(ArmReg::lr));
    } else if (src_inst->IsCall()) {  // call
      int callee_param_num = ir::gFuncTable[label].shape_list_.size();
      for (RegId i = 0; i < std::min(callee_param_num, 4); ++i) {
        use.push_back(i);
      }
      for (RegId i = 0; i < 4; ++i) {
        def.push_back(i);
      }
      def.push_back(static_cast<RegId>(ArmReg::ip));
      def.push_back(static_cast<RegId>(ArmReg::lr));
    }
  } else if (auto src_inst = dynamic_cast<LdrStr *>(inst)) {
    if (src_inst->opkind_ == LdrStr::OpKind::LDR) {
      def.push_back(src_inst->rd_->reg_id_);
    } else {
      use.push_back(src_inst->rd_->reg_id_);
    }
    use.push_back(src_inst->rn_->reg_id_);
    process_op2(src_inst->offset_);
  } else if (auto src_inst = dynamic_cast<LdrPseudo *>(inst)) {
    def.push_back(src_inst->rd_->reg_id_);
  } else if (auto src_inst = dynamic_cast<PushPop *>(inst)) {
    if (src_inst->opkind_ == PushPop::OpKind::PUSH) {
      for (auto reg : src_inst->reg_list_) {
        use.push_back(reg->reg_id_);
      }
    } else {
      for (auto reg : src_inst->reg_list_) {
        if (reg->reg_id_ == static_cast<RegId>(ArmReg::pc)) {
          // 此时作为ret语句 视为对r0的使用以及对pc的定义
          use.push_back(static_cast<RegId>(ArmReg::r0));
        }
        def.push_back(reg->reg_id_);
      }
    }
  } else {  // 未实现其他指令
    // assert(0);
    if (1) {
      std::cerr << "Assert: " << __FILE__ << " " << __LINE__ << std::endl;
      exit(1);
    }
  }

  return {def, use};
}

std::pair<std::vector<Reg *>, std::vector<Reg *>> GetDefUsePtr(Instruction *inst) {
  std::vector<Reg *> def;
  std::vector<Reg *> use;

  auto process_op2 = [&use](Operand2 *op2) {
    if (nullptr == op2) return;
    if (op2->is_imm_) return;
    use.push_back(op2->reg_);
    if (nullptr != op2->shift_ && !op2->shift_->is_imm_) {
      use.push_back(op2->shift_->shift_reg_);
    }
    return;
  };

  if (auto src_inst = dynamic_cast<BinaryInst *>(inst)) {
    if (nullptr != src_inst->rd_) {
      def.push_back(src_inst->rd_);
    }
    use.push_back(src_inst->rn_);
    process_op2(src_inst->op2_);
  } else if (auto src_inst = dynamic_cast<Move *>(inst)) {
    def.push_back(src_inst->rd_);
    process_op2(src_inst->op2_);
  } else if (auto src_inst = dynamic_cast<Branch *>(inst)) {
    if (src_inst->has_x_ && src_inst->label_ == "lr") {  // bx lr 视为对r0和lr的使用
      use.push_back(new Reg(ArmReg::r0));
      use.push_back(new Reg(ArmReg::lr));
    }
  } else if (auto src_inst = dynamic_cast<LdrStr *>(inst)) {
    if (src_inst->opkind_ == LdrStr::OpKind::LDR) {
      def.push_back(src_inst->rd_);
    } else {
      use.push_back(src_inst->rd_);
    }
    use.push_back(src_inst->rn_);
    process_op2(src_inst->offset_);
  } else if (auto src_inst = dynamic_cast<LdrPseudo *>(inst)) {
    def.push_back(src_inst->rd_);
  } else if (auto src_inst = dynamic_cast<PushPop *>(inst)) {
    if (src_inst->opkind_ == PushPop::OpKind::PUSH) {
      for (auto reg : src_inst->reg_list_) {
        use.push_back(reg);
      }
    } else {
      for (auto reg : src_inst->reg_list_) {
        if (reg->reg_id_ == static_cast<RegId>(ArmReg::pc)) {
          // 此时作为ret语句 视为对r0的使用以及对pc的定义
          use.push_back(new Reg(ArmReg::r0));
        }
        def.push_back(reg);
      }
    }
  } else {  // 未实现其他指令
    // assert(0);
    if (1) {
      std::cerr << "Assert: " << __FILE__ << " " << __LINE__ << std::endl;
      exit(1);
    }
  }

  return {def, use};
}

void ArmLivenessAnalysis::Run4Func(ArmFunction *f) {
  for (auto bb : f->bb_list_) {
    // NOTE: 发生实际溢出时会重新分析 需要先清空
    bb->livein_.clear();
    bb->liveout_.clear();
    bb->def_.clear();
    bb->use_.clear();
    // 对于每一条指令 取def和use并记录到basicblock中 加快数据流分析
    for (auto inst : bb->inst_list_) {
      auto [def, use] = GetDefUse(inst);
      for (auto &u : use) {
        // liveuse not use. if in def, not liveuse
        if (bb->def_.find(u) == bb->def_.end()) {
          bb->use_.insert(u);
        }
      }
      for (auto &d : def) {
        if (bb->use_.find(d) == bb->use_.end()) {
          bb->def_.insert(d);
        }
      }
    }
    // liveuse是入口活跃的那些use
    // def是非入口活跃use的那些def
    bb->livein_ = bb->use_;
  }

  // 为每一个basicblock计算livein和liveout
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb : f->bb_list_) {
      std::unordered_set<RegId> new_out;
      // out[s]=U(succ:i)in[i]
      // 对每一个后继基本块
      // std::cout<<bb->succ_.size()<<std::endl;
      for (auto succ : bb->succ_) {
        //   if(succ){
        new_out.insert(succ->livein_.begin(), succ->livein_.end());
        //   }
      }

      if (new_out != bb->liveout_) {
        // 还未到达不动点
        changed = true;
        bb->liveout_ = new_out;
        // in[s]=use[s]U(out[s]-def[s]) 可增量添加
        for (auto s : bb->liveout_) {
          if (bb->def_.find(s) == bb->def_.end()) {
            bb->livein_.insert(s);  // 重复元素插入无副作用
          }
        }
      }
    }  // for every bb
  }    // while end}
}

void ArmLivenessAnalysis::Run() {
  auto m = dynamic_cast<ArmModule *>(*(this->m_));
  // assert(nullptr != m);
  if (nullptr == m) {
    std::cerr << "Assert: " << __FILE__ << " " << __LINE__ << std::endl;
    exit(1);
  }
  for (auto func : m->func_list_) {
    this->Run4Func(func);
  }
}