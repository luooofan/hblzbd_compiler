// Reference: https://developer.arm.com/documentation/100076/0200
#ifndef __ARM_STRUCT_H__
#define __ARM_STRUCT_H__
#include <unordered_set>

#include "../include/arm.h"
#include "../include/ir_struct.h"
namespace arm {

class BasicBlock;
class Function;

class Module {
 public:
  // global declaration
  ir::Scope& global_scope_;
  // all functions
  std::vector<Function*> func_list_;
  Module(ir::Scope& global_scope) : global_scope_(global_scope) {}
  //   void Print();
  void EmitCode(std::ostream& outfile = std::clog);
};

class Function {
 public:
  // all BasicBlocks
  std::string func_name_;
  std::vector<BasicBlock*> bb_list_;
  int stack_size_;
  int arg_num_;
  int virtual_max = 16;  // rx x+1

  // optional
  // Function*
  std::vector<Function*> call_func_list_;
  Function(std::string func_name, int arg_num, int stack_size)
      : func_name_(func_name), arg_num_(arg_num), stack_size_(stack_size) {}
  //   void Print();
  bool IsLeaf() { return call_func_list_.empty(); }
  void EmitCode(std::ostream& outfile = std::clog);
};

class BasicBlock {
 public:
  // all instructions
  // int start_;  // close
  // int end_;    // open
  std::string* label_;
  std::vector<Instruction*> inst_list_;
  // pred succ
  std::vector<BasicBlock*> pred_;
  std::vector<BasicBlock*> succ_;
  // used for reg alloc
  std::unordered_set<int> def_;  // 基本块的def 可看作是def中去掉liveuse
  std::unordered_set<int> liveuse_;  // 基本块的use 可看作是活跃的use
  std::unordered_set<int> livein_;
  std::unordered_set<int> liveout_;

  // in out data stream
  BasicBlock() : label_(nullptr) {}
  BasicBlock(std::string* label) : label_(label) {}
  // BasicBlock(int start) : start_(start), end_(-1) {}
  //   void Print();
  bool HasLabel() { return nullptr != label_; }
  void EmitCode(std::ostream& outfile = std::clog);
};

Module* GenerateArm(ir::Module* module);

// extern std::vector<Instruction> gASMList;
}  // namespace arm
#endif