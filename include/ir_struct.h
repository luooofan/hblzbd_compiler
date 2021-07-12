#ifndef __IR_STRUCT_H__
#define __IR_STRUCT_H__
#include <iostream>
#include <string>

#include "../include/ir.h"

namespace ir {
class BasicBlock {
 public:
  // all irs
  int start_;  // close
  int end_;    // open
  // pred succ
  std::vector<BasicBlock*> pred_;
  std::vector<BasicBlock*> succ_;
  // in out data stream

  BasicBlock(int start) : start_(start), end_(-1) {}
  void Print();
};

class Function {
 public:
  // all BasicBlocks
  std::string func_name_;
  std::vector<BasicBlock*> bb_list_;

  // optional
  // Function*
  std::vector<Function*> call_func_list_;
  Function(std::string func_name) : func_name_(func_name) {}
  void Print();
};

class Module {
 public:
  // global declaration
  Scope& global_scope_;
  // all functions
  std::vector<Function*> func_list_;
  Module(Scope& global_scope) : global_scope_(global_scope) {}
  void Print();
};

Module* ConstructModule();

}  // namespace ir
#endif
