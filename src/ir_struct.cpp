#include "../include/ir_struct.h"

#include <algorithm>

#define ASSERT_ENABLE
#include "../include/myassert.h"

// #define GENIR_WITH_COMMENT

IRModule::~IRModule() {
  for (auto func : func_list_)
    if (nullptr != func) delete func;
}
void IRModule::EmitCode(std::ostream& out) {
  out << "@ module: " << this->name_ << std::endl;
  // this->global_scope_.Print(out);
  for (auto func : this->func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
}

IRFunction::~IRFunction() {
  for (auto bb : bb_list_)
    if (nullptr != bb) delete bb;
}
void IRFunction::EmitCode(std::ostream& out) {
  out << "@ Function: " << this->name_ << std::endl;
#ifdef GENIR_WITH_COMMENT
  out << "@ call_func: ";
  for (auto func : this->call_func_list_) {
    out << func->name_ << " ";
  }
  out << std::endl;
#endif
  out << "@ Function Begin:" << std::endl;
  out << std::endl;
  for (auto bb : this->bb_list_) {
    bb->EmitCode(out);
    out << std::endl;
  }
  out << "@ Function End." << std::endl;
}

IRBasicBlock::~IRBasicBlock() {
  for (auto ir : ir_list_)
    if (nullptr != ir) delete ir;
}
int IRBasicBlock::IndexInFunc() {
  MyAssert(nullptr != this->func_);
  auto& bbs = this->func_->bb_list_;
  MyAssert(std::find(bbs.begin(), bbs.end(), this) != bbs.end());
  return std::distance(bbs.begin(), std::find(bbs.begin(), bbs.end(), this));
}
// 该bb是不是被这个参数bb所支配 或者说参数bb是不是该bb的必经结点
bool IRBasicBlock::IsByDom(IRBasicBlock* bb) {
  auto n = this;
  while (nullptr != n) {
    if (bb == n) return true;
    n = n->idom_;
  }
  return false;
}
void IRBasicBlock::EmitCode(std::ostream& out) {
#ifdef GENIR_WITH_COMMENT
  out << "@ BasicBlock: id:" << this->IndexInFunc() << std::endl;
  out << "@ pred bbs: ";
  for (auto pred : this->pred_) {
    out << pred->IndexInFunc() << " ";
  }
  // out << std::endl;
  out << "@ succ bbs: ";
  for (auto succ : this->succ_) {
    out << succ->IndexInFunc() << " ";
  }
  out << std::endl;
  out << "@ idom: " << (nullptr == this->idom_ ? "" : std::to_string(this->idom_->IndexInFunc()) + " ");
  // out << std::endl;
  out << "@ doms: ";
  for (auto dom : this->doms_) {
    out << dom->IndexInFunc() << " ";
  }
  // out << std::endl;
  out << "@ df: ";
  for (auto df : this->df_) {
    out << df->IndexInFunc() << " ";
  }
  out << std::endl;
  out << "@ def: ";
  for (auto def : this->def_) {
    out << def << " ";
  }
  out << "@ use: ";
  for (auto use : this->use_) {
    out << use << " ";
  }
  out << "@ livein: ";
  for (auto livein : this->livein_) {
    out << livein << " ";
  }
  out << "@ liveout: ";
  for (auto liveout : this->liveout_) {
    out << liveout << " ";
  }
  out << std::endl;
#endif
  for (auto ir : this->ir_list_) {
    ir->PrintIR(out);
  }
}

void IRBasicBlock::EmitBackupCode(std::ostream& out) {
#ifdef GENIR_WITH_COMMENT
  out << "@ BasicBlock: id:" << this->IndexInFunc() << std::endl;
  out << "@ pred bbs: ";
  for (auto pred : this->pred_) {
    out << pred->IndexInFunc() << " ";
  }
  // out << std::endl;
  out << "@ succ bbs: ";
  for (auto succ : this->succ_) {
    out << succ->IndexInFunc() << " ";
  }
  out << std::endl;
  out << "@ idom: " << (nullptr == this->idom_ ? "" : std::to_string(this->idom_->IndexInFunc()) + " ");
  // out << std::endl;
  out << "@ doms: ";
  for (auto dom : this->doms_) {
    out << dom->IndexInFunc() << " ";
  }
  // out << std::endl;
  out << "@ df: ";
  for (auto df : this->df_) {
    out << df->IndexInFunc() << " ";
  }
  out << std::endl;
  out << "@ def: ";
  for (auto def : this->def_) {
    out << def << " ";
  }
  out << "@ use: ";
  for (auto use : this->use_) {
    out << use << " ";
  }
  out << "@ livein: ";
  for (auto livein : this->livein_) {
    out << livein << " ";
  }
  out << "@ liveout: ";
  for (auto liveout : this->liveout_) {
    out << liveout << " ";
  }
  out << std::endl;
#endif
  for (auto ir : this->ir_list_backup_) {
    ir->PrintIR(out);
  }
}

#undef ASSERT_ENABLE  // disable assert. this should be placed at the end of every file.
