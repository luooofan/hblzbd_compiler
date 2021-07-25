#ifndef __IR_STRUCT_H__
#define __IR_STRUCT_H__
#include <iostream>
#include <string>

#include "../include/general_struct.h"
#include "../include/ir.h"
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

  std::vector<IRBasicBlock*> pred_;
  std::vector<IRBasicBlock*> succ_;

  IRBasicBlock() {}
  virtual ~IRBasicBlock() {}
  void EmitCode(std::ostream& out = std::cout);
};

IRModule* ConstructModule(const std::string& module_name);

#endif
