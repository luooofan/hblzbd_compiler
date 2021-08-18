#ifndef __INSTRUCTION_COMBINING__
#define __INSTRUCTION_COMBINING__

#include "../ir.h"
#include "../ir_struct.h"
#include "./pass_manager.h"

class Instruction_Combining : public Transform {
 public:
  Instruction_Combining(Module **m) : Transform(m) {}

  void Run();
};

#endif