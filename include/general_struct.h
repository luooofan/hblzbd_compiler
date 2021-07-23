#ifndef __GENERAL_STRUCT_H__
#define __GENERAL_STRUCT_H__

// define general moudle, function and basicblock
#include <iostream>
#include <string>
#include <vector>

// TODO: not use ir::Scope.
#include "../include/ir.h"  // global scope

class BasicBlock;
class Function;
class Module {
 public:
  // module name. actually file name.
  std::string name_;
  // global declaration
  ir::Scope& global_scope_;

 public:
  Module(const std::string& name, ir::Scope& global_scope) : name_(name), global_scope_(global_scope) {}
  Module(ir::Scope& global_scope) : name_("module"), global_scope_(global_scope) {}
  virtual ~Module() {}
  virtual void EmitCode(std::ostream& out = std::cout) = 0;
};

class Function {
 public:
  // function name
  std::string name_;
  int arg_num_;
  int stack_size_;

 public:
  Function(const std::string& name, int arg_num, int stack_size)
      : name_(name), arg_num_(arg_num), stack_size_(stack_size) {}
  virtual ~Function() {}
  virtual void EmitCode(std::ostream& out = std::cout) = 0;
};

class BasicBlock {
 public:
  BasicBlock() {}
  virtual ~BasicBlock() {}
  virtual void EmitCode(std::ostream& out = std::cout) = 0;
};

#endif