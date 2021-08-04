#include "../include/ssa_struct.h"

#include "../include/ssa.h"
#define ASSERT_ENABLE
#include "../include/myassert.h"

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
    glob->Print(out);
  }
  out << std::endl;
  for (auto func : func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
}

void SSAFunction::AddCallFunc(SSAFunction* func) {
  MyAssert(nullptr != func);
  call_func_list_.insert(func);
}
void SSAFunction::AddCallFunc(FunctionValue* func_val) {
  MyAssert(nullptr != func_val->GetFunction());
  call_func_list_.insert(func_val->GetFunction());
}
void SSAFunction::AddUsedGlobVar(GlobalVariable* glob_var) {
  MyAssert(nullptr != glob_var);
  used_glob_var_list_.insert(glob_var);
}
void SSAFunction::EmitCode(std::ostream& out) {
  out << GetFuncName() << ":" << std::endl;
  out << "; Function Begin:" << std::endl;
  out << "  ; call func set: ";
  for (auto call_func : call_func_list_) {
    out << call_func->GetFuncName() << " ";
  }
  out << std::endl;
  out << "  ; used global var set: ";
  for (auto glob_var : used_glob_var_list_) {
    out << glob_var->GetName() << " ";
  }
  out << std::endl;
  for (auto bb : bb_list_) {
    bb->EmitCode(out);
    out << std::endl;
  }
  out << "; Function End." << std::endl;
}

void SSABasicBlock::EmitCode(std::ostream& out) {
  out << GetLabel() << ":" << std::endl;
  out << "; BB Begin:" << std::endl;
  for (auto inst : inst_list_) {
    inst->Print(out);
  }
  out << "; BB End." << std::endl;
}

#undef ASSERT_ENABLE