#include "../include/ssa_struct.h"

#include "../include/ssa.h"

void SSAModule::AddFunction(SSAFunction* f) {
  func_list_.push_back(f);
  f->SetModule(this);
}
void SSAModule::RemoveFunction(SSAFunction* f) {
  func_list_.remove(f);
  // TODO: manage memory
};
void SSAModule::AddGlobalVariable(GlobalVariable* g) {
  glob_var_list_.push_back(g);
  // g->set...
}
void SSAModule::RemoveGlobalVariable(GlobalVariable* g) {
  glob_var_list_.remove(g);
  // TODO: manage memory
}

void SSAFunction::AddBasicBlock(SSABasicBlock* bb) {
  bb_list_.push_back(bb);
  bb->SetFunction(this);
}
void SSAFunction::RemoveBasicBlock(SSABasicBlock* bb) {
  bb_list_.remove(bb);
  // TODO: manage memory
  for (auto pred : bb->GetPredBB()) {
    pred->RemoveSuccBB(bb);
  }
  for (auto succ : bb->GetSuccBB()) {
    succ->RemovePredBB(bb);
  }
}

void SSABasicBlock::AddInstruction(SSAInstruction* inst) {
  inst_list_.push_back(inst);
  inst->SetParent(this);
}
void SSABasicBlock::RemoveInstruction(SSAInstruction* inst) {
  inst_list_.remove(inst);
  // TODO: manage memory and use used.
}

void SSAModule::EmitCode(std::ostream& out) {
  for (auto glob : glob_var_list_) {
    glob->Print();
  }
  out << std::endl;
  for (auto func : func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
}
void SSAFunction::EmitCode(std::ostream& out) {
  for (auto bb : bb_list_) {
    bb->EmitCode(out);
    out << std::endl;
  }
}
void SSABasicBlock::EmitCode(std::ostream& out) {
  for (auto inst : inst_list_) {
    inst->Print(out);
  }
}
