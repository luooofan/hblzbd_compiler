#ifndef __SSA_H__
#define __SSA_H__

#include <algorithm>
#include <unordered_set>

#include "../ir_struct.h"
#include "pass_manager.h"
using namespace ir;

class ConvertSSA : public Transform {
 public:
  ConvertSSA(Module** m) : Transform(m) {}
  void Run() override;

 private:
  // string用来标识一个变量 form: varname#scope_id
  std::unordered_map<std::string, int> count_;              // 变量的ssa id计数
  std::unordered_map<std::string, std::stack<int>> stack_;  // 变量的最近定值栈

 private:
  void Rename(IRBasicBlock* bb);
  void InsertPhiIR(IRFunction* f);
};

#endif