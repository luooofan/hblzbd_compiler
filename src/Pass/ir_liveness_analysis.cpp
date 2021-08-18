#include "../../include/Pass/ir_liveness_analysis.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"

std::pair<std::vector<Opn *>, std::vector<Opn *>> GetDefUsePtr(IR *ir, bool consider) {
  std::vector<Opn *> def;
  std::vector<Opn *> use;

  auto process_opn = [&use, &consider](Opn *op) {
    if (nullptr == op) return;
    if (op->type_ == Opn::Type::Imm || op->type_ == Opn::Type::Label || op->type_ == Opn::Type::Func ||
        op->type_ == Opn::Type::Null)
      return;
    if (op->type_ == Opn::Type::Array) {
      if (consider) use.push_back(op);
      if (op->offset_->type_ == Opn::Type::Var)
        if (consider || 0 != op->offset_->scope_id_) use.push_back(op->offset_);
    } else {  // var
      if (!consider && 0 == op->scope_id_) return;
      use.push_back(op);
    }
  };

  auto process_res = [&def, &use, &consider](Opn *res) {
    if (nullptr == res) return;
    if (res->type_ == Opn::Type::Label || res->type_ == Opn::Type::Func || res->type_ == Opn::Type::Null) return;
    if (res->type_ == Opn::Type::Array) {
      if (consider) def.push_back(res);
      if (res->offset_->type_ == Opn::Type::Var)
        if (consider || 0 != res->offset_->scope_id_) use.push_back(res->offset_);
    } else {  // var
      if (!consider && 0 == res->scope_id_) return;
      def.push_back(res);
    }
  };

  if (ir->op_ == IR::OpKind::ADD || ir->op_ == IR::OpKind::SUB || ir->op_ == IR::OpKind::MUL ||
      ir->op_ == IR::OpKind::DIV || ir->op_ == IR::OpKind::MOD) {
    process_opn(&(ir->opn1_));
    process_opn(&(ir->opn2_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::NOT || ir->op_ == IR::OpKind::NEG) {
    process_opn(&(ir->opn1_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::LABEL) {
    return {def, use};
  } else if (ir->op_ == IR::OpKind::PARAM) {
    process_opn(&(ir->opn1_));
  } else if (ir->op_ == IR::OpKind::CALL) {
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::RET) {
    process_opn(&(ir->opn1_));
  } else if (ir->op_ == IR::OpKind::GOTO) {
    return {def, use};
  } else if (ir->op_ == IR::OpKind::ASSIGN) {
    process_opn(&(ir->opn1_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::JEQ || ir->op_ == IR::OpKind::JNE || ir->op_ == IR::OpKind::JLT ||
             ir->op_ == IR::OpKind::JLE || ir->op_ == IR::OpKind::JGT || ir->op_ == IR::OpKind::JGE) {
    process_opn(&(ir->opn1_));
    process_opn(&(ir->opn2_));
  } else if (ir->op_ == IR::OpKind::VOID) {
    return {def, use};
  } else if (ir->op_ == IR::OpKind::ASSIGN_OFFSET) {
    process_opn(&(ir->opn1_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::PHI) {
    for (auto &arg : ir->phi_args_) {
      process_opn(&(arg));
    }
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::ALLOCA) {
    process_res(&(ir->opn1_));
  } else if (ir->op_ == IR::OpKind::DECLARE) {
    process_res(&(ir->opn1_));
  } else {
    MyAssert(0);
  }

  return {def, use};
}

//变量形式：变量名_#scope_id
std::pair<std::vector<std::string>, std::vector<std::string>> GetDefUse(IR *ir, bool consider) {
  std::vector<std::string> def;
  std::vector<std::string> use;

  auto process_opn = [&use, &consider](Opn *op) {
    if (nullptr == op) return;
    if (op->type_ == Opn::Type::Imm || op->type_ == Opn::Type::Label || op->type_ == Opn::Type::Func ||
        op->type_ == Opn::Type::Null)
      return;
    if (op->type_ == Opn::Type::Array) {
      if (consider) use.push_back(op->name_ + "." + std::to_string(op->scope_id_));
      if (op->offset_->type_ == Opn::Type::Var)
        if (consider || 0 != op->offset_->scope_id_)
          use.push_back(op->offset_->name_ + "." + std::to_string(op->offset_->scope_id_));
    } else {  // var
      if (!consider && 0 == op->scope_id_) return;
      use.push_back(op->name_ + "." + std::to_string(op->scope_id_));
    }
  };

  auto process_res = [&def, &use, &consider](Opn *res) {
    if (nullptr == res) return;
    if (res->type_ == Opn::Type::Label || res->type_ == Opn::Type::Func || res->type_ == Opn::Type::Null) return;
    if (res->type_ == Opn::Type::Array) {
      if (consider) def.push_back(res->name_ + "." + std::to_string(res->scope_id_));
      if (res->offset_->type_ == Opn::Type::Var)
        if (consider || 0 != res->offset_->scope_id_)
          use.push_back(res->offset_->name_ + "." + std::to_string(res->offset_->scope_id_));
    } else {
      if (!consider && 0 == res->scope_id_) return;
      def.push_back(res->name_ + "." + std::to_string(res->scope_id_));
    }
  };

  if (ir->op_ == IR::OpKind::ADD || ir->op_ == IR::OpKind::SUB || ir->op_ == IR::OpKind::MUL ||
      ir->op_ == IR::OpKind::DIV || ir->op_ == IR::OpKind::MOD) {
    process_opn(&(ir->opn1_));
    process_opn(&(ir->opn2_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::NOT || ir->op_ == IR::OpKind::NEG) {
    process_opn(&(ir->opn1_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::LABEL) {
    return {def, use};
  } else if (ir->op_ == IR::OpKind::PARAM) {
    process_opn(&(ir->opn1_));
  } else if (ir->op_ == IR::OpKind::CALL) {
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::RET) {
    process_opn(&(ir->opn1_));
  } else if (ir->op_ == IR::OpKind::GOTO) {
    return {def, use};
  } else if (ir->op_ == IR::OpKind::ASSIGN) {
    process_opn(&(ir->opn1_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::JEQ || ir->op_ == IR::OpKind::JNE || ir->op_ == IR::OpKind::JLT ||
             ir->op_ == IR::OpKind::JLE || ir->op_ == IR::OpKind::JGT || ir->op_ == IR::OpKind::JGE) {
    process_opn(&(ir->opn1_));
    process_opn(&(ir->opn2_));
  } else if (ir->op_ == IR::OpKind::VOID) {
    return {def, use};
  } else if (ir->op_ == IR::OpKind::ASSIGN_OFFSET) {
    process_opn(&(ir->opn1_));
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::PHI) {
    for (auto &arg : ir->phi_args_) {
      process_opn(&(arg));
    }
    process_res(&(ir->res_));
  } else if (ir->op_ == IR::OpKind::ALLOCA) {
    process_res(&(ir->opn1_));
  } else if (ir->op_ == IR::OpKind::DECLARE) {
    process_res(&(ir->opn1_));
  } else {
    MyAssert(0);
  }

  return {def, use};
}

bool isFound(std::unordered_set<Opn *> &inbb, Opn *inir) {
  for (auto i : inbb) {
    if (i->name_ == inir->name_ && i->scope_id_ == inir->scope_id_) {
      return true;
    }
  }
  return false;
}

void IRLivenessAnalysis::GetDefUse4Func(IRFunction *f, bool consider) {
  for (auto bb : f->bb_list_) {
    // bb->livein_.clear();
    // bb->liveout_.clear();
    bb->def_.clear();
    bb->use_.clear();

    for (auto ir : bb->ir_list_) {
      auto [def, use] = GetDefUse(ir, consider);

      for (auto &u : use) {
        bb->use_.insert(u);
      }

      for (auto &d : def) {
        bb->def_.insert(d);
      }
    }
    // bb->livein_ = bb->use_;
  }  // for bb
}

void IRLivenessAnalysis::Run4Func(IRFunction *f) {
  for (auto bb : f->bb_list_) {
    bb->livein_.clear();
    bb->liveout_.clear();
    bb->def_.clear();
    bb->use_.clear();

    for (auto ir : bb->ir_list_) {
      auto [def, use] = GetDefUse(ir);

      for (auto &u : use) {
        if (bb->def_.find(u) ==
            bb->def_.end()) {  // 如果在use之前没有def，则将其插入use中，说明该变量在bb中首次出现是以use的形式出现
          bb->use_.insert(u);
        }
      }  // for use

      for (auto &d : def) {
        if (bb->use_.find(d) ==
            bb->use_.end()) {  // 如果在def之前没有use，则将其插入def中，说明该变量首次出现是以def形式出现
          bb->def_.insert(d);
        }
      }  // for def
    }    // for ir
    bb->livein_ = bb->use_;
  }  // for bb

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb : f->bb_list_) {
      std::unordered_set<std::string> new_out;

      for (auto succ : bb->succ_) {  // 每个bb的liveout是其后继bb的livein的并集
        new_out.insert(succ->livein_.begin(), succ->livein_.end());
      }

      if (new_out != bb->liveout_) {
        changed = true;
        bb->liveout_ = new_out;

        for (auto s : bb->liveout_) {  // IN[B] = use[B]U(out[B] - def[B])
          if (bb->def_.find(s) == bb->def_.end()) {
            bb->livein_.insert(s);
          }
        }
      }
    }  // for bb
  }    // while changed
}

void IRLivenessAnalysis::Run() {
  auto m = dynamic_cast<IRModule *>(*(this->m_));

  MyAssert(nullptr != m);

  for (auto func : m->func_list_) {
    // std::cout << "analysis func:" << func->name_ << std::endl;
    this->Run4Func(func);
  }
}

#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif