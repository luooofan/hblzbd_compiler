#ifndef __ARM_COND_BR_TO_INSTS_H__
#define __ARM_COND_BR_TO_INSTS_H__

#include "./pass_manager.h"

class Module;
class ArmFunction;
class CondBrToInsts : public Transform {
 public:
  CondBrToInsts(Module** m) : Transform(m) {}
  void Run() override;
  void Run4Func(ArmFunction* func);
};

#endif