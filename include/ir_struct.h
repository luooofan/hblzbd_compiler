#ifndef __IR_STRUCT_H__
#define __IR_STRUCT_H__
#include <iostream>
#include <string>
#include <unordered_set>

#include "./general_struct.h"
#include "./ir.h"
class IRBasicBlock;
class IRFunction;

class IRModule : public Module {
 public:
  // functions: ordered
  std::vector<IRFunction*> func_list_;
  ir::Scope& global_scope_;

 public:
  IRModule(const std::string& name, ir::Scope& global_scope) : Module(name), global_scope_(global_scope) {}
  IRModule(ir::Scope& global_scope) : global_scope_(global_scope) {}
  virtual ~IRModule();
  void EmitCode(std::ostream& out = std::cout);
};

class IRFunction {
 public:
  std::string name_;
  int arg_num_;
  // basicblocks: ordered
  std::vector<IRBasicBlock*> bb_list_;
  std::vector<IRFunction*> call_func_list_;

  // 用于到达定值分析
  std::unordered_map<std::string, std::unordered_set<ir::IR*>> def_pre_var_;

 public:
  IRFunction(const std::string& name, int arg_num) : name_(name), arg_num_(arg_num) {}
  virtual ~IRFunction();
  bool IsLeaf() { return call_func_list_.empty(); }
  void EmitCode(std::ostream& out = std::cout);
};

class IRBasicBlock {
 public:
  // all irs
  std::vector<ir::IR*> ir_list_;
  // only used for emitting
  IRFunction* func_;

  std::vector<IRBasicBlock*> pred_;
  std::vector<IRBasicBlock*> succ_;

  std::unordered_set<std::string> def_;
  std::unordered_set<std::string> use_;
  std::unordered_set<std::string> livein_;
  std::unordered_set<std::string> liveout_;

  // 用于到达定值分析
  std::unordered_set<ir::IR*> gen_;
  std::unordered_set<ir::IR*> kill_;
  std::unordered_set<ir::IR*> reach_in_;
  std::unordered_set<ir::IR*> reach_out_;
  std::vector<std::unordered_map<ir::Opn*, std::unordered_set<ir::IR*>>> use_def_chains_;

  //记录下这个基本块的Label，在基本块中就不处理label语句了
  //形式：Label_#label or Label_#Func
  std::string bb_label_;

  // used for ssa
  IRBasicBlock* idom_ = nullptr;          // 直接支配结点 即支配结点树中的父节点
  std::vector<IRBasicBlock*> doms_;       // 所直接支配的结点 即支配结点树中的子节点
  std::unordered_set<IRBasicBlock*> df_;  // 支配节点边界
  // std::unordered_set<IRBasicBlock*> dom_by_;
  // int dom_level_;
  bool IsByDom(IRBasicBlock* bb);  // 该bb是不是被这个参数bb所支配 或者说参数bb是不是该bb的必经结点

  IRBasicBlock() : func_(nullptr) {}
  IRBasicBlock(IRFunction* func) : func_(func) {}
  virtual ~IRBasicBlock();
  void EmitCode(std::ostream& out = std::cout);
  void EmitBackupCode(std::ostream& out = std::cout);
  int IndexInFunc();
};

IRModule* ConstructModule(const std::string& module_name, std::vector<ir::IR*>& gIRList);

#endif
