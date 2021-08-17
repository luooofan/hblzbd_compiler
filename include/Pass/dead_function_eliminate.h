#ifndef __DEAD_FUNCTION_ELIMINATE__
#define __DEAD_FUNCTION_ELIMINATE__

#include "./pass_manager.h"
#include "../ir_struct.h"
#include "../ir.h"

class DeadFunctionEliminate : public Transform{
public:
  DeadFunctionEliminate(Module **m) : Transform(m) { }

  void Run();
};

#endif