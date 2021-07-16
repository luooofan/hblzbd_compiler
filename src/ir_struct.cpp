#include "../include/ir_struct.h"

namespace ir {

void Module::Print() {
  global_scope_.Print();
  for (auto func : func_list_) {
    func->Print();
  }
}

void Function::Print() {
  std::cout << "Function: " << func_name_ << " size:" << stack_size_
            << std::endl;
  std::cout << "call_func: " << std::endl;
  for (auto func : call_func_list_) {
    std::cout << "\t" << func->func_name_ << std::endl;
  }
  std::cout << "BasicBlocks Begin:==============" << std::endl;
  for (auto bb : bb_list_) {
    bb->Print();
  }
  std::cout << "BasicBloks End.=================" << std::endl;
  std::cout << std::endl;
}

void BasicBlock::Print() {
  std::cout << "BasicBlock: start_ir: " << start_ << " end_ir: " << end_
            << std::endl;
  std::cout << "pred bbs: " << std::endl;
  for (auto pred_bb : pred_) {
    std::cout << "\t" << pred_bb->start_ << " " << pred_bb->end_ << std::endl;
  }
  std::cout << "succ bbs: " << std::endl;
  for (auto succ_bb : succ_) {
    std::cout << "\t" << succ_bb->start_ << " " << succ_bb->end_ << std::endl;
  }
  for (int i = start_; i < end_; ++i) {
    gIRList[i].PrintIR();
  }
}

}  // namespace ir