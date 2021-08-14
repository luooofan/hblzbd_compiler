#ifndef __GENERAL_STRUCT_H__
#define __GENERAL_STRUCT_H__

// define general moudle, function and basicblock
#include <iostream>
#include <string>
#include <vector>

// TODO: not use ir::Scope.
#include "../include/ir.h"  // global scope

class Module {
 public:
  // module name. actually file name.
  std::string name_;
  // global declaration
  // ir::Scope& global_scope_;

 public:
  // Module(const std::string& name, ir::Scope& global_scope) : name_(name), global_scope_(global_scope) {}
  // Module(ir::Scope& global_scope) : name_("module"), global_scope_(global_scope) {}
  Module(const std::string& name) : name_(name) {}
  Module() : name_("module") {}
  virtual ~Module() {}
  virtual void EmitCode(std::ostream& out = std::cout) = 0;
};

#endif