#include "../include/ir_getdefuse.h"
#define ASSERT_ENABLE
#include "../include/myassert.h"

std::pair<std::vector<Opn *>, std::vector<Opn *>> GetDefUsePtr(IR *ir, bool consider_array) {
  std::vector<Opn *> def;
  std::vector<Opn *> use;

  auto process_opn = [&use, &consider_array](Opn *op) {
    if (nullptr == op) return;
    if (op->type_ == Opn::Type::Imm || op->type_ == Opn::Type::Label || op->type_ == Opn::Type::Func ||
        op->type_ == Opn::Type::Null)
      return;
    if (op->type_ == Opn::Type::Array) {
      if (consider_array) use.push_back(op);
      if (op->offset_->type_ == Opn::Type::Var) use.push_back(op->offset_);
    } else {  // var
      use.push_back(op);
    }
  };

  auto process_res = [&def, &use, &consider_array](Opn *res) {
    if (nullptr == res) return;
    if (res->type_ == Opn::Type::Label || res->type_ == Opn::Type::Func || res->type_ == Opn::Type::Null) return;
    if (res->type_ == Opn::Type::Array) {
      if (consider_array) def.push_back(res);
      if (res->offset_->type_ == Opn::Type::Var) use.push_back(res->offset_);
    } else {  // var
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
  } else if (ir->op_ == IR::OpKind::ALLOCA) {  // 跳过
  } else {
    MyAssert(0);
  }

  return {def, use};
}

//变量形式：变量名_#scope_id
std::pair<std::vector<std::string>, std::vector<std::string>> GetDefUse(IR *ir, bool consider_array) {
  std::vector<std::string> def;
  std::vector<std::string> use;

  auto process_opn = [&use, &consider_array](Opn *op) {
    if (nullptr == op) return;
    if (op->type_ == Opn::Type::Imm || op->type_ == Opn::Type::Label || op->type_ == Opn::Type::Func ||
        op->type_ == Opn::Type::Null)
      return;
    if (op->type_ == Opn::Type::Array) {
      if (consider_array) use.push_back(op->name_ + "." + std::to_string(op->scope_id_));
      if (op->offset_->type_ == Opn::Type::Var)
        use.push_back(op->offset_->name_ + "." + std::to_string(op->offset_->scope_id_));
    } else {  // var
      use.push_back(op->name_ + "." + std::to_string(op->scope_id_));
    }
  };

  auto process_res = [&def, &use, &consider_array](Opn *res) {
    if (nullptr == res) return;
    if (res->type_ == Opn::Type::Label || res->type_ == Opn::Type::Func || res->type_ == Opn::Type::Null) return;
    if (res->type_ == Opn::Type::Array) {
      if (consider_array) def.push_back(res->name_ + "." + std::to_string(res->scope_id_));
      if (res->offset_->type_ == Opn::Type::Var)
        use.push_back(res->offset_->name_ + "." + std::to_string(res->offset_->scope_id_));
    } else {
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
  } else if (ir->op_ == IR::OpKind::ALLOCA) {  // 跳过
  } else {
    MyAssert(0);
  }

  return {def, use};
}

void GetDefUse4Func(IRFunction *f, bool consider_array) {
  for (auto bb : f->bb_list_) {
    // bb->livein_.clear();
    // bb->liveout_.clear();
    bb->def_.clear();
    bb->use_.clear();

    for (auto ir : bb->ir_list_) {
      auto [def, use] = GetDefUse(ir, consider_array);

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