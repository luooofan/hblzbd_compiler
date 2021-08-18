#include "./pass_manager.h"
class ArmFunction;

class SPOffsetFixup : public Transform {
 public:
  SPOffsetFixup(Module** m) : Transform(m) {}
  void Run() override;

 private:
  void Fixup4Func(ArmFunction* func);
};