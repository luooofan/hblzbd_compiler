#include <iostream>

#include "pass_manager.h"

class InvariantExtrapolation : public Pass {
 public:
  int x;
  InvariantExtrapolation(Module** m) : Pass(m) {}
  void Run();
};