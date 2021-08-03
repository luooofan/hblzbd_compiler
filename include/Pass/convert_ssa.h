#ifndef __CONVERT_SSA_H__
#define __CONVERT_SSA_H__

#include <algorithm>
#include <unordered_set>

#include "../ir_struct.h"
#include "../ssa.h"
#include "../ssa_struct.h"
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
  void ProcessGlobalVariable(IRModule* irm, SSAModule* ssam);
  void ProcessResValue(const std::string& comp_name, Opn* opn, Value* val, SSABasicBlock* ssabb);
  void GenerateSSABasicBlocks(IRFunction* func, SSAFunction* ssafunc,
                              std::unordered_map<IRBasicBlock*, SSABasicBlock*>& bb_map);
  SSAModule* ConstructSSA(IRModule* module);
  Value* ResolveOpn2Value(Opn* opn, SSABasicBlock* ssabb);
  Value* FindValueFromOpn(Opn* opn, bool is_global);
  Value* FindValueFromCompName(const std::string& comp_name, bool is_global);
  // 变量的唯一性是由变量名+作用域+SSA编号一起决定的 把三个整合为一个string 同一变量映射到同一value
  // 全局变量不加多余字符
  std::unordered_map<std::string, Value*> work_map;
  std::unordered_map<std::string, Value*> glob_map;  // 跨函数使用 全局量 包括函数
};

#endif