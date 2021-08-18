#ifndef __SIMPLIFY_CFG_H__
#define __SIMPLIFY_CFG_H__

#include "../ir_struct.h"
#include "../ssa_struct.h"
#include "pass_manager.h"

class SimplifyCFG : public Transform {
 public:
  SimplifyCFG(Module** m) : Transform(m) {}
  void Run() override;
  void Run4IRFunc(IRFunction* func);
  void Run4SSAFunc(SSAFunction* func);
};
#endif