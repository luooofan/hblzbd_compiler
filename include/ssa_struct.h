#ifndef __SSA_STRUCT_H__
#define __SSA_STRUCT_H__
#include <iostream>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

#include "./general_struct.h"
class Value;
class SSAInstruction;
class GlobalVariable;
class SSABasicBlock;
class SSAFunction;
class FunctionValue;
class BasicBlockValue;

class SSAModule : public Module {
 private:
  std::list<GlobalVariable*> glob_var_list_;
  std::list<SSAFunction*> func_list_;

 public:
  // a symbol table
  std::string name_;
  SSAModule(std::string name = "") : Module(name), name_(name){};

  const std::list<SSAFunction*>& GetFuncList() const { return func_list_; }
  const std::list<GlobalVariable*>& GetGlobVarList() const { return glob_var_list_; }
  void AddFunction(SSAFunction* f);
  void RemoveFunction(SSAFunction* f);
  void AddGlobalVariable(GlobalVariable* g);
  void RemoveGlobalVariable(GlobalVariable* g);

  void EmitCode(std::ostream& out = std::cout);
};

class SSAFunction {
 private:
  std::string func_name_;
  SSAModule* m_ = nullptr;
  FunctionValue* value_ = nullptr;

  // need to maintain these data structure in every ssa-transform pass
  std::list<SSABasicBlock*> bb_list_;
  std::unordered_set<SSAFunction*> call_func_list_;         // may can be unorderd
  std::unordered_set<GlobalVariable*> used_glob_var_list_;  // may can be unorderd

 public:
  void BindValue(FunctionValue* value) { value_ = value; }
  FunctionValue* GetValue() const { return value_; }

  const std::string& GetFuncName() const { return func_name_; }
  SSAFunction(const std::string& func_name, SSAModule* m) : func_name_(func_name), m_(m) { m->AddFunction(this); }
  SSAFunction(const std::string& func_name) : func_name_(func_name) {}  // builtin function will not be added to module

  const std::list<SSABasicBlock*>& GetBBList() const { return bb_list_; }

  void AddCallFunc(SSAFunction* func);
  void AddCallFunc(FunctionValue* func_val);
  std::unordered_set<SSAFunction*>& GetCallFuncList() { return call_func_list_; }

  void AddUsedGlobVar(GlobalVariable* glob_var);
  std::unordered_set<GlobalVariable*>& GetUsedGlobVarList() { return used_glob_var_list_; }

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

  BasicBlockValue* value_ = nullptr;

 public:
  void BindValue(BasicBlockValue* value) { value_ = value; }
  BasicBlockValue* GetValue() const { return value_; }

  SSABasicBlock(const std::string& label, SSAFunction* func) : label_(label), func_(func) { func->AddBasicBlock(this); }
  SSABasicBlock(SSAFunction* func) : func_(func) { func->AddBasicBlock(this); }

  const std::list<SSAInstruction*>& GetInstList() const { return inst_list_; }

  std::string& GetLabel() { return label_; }
  void SetLabel(const std::string& label) { label_ = label; }

  void SetFunction(SSAFunction* func) { func_ = func; }
  SSAFunction* GetFunction() const { return func_; }

  std::list<SSAInstruction*>& GetInstructions() { return inst_list_; }
  void AddInstruction(SSAInstruction* inst);
  void RemoveInstruction(SSAInstruction* inst);

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