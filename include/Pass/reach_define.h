#ifndef __REACH_DEFINE__
#define __REACH_DEFINE__

#include "../ir.h"
#include "../ir_struct.h"
#include "pass_manager.h"

class ReachDefine : public Analysis {
 public:
  ReachDefine(Module **m) : Analysis(m) {}

  void Run();
  void Run4Func(IRFunction *f);
  void GetGenKill4Func(IRFunction *f);
  std::pair<std::unordered_set<ir::IR *>, std::unordered_set<ir::IR *>> GetGenKill(ir::IR *ir, IRFunction *func);
  void GetUseDefChain(IRModule *m);
};

#endif