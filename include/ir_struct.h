#ifndef __IR_STRUCT_H__
#define __IR_STRUCT_H__
#include <iostream>
#include <string>
#include <unordered_set>

#include "../include/general_struct.h"
#include "../include/ir.h"
#include "DAG.h"
class IRBasicBlock;
class IRFunction;

class IRModule : public Module {
 public:
  // functions: ordered
  std::vector<IRFunction*> func_list_;

 public:
  IRModule(const std::string& name, ir::Scope& global_scope) : Module(name, global_scope) {}
  IRModule(ir::Scope& global_scope) : Module(global_scope) {}
  virtual ~IRModule() {}
  void EmitCode(std::ostream& out = std::cout);
};

class IRFunction : public Function {
 public:
  // basicblocks: ordered
  std::vector<IRBasicBlock*> bb_list_;
  std::vector<IRFunction*> call_func_list_;

 public:
  IRFunction(const std::string& name, int arg_num, int stack_size) : Function(name, arg_num, stack_size) {}
  virtual ~IRFunction() {}
  bool IsLeaf() { return call_func_list_.empty(); }
  void EmitCode(std::ostream& out = std::cout);
};

class IRBasicBlock : public BasicBlock {
 public:
  // all irs
  std::vector<ir::IR*> ir_list_;
  // only used for emitting
  IRFunction* func_;

  std::vector<IRBasicBlock*> pred_;
  std::vector<IRBasicBlock*> succ_;

  std::unordered_set<ir::Opn*> def_;
  std::unordered_set<ir::Opn*> use_;
  std::unordered_set<ir::Opn*> livein_;
  std::unordered_set<ir::Opn*> liveout_;

  std::vector<DAG_node*> node_list_;

  // used for ssa
  IRBasicBlock* idom_ = nullptr;          // 直接支配结点 即支配结点树中的父节点
  std::vector<IRBasicBlock*> doms_;       // 所直接支配的结点 即支配结点树中的子节点
  std::unordered_set<IRBasicBlock*> df_;  // 支配节点边界
  // std::unordered_set<IRBasicBlock*> dom_by_;
  // int dom_level_;
  bool IsByDom(IRBasicBlock* bb);  // 该bb是不是被这个参数bb所支配 或者说参数bb是不是该bb的必经结点

  IRBasicBlock() : func_(nullptr) {}
  IRBasicBlock(IRFunction* func) : func_(func) {}
  virtual ~IRBasicBlock() {}
  void EmitCode(std::ostream& out = std::cout);
  int IndexInFunc();
};

IRModule* ConstructModule(const std::string& module_name);

#endif
