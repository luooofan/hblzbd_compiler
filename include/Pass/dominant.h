#include <unordered_set>

#include "../ir_struct.h"
#include "pass_manager.h"
using namespace ir;

class ComputeDominance : public Analysis {
 public:
  using BB = IRBasicBlock;
  ComputeDominance(Module** m) : Analysis(m) {}
  void ResetTempInfo(IRFunction* f);
  void ComputeIDomInfo(IRFunction* f);
  void ComputeDomFrontier(BB* bb);
  void Run();

 public:
  int N;
  std::vector<BB*> vertex;                // i->BB* 生成树编号为i的是哪个bb
  std::unordered_map<BB*, int> dfnum;     // BB*->treeid 第i个bb的深度优先生成树编号
  std::unordered_map<BB*, BB*> parent;    // BB*->BB* bb在生成树中的父bb
  std::unordered_map<BB*, BB*> ancestor;  // BB*->BB* 生成树森林
  std::unordered_map<BB*, BB*> best;      // best[a]=y represents a node y:(ancestor[a],a]:min(dfnum[semi[y]])
  std::unordered_map<BB*, BB*> semi;      // BB*->BB* bb的半必经结点bb
  // std::unordered_map<BB*, BB*> idom;      // BB*->BB* bb的直接必经结点bb
  std::unordered_map<BB*, BB*> samedom;                     // BB*->BB* 两个bb的idom一致
  std::unordered_map<BB*, std::unordered_set<BB*>> bucket;  // BB*->array(BB*) bb的直接必经结点bb
  void DFS(BB* p, BB* n);
  void Link(BB* p, BB* n);
  BB* AncestorWithLowestSemi(BB* v);
};