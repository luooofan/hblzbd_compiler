#define ASSERT_ENABLE  // enable assert for this file. place this at the top of a file.
#include "../../include/Pass/dominant.h"

#include "../../include/myassert.h"

void ComputeDominance::DFS(BB* p, BB* n) {
  if (dfnum.find(n) != dfnum.end()) return;
  dfnum.insert({n, N});
  vertex[N] = n;
  parent.insert({n, p});  // NOTE: parent[root]=nullptr
  ++N;
  for (auto succ : n->succ_) {
    DFS(n, succ);
  }
}

void ComputeDominance::Link(BB* p, BB* n) {
  ancestor.insert({n, p});  // 此时此刻的生成树森林状态为 dfnum比n大的都在
  best.insert({n, n});
}

ComputeDominance::BB* ComputeDominance::AncestorWithLowestSemi(ComputeDominance::BB* v) {
  MyAssert(ancestor.find(v) != ancestor.end());
  // 从a到v(included)这段已经算过了 存在best[v]里 compute min(dfnum[semi[y]])
  auto a = ancestor[v];
  MyAssert(nullptr != a);  // 不会出现a为空的情况 根节点不会被链入到ancestor中
  if (ancestor.find(a) != ancestor.end()) {
    // 把当前生成树森林中v的最小的祖先到a(included)这段也算了 存在best[a]里 即b
    auto b = AncestorWithLowestSemi(a);
    // 更新 使更新后从ancestor[a]到v(included)这段都算过 存在best[v]里
    ancestor[v] = ancestor[a];
    if (dfnum[semi[b]] < dfnum[semi[best[v]]]) {
      best[v] = b;
    }
  }
  return best[v];
}

void ComputeDominance::ResetTempInfo(IRFunction* f) {
  // 初始化数据 9
  vertex.clear();
  dfnum.clear();
  parent.clear();
  semi.clear();
  // idom.clear();
  bucket.clear();
  ancestor.clear();
  samedom.clear();
  best.clear();
  vertex.resize(f->bb_list_.size());
  N = 0;
}

// Lengauer-Tarjan算法 ref: https://dl.acm.org/doi/10.1145/357062.357071
// 填写f中每个bb的idom和doms信息
void ComputeDominance::ComputeIDomInfo(IRFunction* f) {
  // 构造深度优先生成树 构造完后 保证vertex dfnum有效完整 parent[r]为空
  DFS(nullptr, f->bb_list_.front());
  // std::cout << N << " " << f->bb_list_.size() << std::endl;
  MyAssert(N == f->bb_list_.size());

  // 基于半必经结点定理计算所有 非根节点 的半必经结点
  for (int i = N - 1; i > 0; --i) {
    BB* n = vertex[i];
    auto p = parent[n];
    MyAssert(nullptr != p);
    auto s = p;  // s即n的半必经结点
    for (auto pred : n->pred_) {
      BB* s_temp = nullptr;
      if (dfnum[pred] <= dfnum[n]) {  // 该前驱在生成树上是n的祖先
        s_temp = pred;                // 则该结点是semi[n]的候选
      } else {                        // 该前驱在生成树上不是n的祖先
        s_temp = semi[AncestorWithLowestSemi(pred)];
        // 则该前驱到其公共祖先的路径上的y:min(dfnum[semi[y]]) semi[y]成为semi[n]候选
      }
      if (dfnum[s_temp] < dfnum[s]) {  // 半必经结点是dfnum最小的
        s = s_temp;
      }
    }
    semi.insert({n, s});  // semi和bucket和ancestor中不会有key为root的pair
    bucket[s].insert(n);
    Link(p, n);  // 把p->n链入到生成树森林

    // 计算 以p为半必经结点的 结点v 的 必经结点 这里可以保证会把semi为root结点的结点的idom都计算到
    for (auto v : bucket[p]) {
      BB* y = AncestorWithLowestSemi(v);  // min(dfnum[semi[y]]) y: from p to v(included). dfnum[v]>dfnum[y].
      if (semi[y] == p /*semi[v]*/) {     // 没有旁路p的分支
        // idom.insert({v, p});
        v->idom_ = p;
        p->doms_.push_back(v);
      } else {
        samedom.insert({v, y});  // fill idom[samedom.key]=idom[samedom.value] later.
      }
    }
    bucket[p].clear();
  }

  // fill idom[samedom.key]=idom[samedom.value]
  for (int i = 1; i < N; ++i) {
    auto n = vertex[i];
    if (samedom.find(n) != samedom.end()) {
      auto idom = samedom[n]->idom_;
      // MyAssert(idom.find(samedom[n]) != idom.end());
      // idom.insert({n, idom[samedom[n]]});
      MyAssert(nullptr != idom);
      n->idom_ = idom;
      idom->doms_.push_back(n);
    }
  }
}

// 填写该结点和该结点的支配结点的df信息
void ComputeDominance::ComputeDomFrontier(ComputeDominance::BB* bb) {
  for (auto succ : bb->succ_) {
    if (succ->idom_ != bb) {  // bb一定不是该succ的必经结点
      bb->df_.insert(succ);
    }
  }
  for (auto child : bb->doms_) {
    ComputeDomFrontier(child);
    for (auto child_df : child->df_) {
      if (!child_df->IsByDom(bb) || child_df == bb) {  // bb不是child_df的严格必经结点
        bb->df_.insert(child_df);
      }
    }
  }
}

void ComputeDominance::Run() {
  auto m = dynamic_cast<IRModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->func_list_) {
    ResetTempInfo(func);
    ComputeIDomInfo(func);                       // 填写func中每个bb的idom和doms信息
    ComputeDomFrontier(func->bb_list_.front());  // 填写func中每个bb的df信息
  }
}

#undef ASSERT_ENABLE  // disable assert. this should be placed at the end of every file.