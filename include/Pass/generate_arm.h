#include "../arm.h"
#include "../arm_struct.h"
#include "pass_manager.h"

class GenerateArm : public Transform {
 public:
  GenerateArm(Module** m) : Transform(m) {}
  ArmModule* GenCode(IRModule* module);
  // will change *m: IRModule -> ArmModule
  // remember release source *m space
  void Run();
};