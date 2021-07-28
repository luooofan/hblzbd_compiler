#include "../ir_struct.h"
#include "pass_manager.h"
//
using namespace ir;
// using RegId = int;

std::pair<std::vector<Opn*>, std::vector<Opn*>> GetDefUse(IR *ir);
// std::pair<std::vector<Reg *>, std::vector<Reg *>> GetDefUsePtr(Instruction *inst);

class IRLivenessAnalysis : public Analysis {
 public:
  IRLivenessAnalysis(Module **m) : Analysis(m) {}
  // 对一个arm function做活跃变量分析
  static void Run4Func(IRFunction *f);
  // 对整个module做活跃变量分析
  void Run();
};