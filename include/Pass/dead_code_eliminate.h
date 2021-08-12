#ifndef __DEAD_CODE_ELIMINATE_H__
#define __DEAD_CODE_ELIMINATE_H__
#include <unordered_set>

#include "./pass_manager.h"
class SSAModule;
class SSAFunction;
class SSAInstruction;
class Value;
// DeadCodeEliminat: one of SSA-Pass
class DeadCodeEliminate : public Transform {
 public:
  DeadCodeEliminate(Module** m) : Transform(m) {}
  void Run() override;
  static void RemoveFromNoSideEffectFuncs(SSAFunction* func, std::unordered_set<SSAFunction*>& no_side_effect_funcs);
  static void FindNoSideEffectFunc(SSAModule* m, std::unordered_set<SSAFunction*>& no_side_effect_funcs);

 private:
  std::unordered_set<SSAFunction*> no_side_effect_funcs;

  bool IsSideEffect(Value* val);
  bool IsSideEffect(SSAInstruction* inst);
  void DeleteDeadFunc(SSAModule* m);
  void DeleteDeadInst(SSAFunction* func);
  // void FindNoSideEffectFunc(SSAModule* m);
};

#endif