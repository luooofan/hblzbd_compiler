#ifndef __INSTRUCTION_COMBINING__
#define __INSTRUCTION_COMBINING__

#include "./pass_manager.h"
#include "../ir_struct.h"
#include "../ir.h"

class Instruction_Combining : public Transform{
public:
  Instruction_Combining(Module **m) : Transform(m) {}

  void Run();
};

#endif