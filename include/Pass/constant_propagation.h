#ifndef __CONSTANT_PROPAGATION_H__
#define __CONSTANT_PROPAGATION_H__

#include "./pass_manager.h"
#include "./reach_define.h"

class ConstantPropagation : public Transform {
 public:
  ConstantPropagation(Module **m) : Transform(m) {}

  void Run();
};

#endif