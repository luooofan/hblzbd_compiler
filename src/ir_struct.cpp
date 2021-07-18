#include "../include/ir_struct.h"

void IRModule::EmitCode(std::ostream& out) {
  out << "@ module: " << this->name_ << std::endl;
  this->global_scope_.Print();
  for (auto func : this->func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
}

void IRFunction::EmitCode(std::ostream& out) {
  out << "@ Function: " << this->name_ << " size:" << this->stack_size_
      << std::endl;
  out << "@ call_func: ";
  for (auto func : this->call_func_list_) {
    out << func->name_ << " ";
  }
  out << std::endl;
  out << "@ Function Begin:" << std::endl;
  for (auto bb : this->bb_list_) {
    bb->EmitCode(out);
  }
  out << "@ Function End." << std::endl;
}

void IRBasicBlock::EmitCode(std::ostream& out) {
  out << "@ BasicBlock: start_ir: " << start_ << " end_ir: " << end_
      << std::endl;
  out << "@ pred bbs: " << std::endl;
  for (auto pred : this->pred_) {
    out << "\t@ " << pred->start_ << " " << pred->end_ << std::endl;
  }
  out << "@ succ bbs: " << std::endl;
  for (auto succ : this->succ_) {
    out << "\t@ " << succ->start_ << " " << succ->end_ << std::endl;
  }
  for (int i = start_; i < end_; ++i) {
    ir::gIRList[i].PrintIR();
  }
}
