#include "../../include/Pass/allocate_register.h"
// GetUseDef GetUseDefPtr
#include "../../include/Pass/arm_liveness_analysis.h"
using namespace arm;

#define IS_PRECOLORED(i) (i < 16)

void dbg_print_worklist(RegAlloc::WorkList &wl, std::ostream &outfile) {
  for (auto item : wl) {
    outfile << item << " ";
  }
  outfile << std::endl;
}
// 利用adj_list返回一个结点的所有有效邻接点 无效邻接点为选择栈中的结点和已合并的结点
// called Adjacent in paper.
RegAlloc::WorkList RegAlloc::ValidAdjacentSet(RegId reg, AdjList &adj_list, std::vector<RegId> &select_stack,
                                              WorkList &coalesced_nodes) {
  WorkList valid_adjacent = adj_list[reg];
  // outfile << valid_adjacent.size() << std::endl;
  for (auto iter = valid_adjacent.begin(); iter != valid_adjacent.end();) {
    // outfile << *iter << std::endl;
    // NOTE: erase
    if (std::find(select_stack.begin(), select_stack.end(), *iter) == select_stack.end() &&
        coalesced_nodes.find(*iter) == coalesced_nodes.end()) {
      ++iter;
    } else {
      iter = valid_adjacent.erase(iter);
    }
    // dbg_print_worklist(valid_adjacent, outfile);
  }
  return valid_adjacent;
};

void RegAlloc::Run() {
  auto m = dynamic_cast<ArmModule *>(*(this->m_));
  assert(nullptr != m);
  this->AllocateRegister(m);
}

