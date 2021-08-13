#include "../include/ssa_struct.h"

#include <algorithm>

#include "../include/ssa.h"
#define ASSERT_ENABLE
#include "../include/myassert.h"

void SSAModule::AddFunction(SSAFunction* f) {
  func_list_.push_back(f);
  f->SetModule(this);
}
void SSAModule::AddBuiltinFunction(SSAFunction* f) {
  builtin_func_list_.push_back(f);
  // f->SetModule(this);
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
SSAModule::~SSAModule() {
  // glob_var_list_.clear();
  for (auto func : func_list_) delete func;
  // func_list_.clear();
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
void SSAFunction::MaintainCallFunc() {
  call_func_list_.clear();
  for (auto bb : bb_list_) {
    for (auto inst : bb->GetInstList()) {
      if (auto call_inst = dynamic_cast<CallInst*>(inst)) {
        auto func_val = dynamic_cast<FunctionValue*>(call_inst->GetOperand(0));
        MyAssert(nullptr != func_val);
        call_func_list_.insert(func_val->GetFunction());
      }
    }
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
bool SSAFunction::IsCalled() { return value_->IsUsed(); }
void SSAFunction::Remove() {
  // 要删除该函数 必须保证module内没有调用该函数的语句
  MyAssert(GetValue()->GetUses().empty());
  // 首先需要移除该函数在其调用的函数和使用的全局量中的使用信息 然后从module中删除它
  // 该函数调用的函数对应的value的uselist中存着该函数对应的value 是在生成call语句的时候加进去的
  for (auto call_func : call_func_list_) {
    std::cout << call_func->GetFuncName() << std::endl;
    call_func->GetValue()->KillUse(this);
  }
  // 该函数使用的globalvariable的uselist中存着该函数中所有对该globvar的load和store
  for (auto glob_var : used_glob_var_list_) {
    glob_var->KillUse(this);
  }
  m_->RemoveFunction(this);
  // FIXME: 在该函数中清空信息还是放到析构函数中清空???
}
SSAFunction::~SSAFunction() {
  // m_ = nullptr;
  // delete value_; // FIXME: should be delete and remove all use
  for (auto bb : bb_list_) delete bb;
  // bb_list_.clear();
  // call_func_list_.clear();
  // used_glob_var_list_.clear();
}

SSABasicBlock* SSABasicBlock::GetNextBB() {
  // maybe return nullptr
  auto& bb_list = func_->GetBBList();
  auto it = ++std::find(bb_list.begin(), bb_list.end(), this);
  if (it == bb_list.end())
    return nullptr;
  else
    return *it;
}
void SSABasicBlock::AddInstruction(SSAInstruction* inst) {
  inst_list_.push_back(inst);
  inst->SetParent(this);
}
void SSABasicBlock::AddInstruction(SSAInstruction* inst, SSAInstruction* insert_before) {
  // NOTE HERE
  inst_list_.insert(std::find(inst_list_.begin(), inst_list_.end(), insert_before), inst);
  inst->SetParent(this);
}
void SSABasicBlock::RemoveInstruction(SSAInstruction* inst) {
  inst_list_.remove(inst);
  // TODO: manage memory and use used.
}
SSABasicBlock::~SSABasicBlock() {
  // func_ = nullptr;
  // delete value_; // FIXME: should be delete and remove all use
  for (auto inst : inst_list_) {
    delete inst;
    // std::cout << "ok\n";
  }
  // inst_list_.clear();
  // pred_.clear();
  // succ_.clear();
}

void SSAModule::EmitCode(std::ostream& out) {
  for (auto glob : glob_var_list_) {
    glob->Print(out);
  }
  out << std::endl;
  for (auto func : builtin_func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
  for (auto func : func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
}

void SSAFunction::EmitCode(std::ostream& out) {
  out << func_name_ << ":" << std::endl;
  out << "; Function Begin:" << std::endl;
  out << "  ; called by func: ";
  for (auto called_func_use : value_->GetUses()) {
    // called_func_use->Get()->Print();
    auto inst = dynamic_cast<SSAInstruction*>(called_func_use->GetUser());
    MyAssert(nullptr != inst);
    out << inst->GetParent()->GetFunction()->GetFuncName() << " ";
  }
  out << std::endl;
  out << "  ; call func: ";
  for (auto call_func : call_func_list_) {
    out << call_func->GetFuncName() << " ";
  }
  out << std::endl;
  out << "  ; used global var: ";
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
  out << label_ << ":" << std::endl;
  out << "; BB Begin:" << std::endl;
  for (auto inst : inst_list_) {
    inst->Print(out);
  }
  out << "; BB End." << std::endl;
}

#undef ASSERT_ENABLE