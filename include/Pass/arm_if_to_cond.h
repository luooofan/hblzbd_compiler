#ifndef __ARM_IF_TO_COND_H__
#define __ARM_IF_TO_COND_H__

#include "./pass_manager.h"

class Module;
class ArmFunction;
class IfToCond : public Transform {
 public:
  IfToCond(Module** m) : Transform(m) {}
  void Run() override;
  void Run4Func(ArmFunction* func);
};

#endif