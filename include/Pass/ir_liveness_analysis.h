#ifndef __IR_LIVENESS_ANALYSIS__
#define __IR_LIVENESS_ANALYSIS__

#include "../ir_struct.h"
#include "pass_manager.h"
//
using namespace ir;
// using RegId = int;

std::pair<std::vector<Opn *>, std::vector<Opn *>> GetDefUsePtr(IR *ir, bool consider = true);
std::pair<std::vector<std::string>, std::vector<std::string>> GetDefUse(IR *ir, bool consider = true);

class IRLivenessAnalysis : public Analysis {
 public:
  IRLivenessAnalysis(Module **m) : Analysis(m) {}
  static void GetDefUse4Func(IRFunction *f, bool consider = true);
  // 对一个ir function做活跃变量分析
  static void Run4Func(IRFunction *f);
  // 对整个module做活跃变量分析
  void Run();
};

#endif