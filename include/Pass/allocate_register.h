#include <algorithm>
#include <cassert>
#include <map>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include "../arm_struct.h"
#include "pass_manager.h"
class RegAlloc : public Transform {
 public:
  RegAlloc(Module **m) : Transform(m) {}

  using RegId = int;
  // 工作表 一个工作表是一些寄存器RegId的集合
  using WorkList = std::unordered_set<RegId>;
  // using RegSet = ...
  // 邻接表
  using AdjList = std::unordered_map<RegId, std::unordered_set<RegId>>;
  // using Reg2AdjList = ...
  // 冲突边的偶数对集
  using AdjSet = std::set<std::pair<RegId, RegId>>;
  // 结点的度数
  using Degree = std::unordered_map<RegId, int>;

  // mov指令集
  using MovSet = std::unordered_set<arm::Move *>;
  // 一个reg相关的mov指令
  using MovList = std::unordered_map<RegId, MovSet>;
  // using Reg2MovList = ...
  WorkList ValidAdjacentSet(RegId reg, AdjList &adj_list,
                            std::vector<RegId> &select_stack,
                            WorkList &coalesced_nodes);
  void AllocateRegister(ArmModule *m, std::ostream &outfile = std::clog);
  void Run();
};
