#include "../arm_struct.h"
#include "pass_manager.h"

class SimplifyArm : public Transform {
 public:
  SimplifyArm(Module **m) : Transform(m) {}
  void Run();
};