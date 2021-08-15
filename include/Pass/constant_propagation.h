#ifndef __CONSTANT_PROPAGATION_H__
#define __CONSTANT_PROPAGATION_H__

#include "./reach_define.h"
#include "./pass_manager.h"

class ConstantPropagation : public Transform{
public:
  ConstantPropagation(Module **m) : Transform(m){}

  void Run();
};

#endif