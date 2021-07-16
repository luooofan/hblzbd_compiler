#include "../include/arm_struct.h"

#include <cassert>
#include <iostream>
#include <string>
namespace arm {

void Module::EmitCode(std::ostream& outfile) {
  outfile << ".arch armv7ve" << std::endl;
  outfile << ".arm" << std::endl;
  auto& global_symtab = global_scope_.symbol_table_;
  if (!global_symtab.empty()) {
    outfile << ".section .data" << std::endl;
    outfile << ".align 4" << std::endl << std::endl;
    // TODO: order?
    for (auto& symbol : global_symtab) {
      outfile << ".global " << symbol.first << std::endl;
      outfile << "\t.type " << symbol.first << ", %object" << std::endl;
      outfile << symbol.first << ":" << std::endl;
      for (auto value : symbol.second.initval_) {
        outfile << "\t.word " << std::to_string(value) << std::endl;
      }
      outfile << std::endl;
    }
  }
  outfile << ".section .text" << std::endl;
  // outfile << ".syntax unified" << std::endl << std::endl;
  for (auto func : func_list_) {
    func->EmitCode(outfile);
  }
}

void Function::EmitCode(std::ostream& outfile) {
  outfile << ".global " << func_name_ << std::endl;
  outfile << "\t.type " << func_name_ << ", %function" << std::endl;
  outfile << func_name_ << ":" << std::endl;
  outfile << "@ call_func: " << std::endl;
  for (auto func : call_func_list_) {
    outfile << "  @ " << func->func_name_ << std::endl;
  }
  outfile << "@ Function Begin." << std::endl;
  for (auto bb : bb_list_) {
    bb->EmitCode(outfile);
  }
  outfile << "@ Function End." << std::endl;
  outfile << std::endl;
}

void BasicBlock::EmitCode(std::ostream& outfile) {
  if (this->HasLabel()) {
    outfile << *this->label_ << ":" << std::endl;
  }
  outfile << "  @ BasicBlock Begin." << std::endl;
  outfile << "  @ pred: ";
  for (auto pred : this->pred_) {
    outfile << (nullptr != (*pred).label_ ? (*((*pred).label_)) : ("Unamed"))
            << " ";
  }
  // outfile << std::endl;
  outfile << "  @ succ: ";
  for (auto succ : this->succ_) {
    outfile << (nullptr != (*succ).label_ ? (*((*succ).label_)) : ("Unamed"))
            << " ";
  }
  // outfile << std::endl;
  outfile << "  @ liveuse: ";
  for (auto liveuse : this->liveuse_) {
    outfile << liveuse << " ";
  }
  // outfile << std::endl;
  outfile << "  @ def: ";
  for (auto def : this->def_) {
    outfile << def << " ";
  }
  // outfile << std::endl;
  outfile << "  @ livein: ";
  for (auto livein : this->livein_) {
    outfile << livein << " ";
  }
  // outfile << std::endl;
  outfile << "  @ liveout: ";
  for (auto liveout : this->liveout_) {
    outfile << liveout << " ";
  }
  outfile << std::endl;
  for (auto inst : this->inst_list_) {
    inst->EmitCode(outfile);
    // std::clog << "print 1 inst successfully." << std::endl;
  }
  outfile << "  @ BasicBlock End." << std::endl;
}

}  // namespace arm
