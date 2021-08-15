#ifndef __SIMPLE_OPTIMIZE_H__
#define __SIMPLE_OPTIMIZE_H__
#include "./pass_manager.h"

class Module;
class SimpleOptimize : public Transform {
 public:
  SimpleOptimize(Module** m) : Transform(m) {}
  void Run() override;
};

#endif