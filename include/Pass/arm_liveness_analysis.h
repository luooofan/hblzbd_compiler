#include "../arm_struct.h"
#include "pass_manager.h"
//
using namespace arm;
using RegId = int;

std::pair<std::vector<RegId>, std::vector<RegId>> GetDefUse(Instruction *inst);
std::pair<std::vector<Reg *>, std::vector<Reg *>> GetDefUsePtr(Instruction *inst);

class ArmLivenessAnalysis : public Analysis {
 public:
  ArmLivenessAnalysis(Module **m) : Analysis(m) {}
  // 对一个arm function做活跃变量分析
  static void Run4Func(ArmFunction *f);
  // 对整个module做活跃变量分析
  void Run();
};