// iterated register coalescing
void RegAlloc::AllocateRegister(ArmModule *m, std::ostream &outfile) {
  int i = 0;
  for (auto func : m->func_list_) {
    // outfile << "第" << i++ << "个函数: " << func->name_ << std::endl;
    std::set<RegId> used_callee_saved_regs;
    int src_stack_size = func->stack_size_;
    bool done = false;
    while (!done) {
      used_callee_saved_regs.clear();
      // K color
      const int K = 14;  // r0-r11 r12 lr(r14)

      // 冲突图
      AdjList adj_list;
      AdjSet adj_set;
      Degree degree;

      WorkList simplify_worklist;  // 低度数的mov无关的结点集合 可简化
      WorkList freeze_worklist;    // 低度数的mov有关的结点集合 可冻结
      WorkList spill_worklist;     // 高度数的结点结合 可能溢出
      // 需要在选择栈中查找元素 所以用vector
      std::vector<RegId> select_stack;

      WorkList spilled_nodes;
      WorkList coalesced_nodes;
      WorkList colored_nodes;
      // v合并到u alias[v]=u
      // TODO: 并查集
      std::unordered_map<RegId, RegId> alias;

      // Mov相关
      MovList move_list;

      MovSet coalesced_moves;
      MovSet constrained_moves;
      MovSet frozen_moves;

      // (现在或将来)可用于合并的mov指令集合
      MovSet worklist_moves;  // 一开始mov指令都放在这里
      MovSet active_moves;    // 里面放的是暂时无法合并的mov指令
                              // 如果有合并的可能后会加到worklist moves中

      // 处理过程
      // livenessanalysis() 活跃分析
      // build() 构造冲突图
      // mk_worklist() 初始化各个工作表 里面包含了全部的结点
      // simplify() 简化 从冲突图中删除度<K的结点(在simplifywl中)及其边
      // coleasce() 合并 在冲突图中把两个mov-rel结点合并(在freezewl中)
      // freeze() 冻结 把一个mov-rel结点冻结()
      // spill() 溢出
      // rewriteprogram() 插入对溢出变量的ldr和str
      // addcalleesavereg() 在函数开头插入对使用的callee save寄存器的保存

      // 把u-v v-u添加到冲突图中 不维护预着色结点的邻接表
      auto add_edge = [&adj_set, &adj_list, &degree](RegId u, RegId v) {
        if (adj_set.find({u, v}) == adj_set.end() && u != v) {
          adj_set.insert({u, v});
          adj_set.insert({v, u});
          if (!IS_PRECOLORED(u)) {
            adj_list[u].insert(v);
            ++degree[u];
          }
          if (!IS_PRECOLORED(v)) {
            adj_list[v].insert(u);
            ++degree[v];
          }
        }
      };

      // 根据每个bb中的livein liveout计算每条指令的livein liveout 据此构造冲突图
      auto build = [&]() {
        for (auto bb : func->bb_list_) {
          // live-out[B] is also live-out[B-last-inst]
          auto live = bb->liveout_;
          for (auto inst_iter = bb->inst_list_.rbegin(); inst_iter != bb->inst_list_.rend(); ++inst_iter) {
            auto [def, use] = GetDefUse(*inst_iter);
            if (auto src_inst = dynamic_cast<Move *>(*inst_iter)) {
              // 对简单的mov指令要特殊处理 NOTE: 带条件的也可以
              if (!src_inst->op2_->is_imm_ && nullptr == src_inst->op2_->shift_ /*&& src_inst->cond_ == Cond::AL*/) {
                RegId op2_regid = src_inst->op2_->reg_->reg_id_;
                // mov s,t 并没有使得s和t冲突 所以从live中删除use 即op2
                live.erase(op2_regid);
                // 维护move_list和wl_moves
                move_list[src_inst->rd_->reg_id_].insert(src_inst);
                move_list[op2_regid].insert(src_inst);
                worklist_moves.insert(src_inst);
              }
            }
            // for edges between defs 可能live中已经有def了
            for (auto &d : def) {
              live.insert(d);
            }
            // for edges between def and live 定值点活跃视为冲突 TODO: 和同时活跃视为冲突相比?
            for (auto &d : def) {
              for (auto &l : live) {
                add_edge(l, d);
              }
            }
            // live-out[s-1]=live-in[s]=use[s]U(live-out[s]-def[s])
            // remove defs from live.  NOTE: can't omit
            for (auto &d : def) {
              live.erase(d);
              // TODO: 可以计算保留一些循环信息
            }
            // add uses to live
            for (auto u : use) {
              live.insert(u);
              // TODO: 可以计算保留一些循环信息
            }
          }
        }
      };

      // 根据move_list返回一个结点的有效的mov指令集合 有效的mov指令在active moves和worklist moves中
      // called NodeMoves in paper.
      auto valid_mov_set = [&move_list, &active_moves, &worklist_moves](RegId reg) {
        MovSet res = move_list[reg];
        for (auto iter = res.begin(); iter != res.end();) {
          if (active_moves.find(*iter) == active_moves.end() && worklist_moves.find(*iter) == worklist_moves.end()) {
            iter = res.erase(iter);
          } else {
            ++iter;
          }
        }
        return res;
      };

      // 如果该reg的有效mov指令集不空 则是mov-related
      auto move_related = [&valid_mov_set](RegId reg) { return !valid_mov_set(reg).empty(); };

      // 根据degree填三张worklist
      auto mk_worklist = [&degree, &spill_worklist, &freeze_worklist, &func, &simplify_worklist, &K, &move_related]() {
        for (RegId i = 16; i < func->virtual_reg_max; ++i) {
          if (degree.find(i) == degree.end()) {  // 没在degree中说明度数为0
            degree[i] = 0;
          }
          if (degree[i] >= K) {
            spill_worklist.insert(i);
          } else if (move_related(i)) {
            freeze_worklist.insert(i);
          } else {
            simplify_worklist.insert(i);
          }
        }
      };

      // active_moves---mov-->worklist_moves 激活可能合并的mov指令
      // 放在active_moves中说明之前尝试合并该mov指令失败了
      auto enable_moves = [&](RegId reg) {
        // enablemoves({reg}UValidAdjacent(reg))
        for (auto m : valid_mov_set(reg)) {
          if (active_moves.find(m) != active_moves.end()) {
            active_moves.erase(m);
            worklist_moves.insert(m);
          }
        }

        auto &&adj_regs = ValidAdjacentSet(reg, adj_list, select_stack, colored_nodes);
        for (auto adj_reg : adj_regs) {
          for (auto m : valid_mov_set(adj_reg)) {
            if (active_moves.find(m) != active_moves.end()) {
              active_moves.erase(m);
              worklist_moves.insert(m);
            }
          }
        }
      };

      // auto enable_moves = [&](WorkList)

      auto decrement_degree = [&spill_worklist, &simplify_worklist, &freeze_worklist, &degree, &move_related,
                               &enable_moves](RegId reg) {
        --degree[reg];
        // degree可能从K变为了K-1 增加了简化与合并的机会 spill_worklist---reg-->simplify/freeze_worklist
        if (degree[reg] + 1 == K) {
          // 基于启发式规则 这个结点和它的所有邻接点的所有mov指令都有可能被合并
          enable_moves(reg);
          spill_worklist.erase(reg);
          if (move_related(reg)) {
            freeze_worklist.insert(reg);
          } else {
            simplify_worklist.insert(reg);
          }
        }
      };

      // 逻辑上从冲突图中删除某节点 实际上是simplify_worklist---reg-->select_stack以及邻接点degree的更新
      auto simplify = [&decrement_degree, &simplify_worklist, &select_stack, this, &adj_list, &coalesced_nodes]() {
        // if (simplify_worklist.empty()) return;
        RegId reg = *simplify_worklist.begin();
        simplify_worklist.erase(reg);
        select_stack.push_back(reg);
        // 所有邻接结点的度数-1
        auto &&valid_adj_set = this->ValidAdjacentSet(reg, adj_list, select_stack, coalesced_nodes);
        // outfile << valid_adj_set.size() << std::endl;
        for (auto &m : valid_adj_set) {
          decrement_degree(m);
        }
      };

      // 拿到这些结点被合并到的那一个结点 TODO: 并查集
      auto get_alias = [&alias, &coalesced_nodes](RegId reg) {
        while (std::find(coalesced_nodes.begin(), coalesced_nodes.end(), reg) != coalesced_nodes.end()) {
          reg = alias[reg];
        }
        return reg;
      };

      // 合并策略Briggs: 合并后结点的高度数邻接点个数少于K 用于两个virtual reg合并的情况 b->a
      // called Conservative() in paper.
      auto briggs_coalesce_test = [&](RegId a, RegId b) {
        auto &&adj_a = ValidAdjacentSet(a, adj_list, select_stack, coalesced_nodes);
        int cnt = 0;
        for (auto n : adj_a) {
          // 如果是公共邻接点 则现在度数-1
          int deg = degree[n];
          if (adj_set.find({n, b}) != adj_set.end()) {
            --deg;
          }
          if (deg >= K) {
            ++cnt;
          }
        }
        auto &&adj_b = ValidAdjacentSet(b, adj_list, select_stack, coalesced_nodes);
        for (auto n : adj_b) {
          if (adj_set.find({a, n}) == adj_set.end() && degree[n] >= K) {
            ++cnt;
          }
        }
        return cnt < K;
      };

      // George: a->b a的每一个adj t 要么和b冲突 要么低度数 用于一个virtual reg要合并到precolored reg的情况 a->b
      // called OK() in paper.
      auto george_coalesce_test = [&](RegId a, RegId b) {
        assert(IS_PRECOLORED(b));
        auto &&valid_adj_regs = ValidAdjacentSet(a, adj_list, select_stack, coalesced_nodes);
        for (auto adj_reg : valid_adj_regs) {
          if (degree[adj_reg] < K || IS_PRECOLORED(adj_reg) || adj_set.find({adj_reg, b}) != adj_set.end()) {
            continue;
          } else {
            return false;
          }
        }
        return true;
      };

      // 从worklist movs中移除一条mov指令后会调用该函数 尝试freeze_worklist---reg-->simplify_worklist
      auto add_worklist = [&](RegId reg) {
        if (!IS_PRECOLORED(reg) && !move_related(reg) && degree[reg] < K) {
          freeze_worklist.erase(reg);
          simplify_worklist.insert(reg);
        }
      };

      // 合并结点操作 把b合并到a 公共邻接点度数-1 自身度数可能变大 维护movelist 维护冲突图
      // freeze/spill_worklist---b-->coalesced_nodes freeze_worklist --a--> spill_worklist
      auto combine = [&](RegId a, RegId b) {
        auto it = freeze_worklist.find(b);
        if (it != freeze_worklist.end()) {
          freeze_worklist.erase(it);
        } else {
          spill_worklist.erase(b);
        }
        coalesced_nodes.insert(b);
        alias[b] = a;

        auto &m = move_list[a];
        for (auto n : move_list[b]) {
          m.insert(n);
        }
        // enable_moves(b); 原来处理有b的一条mov指令失败 合并后处理这条指令仍会失败
        for (auto t : ValidAdjacentSet(b, adj_list, select_stack, coalesced_nodes)) {
          add_edge(t, a);
          decrement_degree(t);
        }

        // 合并后可能原来度数<K现在>=K 需要移动
        if (degree[a] >= K && freeze_worklist.find(a) != freeze_worklist.end()) {
          freeze_worklist.erase(a);
          spill_worklist.insert(a);
        }
      };

      auto coalesce = [&]() {
        // 有两种合并 根据两种启发式规则进行 一种是virtual reg到precolored reg的合并 另一种是virtual regs之间的合并
        // TODO: 或许可以用启发式规则选择要合并的mov指令
        auto m = *worklist_moves.begin();
        auto u = get_alias(m->rd_->reg_id_);
        auto v = get_alias(m->op2_->reg_->reg_id_);

        if (IS_PRECOLORED(v)) {
          auto temp = u;
          u = v;
          v = temp;
        }

        worklist_moves.erase(m);

        // u和v之前一定是mov-related 所以要么在spill wl中 要么在freeze wl中
        // 从wl moves中删除该mov指令后 u和v的mov有关无关状态可能发生改变 如果成功合并 可能会改变u和v的度数
        // 如果变为mov无关并且度数<K并且非precolored 则将其从freeze worklist移到simplify worklist中去 即AddWorklist函数
        if (u == v) {  // 已经合并了
          coalesced_moves.insert(m);
          add_worklist(u);
          return;
        }
        if ((IS_PRECOLORED(v)) || adj_set.find({u, v}) != adj_set.end()) {  // 冲突
          constrained_moves.insert(m);
          add_worklist(u);
          add_worklist(v);
          return;
        }
        // 此时要么u precolored 要么都是virtual register
        if ((IS_PRECOLORED(u) && george_coalesce_test(v, u)) || (!IS_PRECOLORED(u) && briggs_coalesce_test(u, v))) {
          coalesced_moves.insert(m);
          combine(u, v);  // 执行合并操作v->u 修改相应的数据结构
          add_worklist(u);
          return;
        }
        // 无法合并
        active_moves.insert(m);
        return;
      };

      auto freeze_moves = [&](RegId reg) {
        // 把和reg有关的 本来可能合并的mov指令冻结
        for (auto m : valid_mov_set(reg)) {
          if (active_moves.find(m) != active_moves.end()) {
            active_moves.erase(m);
          } else {
            worklist_moves.erase(m);
          }
          frozen_moves.insert(m);

          // 冻结后 该mov指令的另一个结点可能变为mov无关
          auto v = m->rd_->reg_id_ == reg ? m->op2_->reg_->reg_id_ : m->rd_->reg_id_;
          if (!move_related(v) && degree[v] < K) {
            freeze_worklist.erase(v);
            simplify_worklist.insert(v);
          }
        }
      };

      auto freeze = [&freeze_worklist, &simplify_worklist, &freeze_moves]() {
        // if (freeze_worklist.empty()) return;
        // 把低度数的传送有关的结点冻结为传送无关
        // TODO: 这里或许可以用启发式规则选择要冻结的结点
        RegId reg = *freeze_worklist.begin();
        freeze_worklist.erase(reg);
        simplify_worklist.insert(reg);
        // 相关的mov指令也要冻结
        freeze_moves(reg);
      };

      // TODO: 溢出代价计算
      auto select_spill = [&]() {
        // select node with max degree (heuristic)
        RegId reg = *std::max_element(spill_worklist.begin(), spill_worklist.end(),
                                      [&](auto a, auto b) { return degree[a] < degree[b]; });
        // 选择的结点可能mov有关也可能mov无关
        simplify_worklist.insert(reg);
        freeze_moves(reg);
        spill_worklist.erase(reg);
      };

      auto assign_colors = [&]() {
        // virtual reg到颜色的一个映射
        std::unordered_map<RegId, RegId> colored;
        for (RegId i = 0; i < 16; ++i) {
          colored.insert({i, i});
        }

        // outfile << "选择栈中有" << select_stack.size() << "个结点" << std::endl;
        while (!select_stack.empty()) {
          auto vreg = select_stack.back();
          // outfile << "现在为" << vreg << "分配寄存器:" << std::endl;
          select_stack.pop_back();
          std::unordered_set<RegId> ok_colors;

          auto dbg_print_ok_colors = [&ok_colors, &outfile](std::string msg) {
            // outfile << "输出ok_colors " << msg << std::endl;
            for (auto ok_color : ok_colors) {
              // outfile << ok_color << " ";
            }
            // outfile << std::endl;
          };

          // 先填
          for (RegId i = 0; i < K - 1; ++i) {
            ok_colors.insert(i);
          }
          ok_colors.insert(static_cast<RegId>(ArmReg::lr));
          // dbg_print_ok_colors("填好之后");

          // 再删
          for (auto adj_reg : adj_list[vreg]) {
            auto real_reg = get_alias(adj_reg);
            if (colored.find(real_reg) != colored.end()) {
              ok_colors.erase(colored[real_reg]);
              //   outfile << "erase:" << colored[real_reg] << std::endl;
            }
          }
          // dbg_print_ok_colors("删完之后");

          // 最后分配
          if (ok_colors.empty()) {  // 实际溢出 不给它分配 继续进行其他结点的分配以找到全部的实际溢出结点
            spilled_nodes.insert(vreg);
          } else {  // 可分配
            auto color = *std::min_element(ok_colors.begin(), ok_colors.end());
            if (color >= 4 && color <= 11) {
              used_callee_saved_regs.insert(color);
            }
            colored[vreg] = color;
            // outfile << "给" << vreg << "结点分配了" << color << std::endl;
          }
        }

        if (!spill_worklist.empty()) {  // 有实际溢出的话 不继续分配
          return;
        }

        // 为合并了的结点分配颜色
        for (auto n : coalesced_nodes) {
          colored[n] = colored[get_alias(n)];
        }

        // DEBUG PRINT
        // for (auto &[before, after] : colored) {
        //   auto colored =
        //       std::to_string(before) + " => " + std::to_string(after);
        //   //   dbg(colored);
        //   outfile << colored << std::endl;
        // }

        // modify_armcode
        // replace usage of virtual registers
        for (auto bb : func->bb_list_) {
          for (auto inst : bb->inst_list_) {
            auto [def, use] = GetDefUsePtr(inst);
            // Reg *def;
            // std::vector<Reg *> use;
            for (auto &d : def) {
              assert(nullptr != d && colored.find(d->reg_id_) != colored.end());
              d->reg_id_ = colored[d->reg_id_];
            }
            for (auto &u : use) {
              assert(nullptr != u && colored.find(u->reg_id_) != colored.end());
              // if (u && colored.find(u->reg_id_) != colored.end()) {
              u->reg_id_ = colored[u->reg_id_];
              // }
            }
          }
        }
      };

      // outfile << "LivenessAnalysis Start:" << std::endl;
      ArmLivenessAnalysis::Run4Func(func);
      // outfile << "LivenessAnalysis End." << std::endl;

      // 机器寄存器预着色 每一个机器寄存器都不可简化 不可溢出 把这些寄存器的degree初始化为极大
      for (RegId i = 0; i < 16; ++i) {
        degree[i] = 0x3f3f3f3f;
      }

      auto dbg_print_worklist = [&outfile](WorkList &wl, std::string msg) {
        // outfile << msg << std::endl;
        for (auto n : wl) {
          // outfile << n << " ";
        }
        // outfile << std::endl;
      };

      // outfile << "Build Start:" << std::endl;
      build();
      // outfile << "End." << std::endl;
      // outfile << "MK_WL Start:" << std::endl;
      mk_worklist();
      // outfile << "End." << std::endl;
      do {
        if (!simplify_worklist.empty()) {
          // outfile << "Simplify Start:" << std::endl;
          // dbg_print_worklist(simplify_worklist, "可简化的结点如下:");
          simplify();
          // outfile << "End." << std::endl;
        } else if (!worklist_moves.empty()) {
          // outfile << "Coalesce Start:" << std::endl;
          coalesce();
          // outfile << "End." << std::endl;
        } else if (!freeze_worklist.empty()) {
          // dbg_print_worklist(simplify_worklist, "可冻结的结点如下:");
          // outfile << "Freeze Start:" << std::endl;
          freeze();
          // outfile << "End." << std::endl;
        } else if (!spill_worklist.empty()) {
          // outfile << "SelectSpill Start:" << std::endl;
          // dbg_print_worklist(simplify_worklist, "可能溢出的结点如下:");
          select_spill();
          // outfile << "End." << std::endl;
        }
      } while (!simplify_worklist.empty() || !worklist_moves.empty() || !freeze_worklist.empty() ||
               !spill_worklist.empty());
      // outfile << "Coloring Start:" << std::endl;
      assign_colors();
      // outfile << "End." << std::endl;
      if (spilled_nodes.empty()) {
        done = true;
      } else {
        // NOTE: don't test.
        // outfile << "actual spill" << std::endl;
        // rewrite program 会导致func的virtual reg max和stack size属性发生变化
        for (auto &spill_reg : spilled_nodes) {
          // allocate on stack
          auto offset = func->stack_size_;
          for (auto bb : func->bb_list_) {
            auto gen_ldrstr_inst = [&func, &offset, &bb, &spill_reg](std::vector<Instruction *>::iterator iter,
                                                                     LdrStr::OpKind opkind) {
              Instruction *inst;
              if (LdrStr::CheckImm12(offset)) {
                // NOTE: sp_vreg一定为r16
                inst = static_cast<Instruction *>(
                    new LdrStr(opkind, LdrStr::Type::Norm, Cond::AL, new Reg(spill_reg), new Reg(16), offset));
                if (opkind == LdrStr::OpKind::LDR) {
                  return bb->inst_list_.insert(iter, inst);
                } else {
                  return bb->inst_list_.insert(iter + 1, inst);
                }
              } else {
                auto vreg = new Reg(func->virtual_reg_max++);
                if (Operand2::CheckImm8m(offset)) {
                  inst = static_cast<Instruction *>(new Move(false, Cond::AL, vreg, new Operand2(offset)));
                  // } else if (offset < 0 && Operand2::CheckImm8m(-offset - 1)) {  // mvn
                  //   assert(0);
                  //   inst = static_cast<Instruction *>(new Move(false, Cond::AL, vreg, new Operand2(-offset - 1),
                  //   true));
                } else {
                  inst = static_cast<Instruction *>(new LdrPseudo(Cond::AL, vreg, offset));
                }
                if (opkind == LdrStr::OpKind::LDR) {
                  iter = bb->inst_list_.insert(iter, inst);
                } else {
                  iter = bb->inst_list_.insert(iter + 1, inst);
                }
                ++iter;
                inst = static_cast<Instruction *>(new LdrStr(opkind, LdrStr::Type::Norm, Cond::AL, new Reg(spill_reg),
                                                             new Reg(16), new Operand2(vreg)));
                if (opkind == LdrStr::OpKind::LDR) {
                  return bb->inst_list_.insert(iter, inst);
                } else {
                  return bb->inst_list_.insert(iter + 1, inst);
                }
              }
            };
            // 找到基本块中每一次定值和使用 添加相应的ldr和str语句
            for (auto iter = bb->inst_list_.begin(); iter != bb->inst_list_.end();) {
              auto [def, use] = GetDefUsePtr(*iter);
              for (auto &u : use) {
                if (u->reg_id_ == spill_reg) {  // load
                  iter = gen_ldrstr_inst(iter, LdrStr::OpKind::LDR);
                }
              }
              for (auto &d : def) {
                if (d->reg_id_ == spill_reg) {  // store
                  iter = gen_ldrstr_inst(iter, LdrStr::OpKind::STR);
                }
              }
              ++iter;
            }
          }
          func->stack_size_ += 4;  // increase stack size
        }
        done = false;
      }
    }
    // 每一个函数确定了之后 更改push/pop指令 更改add/sub sp sp stack_size指令 删除自己到自己的mov指令
    int stack_size_diff = func->stack_size_ - src_stack_size;
    int offset_fixup_diff = used_callee_saved_regs.size() * 4 + stack_size_diff;  // maybe -4
    if (func->IsLeaf()) {                                                         //原本计算有lr
      offset_fixup_diff -= 4;
    }
    for (auto bb : func->bb_list_) {
      for (auto iter = bb->inst_list_.begin(); iter != bb->inst_list_.end();) {
        if (auto pushpop_inst = dynamic_cast<PushPop *>(*iter)) {
          // pushpop_inst->EmitCode(outfile);
          pushpop_inst->reg_list_.clear();
          for (auto reg : used_callee_saved_regs) {
            pushpop_inst->reg_list_.push_back(new Reg(reg));
          }
          if (pushpop_inst->opkind_ == PushPop::OpKind::PUSH) {
            if (!func->IsLeaf()) {
              pushpop_inst->reg_list_.push_back(new Reg(ArmReg::lr));
            }
            if (pushpop_inst->reg_list_.empty()) {
              iter = bb->inst_list_.erase(iter);
              continue;
            }
          } else {
            if (!func->IsLeaf()) {
              pushpop_inst->reg_list_.push_back(new Reg(ArmReg::pc));
            }
            if (pushpop_inst->reg_list_.empty()) {
              iter = bb->inst_list_.erase(iter);
            } else {
              ++iter;
            }
            // 此时iter指向原pop指令的下一条指令
            if (func->IsLeaf()) {  // 插入一条 bx lr
              iter = bb->inst_list_.insert(iter, static_cast<Instruction *>(new Branch(false, true, Cond::AL, "lr")));
            }
            continue;
          }
          ++iter;
        } else if (auto move_inst = dynamic_cast<Move *>(*iter)) {  // 删除自己到自己的mov语句
          auto op2 = move_inst->op2_;
          if (!op2->is_imm_ && nullptr == op2->shift_ && move_inst->rd_->reg_id_ == op2->reg_->reg_id_) {
            iter = bb->inst_list_.erase(iter);
          } else {
            ++iter;
          }
        } else {
          ++iter;
        }
      }
    }
    // push和pop中添加了r4-r11 或者删除了lr和sp的话 需要修复栈中实参的位置
    for (auto inst : func->sp_arg_fixup_) {
      if (auto src_inst = dynamic_cast<Move *>(inst)) {
        assert(src_inst->op2_->is_imm_);
        if (src_inst->is_mvn_) {
          src_inst->op2_->imm_num_ -= offset_fixup_diff;
        } else {
          src_inst->op2_->imm_num_ += offset_fixup_diff;
        }
      } else if (auto src_inst = dynamic_cast<LdrStr *>(inst)) {
        assert(src_inst->is_offset_imm_);
        src_inst->offset_imm_ += offset_fixup_diff;
      } else if (auto src_inst = dynamic_cast<LdrPseudo *>(inst)) {
        assert(src_inst->IsImm());
        src_inst->imm_ += offset_fixup_diff;
      } else if (auto src_inst = dynamic_cast<BinaryInst *>(inst)) {
        assert(src_inst->op2_->is_imm_);
        src_inst->op2_->imm_num_ += stack_size_diff;
      }
    }

  }  // end of func loop
}
