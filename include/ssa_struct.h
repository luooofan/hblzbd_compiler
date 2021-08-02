#ifndef __SSA_STRUCT_H__
#define __SSA_STRUCT_H__
#include <iostream>
#include <list>
#include <string>
#include <vector>
class Value;
class SSAInstruction;
class GlobalVariable;
class SSABasicBlock;
class SSAFunction;
class FunctionValue;
class BasicBlockValue;

class SSAModule {
 private:
  std::list<GlobalVariable*> glob_var_list_;
  std::list<SSAFunction*> func_list_;

 public:
  // list of funcs
  // list of globalvars
  // a symbol table
  // a few helpful member funcs that try to make common operations easy.
  std::string name_;
  SSAModule(std::string name = "") : name_(name){};
  void AddFunction(SSAFunction* f);
  void RemoveFunction(SSAFunction* f);
  void AddGlobalVariable(GlobalVariable* g);
  void RemoveGlobalVariable(GlobalVariable* g);

  void EmitCode(std::ostream& out = std::cout);
};

class SSAFunction {
 private:
  SSAModule* m_;
  std::list<SSABasicBlock*> bb_list_;

  FunctionValue* value_;

 public:
  void BindValue(FunctionValue* value) { value_ = value; }
  FunctionValue* GetValue() const { return value_; }

  std::string func_name_;
  SSAFunction(const std::string& func_name, SSAModule* m) : func_name_(func_name), m_(m) { m->AddFunction(this); }

  void SetModule(SSAModule* m) { m_ = m; }
  SSAModule* GetModule() const { return m_; }

  std::list<SSABasicBlock*>& GetBasicBlocks() { return bb_list_; }
  void AddBasicBlock(SSABasicBlock* bb);
  void RemoveBasicBlock(SSABasicBlock* bb);

  void EmitCode(std::ostream& out = std::cout);
};

class SSABasicBlock {
 private:
  SSAFunction* func_;
  std::string label_;

  std::list<SSAInstruction*> inst_list_;
  std::list<SSABasicBlock*> pred_;
  std::list<SSABasicBlock*> succ_;

  BasicBlockValue* value_;

 public:
  void BindValue(BasicBlockValue* value) { value_ = value; }
  BasicBlockValue* GetValue() const { return value_; }

  SSABasicBlock(const std::string& label, SSAFunction* func) : label_(label), func_(func) { func->AddBasicBlock(this); }
  SSABasicBlock(SSAFunction* func) : func_(func) { func->AddBasicBlock(this); }

  std::string& GetLabel() { return label_; }
  void SetLabel(const std::string& label) { label_ = label; }

  void SetFunction(SSAFunction* func) { func_ = func; }
  SSAFunction* GetFunction() const { return func_; }

  // interface to man insts
  std::list<SSAInstruction*>& GetInstructions() { return inst_list_; }
  void AddInstruction(SSAInstruction* inst);
  void RemoveInstruction(SSAInstruction* inst);

  // interface to man pred and succ. dont't auto .. this's pred/succ and pred/succ bb's succ/pred
  std::list<SSABasicBlock*>& GetPredBB() { return pred_; }
  std::list<SSABasicBlock*>& GetSuccBB() { return succ_; }
  void AddPredBB(SSABasicBlock* pred) { pred_.push_back(pred); }
  void AddSuccBB(SSABasicBlock* succ) { succ_.push_back(succ); }
  void RemovePredBB(SSABasicBlock* pred) {
    pred_.remove(pred);  // only remove relationship
  }
  void RemoveSuccBB(SSABasicBlock* succ) {
    succ_.remove(succ);  // only remove relationship
  }

  void EmitCode(std::ostream& out = std::cout);
};

#endif