#include <iostream>

#include "pass_manager.h"

class MXD : public Pass {
 public:
  int x;
  MXD(Module** m) : Pass(m) {}
  void Run();
};