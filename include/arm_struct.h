// Reference: https://developer.arm.com/documentation/100076/0200
#ifndef __ARM_STRUCT_H__
#define __ARM_STRUCT_H__
#include <unordered_set>

#include "../include/arm.h"
#include "../include/general_struct.h"
#include "../include/ir.h"
using namespace arm;
class ArmBasicBlock;
class ArmFunction;

class ArmModule : public Module {
 public:
  // functions: ordered
  std::vector<ArmFunction*> func_list_;

 public:
  ArmModule(const std::string& name, ir::Scope& global_scope) : Module(name, global_scope) {}
  ArmModule(ir::Scope& global_scope) : Module(global_scope) {}
  virtual ~ArmModule() {}
  void EmitCode(std::ostream& out = std::cout);
  void Check();
};

class ArmFunction : public Function {
 public:
  int virtual_reg_max = 16;  // rx x+1
  // basicblocks: ordered
  std::vector<ArmBasicBlock*> bb_list_;
  std::vector<ArmFunction*> call_func_list_;
  std::vector<Instruction*> sp_arg_fixup_;  // a ldr-pseudo inst
  std::vector<Instruction*> sp_fixup_;

  ArmFunction(const std::string& name, int arg_num, int stack_size) : Function(name, arg_num, stack_size) {}
  virtual ~ArmFunction() {}
  bool IsLeaf() { return call_func_list_.empty(); }
  void EmitCode(std::ostream& out = std::cout);
  void Check();
};

class ArmBasicBlock : public BasicBlock {
 public:
  std::string* label_;
  // only used for emitting
  ArmFunction* func_;

  // all instructions
  std::vector<arm::Instruction*> inst_list_;

  std::vector<ArmBasicBlock*> pred_;
  std::vector<ArmBasicBlock*> succ_;

  // used for reg alloc
  std::unordered_set<int> def_;  // 基本块的def 可看作是def中去掉liveuse
  std::unordered_set<int> use_;  // 基本块的use 可看作是活跃的use
  std::unordered_set<int> livein_;
  std::unordered_set<int> liveout_;

  ArmBasicBlock() : label_(nullptr) {}
  ArmBasicBlock(std::string* label) : label_(label) {}
  virtual ~ArmBasicBlock() {}

  bool HasLabel() { return nullptr != label_; }
  void EmitCode(std::ostream& out = std::cout);
  void Check();
  int IndexInFunc();
};

#endif