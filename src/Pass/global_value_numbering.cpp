#include "../../include/Pass/global_value_numbering.h"

#include <algorithm>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>

#include "../../include/Pass/dead_code_eliminate.h"
#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_GVN_PROCESS
// #define DEBUG_GVN_EFFECT
static int kCount = 0;

void GlobalValueNumbering::Run() {
  kCount = 0;
  auto m = dynamic_cast<SSAModule*>(*(this->m_));
  MyAssert(nullptr != m);
  DeadCodeEliminate::FindNoSideEffectFunc(m, no_side_effect_funcs);

#ifdef DEBUG_GVN_PROCESS
  std::cout << "No side effect functions: ";
  for (auto func : no_side_effect_funcs) {
    std::cout << func->GetFuncName() << " ";
  }
  std::cout << std::endl;
#endif

  for (auto func : m->GetFuncList()) {
    // find back-edge
    std::set<std::pair<SSABasicBlock*, SSABasicBlock*>> back_edges;
    for (auto bb : func->GetBasicBlocks()) {
      auto&& dom_set = bb->GetDomSet();
      for (auto succ : bb->GetSuccBB()) {
        if (dom_set.count(succ)) back_edges.insert({bb, succ});
      }
    }
    // find loop and fill bbs_in_loop
    bbs_in_loop.clear();
    std::queue<SSABasicBlock*> q;
    std::unordered_set<SSABasicBlock*> loop;
    for (auto [st, ed] : back_edges) {
      MyAssert(q.empty() && loop.empty());
      loop.insert(ed);
      q.push(st);
      while (!q.empty()) {
        auto front = q.front();
        q.pop();
        if (loop.count(front)) continue;
        loop.insert(front);
        for (auto pred : front->GetPredBB()) q.push(pred);
      }
      bbs_in_loop.insert(loop.begin(), loop.end());
      loop.clear();
    }
    // prepared run for function
    exp_val_vec.clear();
    Run4Func(func);
  }

#ifdef DEBUG_GVN_EFFECT
  std::cout << "GVN Pass works on " << kCount << " insts" << std::endl;
#endif
}

void GlobalValueNumbering::Run4Func(SSAFunction* func) {
#ifdef DEBUG_GVN_PROCESS
  std::cout << "CommenExpressionEliminate for function: " << func->GetFuncName() << std::endl;
#endif
  // if (func->GetFuncName() != "main") return;  // debug for single function in single testcase

  if (func->GetBBList().size() > 0) CommenExpressionEliminate(func->GetBBList().front());
}

void GlobalValueNumbering::CommenExpressionEliminate(SSABasicBlock* bb) {
#ifdef DEBUG_GVN_PROCESS
  std::cout << "  BB: " << bb->GetLabel() << std::endl;
#endif

  int src_vec_size = exp_val_vec.size();

  // compute exp and perform eliminate
  for (auto it = bb->GetInstList().begin(); it != bb->GetInstList().end();) {
    auto inst = *it;
    Expression exp(inst);

    auto find_if_cmp = [&exp](std::pair<Expression, Value*>& kv) {
      if (exp.op_ == kv.first.op_) {
        // ????????????????????????????????? ?????????????????????????????????????????? ???????????????????????????(????????????SimpleOptimizePass???????????????)
        if (exp.op_ == Expression::ADD || exp.op_ == Expression::MUL) {
          if (nullptr != dynamic_cast<ConstantInt*>(exp.operands_[0])) {
            auto temp = exp.operands_[1];
            exp.operands_[1] = exp.operands_[0];
            exp.operands_[0] = temp;
          }
          if (nullptr != dynamic_cast<ConstantInt*>(kv.first.operands_[0])) {
            auto temp = kv.first.operands_[1];
            kv.first.operands_[1] = kv.first.operands_[0];
            kv.first.operands_[0] = temp;
          }
        }
        // ????????????????????????????????? ?????????????????????????????????????????? ????????????value????????????????????????
        for (int i = 0; i < exp.operands_.size(); ++i) {
          if (auto constint = dynamic_cast<ConstantInt*>(exp.operands_[i])) {
            if (auto constint2 = dynamic_cast<ConstantInt*>(kv.first.operands_[i])) {
              if (constint->GetImm() == constint2->GetImm()) continue;
            } else {
              return false;
            }
          }
          if (exp.operands_[i] != kv.first.operands_[i]) return false;
        }
        return true;
      } else {
        return false;
      }
    };
    auto replace = [&bb, &it](Value* src, Value* target) {
      ++kCount;
#ifdef DEBUG_GVN_PROCESS
      std::cout << "  find redundant: ";
      src->Print(std::cout);
      std::cout << "             ==>: ";
      target->Print(std::cout);
      std::cout << std::endl;
#endif
      src->ReplaceAllUseWith(target);
      it = bb->GetInstList().erase(it);
      delete src;
    };
    auto find_and_replace = [&exp, &it, &find_if_cmp, &replace, this](Value* src) {
      auto vec_it = std::find_if(exp_val_vec.begin(), exp_val_vec.end(), find_if_cmp);
      if (vec_it == exp_val_vec.end()) {
        exp_val_vec.push_back({exp, src});
        ++it;
      } else {
        replace(src, (*vec_it).second);
      }
    };

    // ??????????????????
    if (auto src_inst = dynamic_cast<BinaryOperator*>(inst)) {
      switch (src_inst->op_) {
        case BinaryOperator::ADD:
          exp.op_ = Expression::ADD;
          break;
        case BinaryOperator::SUB:
          exp.op_ = Expression::SUB;
          break;
        case BinaryOperator::MUL:
          exp.op_ = Expression::MUL;
          break;
        case BinaryOperator::DIV:
          exp.op_ = Expression::DIV;
          break;
        case BinaryOperator::MOD:
          exp.op_ = Expression::MOD;
          break;
        default:
          MyAssert(0);
          break;
      }
      exp.operands_.push_back(src_inst->GetOperand(0));
      exp.operands_.push_back(src_inst->GetOperand(1));

      find_and_replace(src_inst);
    }
    // ??????????????????
    else if (auto src_inst = dynamic_cast<UnaryOperator*>(inst)) {
      switch (src_inst->op_) {
        case UnaryOperator::NEG:
          exp.op_ = Expression::NEG;
          break;
        case UnaryOperator::NOT:
          exp.op_ = Expression::NOT;
          break;
        default:
          MyAssert(0);
          break;
      }
      exp.operands_.push_back(src_inst->GetOperand(0));

      find_and_replace(src_inst);
    }
    // Call?????? ????????????call??????????????????????????? ???????????????????????? kill????????????
    else if (auto src_inst = dynamic_cast<CallInst*>(inst)) {
      auto func_val = dynamic_cast<FunctionValue*>(src_inst->GetOperand(0));
      MyAssert(nullptr != func_val);
      // ???????????????????????????????????? ?????????????????????
      if (no_side_effect_funcs.find(func_val->GetFunction()) == no_side_effect_funcs.end()) {
        // TODO: ???????????????????????????????????????load exp???kill???
        ++it;
        continue;
      }
      // ?????????void?????????????????????????????? ?????????????????? ?????????????????????????????????call
      // ????????????????????????????????? ?????????????????????????????????void?????????call??????

      // Expression exp(Expression::CALL);
      exp.op_ = Expression::CALL;
      for (int i = 0; i < src_inst->GetNumOperands(); ++i) {
        exp.operands_.push_back(src_inst->GetOperand(i));
      }

      find_and_replace(src_inst);
    }
    // Load?????? ???????????????????????????load?????? ?????????????????????????????????????????????????????????????????????????????????target???store??????
    else if (auto src_inst = dynamic_cast<LoadInst*>(inst)) {
      exp.op_ = Expression::LOAD;
      for (int i = 0; i < src_inst->GetNumOperands(); ++i) {
        exp.operands_.push_back(src_inst->GetOperand(i));
      }

      auto check_inst_range = [this](std::list<SSAInstruction*>::iterator begin,
                                     std::list<SSAInstruction*>::iterator end, LoadInst* target) {
        Value* memory = target->GetOperand(0);
        for (; begin != end; ++begin) {
          // ?????????????????????????????????call??????
          auto call_inst = dynamic_cast<CallInst*>(*begin);
          if (nullptr != call_inst) {
            auto func_val = dynamic_cast<FunctionValue*>(call_inst->GetOperand(0));
            if (no_side_effect_funcs.find(func_val->GetFunction()) == no_side_effect_funcs.end()) {
              return false;
            }
          }

          // ??????????????????????????????store??????
          auto store_inst = dynamic_cast<StoreInst*>(*begin);
          if (nullptr != store_inst && store_inst->GetOperand(1) == memory) {
            // ??????load???store?????????????????????????????????????????????????????? ???????????? ???????????????store??????
            // NOTE: ???????????? ???????????? ???????????? ?????????????????????????????????
            if (target->GetNumOperands() == 2 && store_inst->GetNumOperands() == 3) {
              if (auto constint = dynamic_cast<ConstantInt*>(target->GetOperand(1))) {
                if (auto constint2 = dynamic_cast<ConstantInt*>(store_inst->GetOperand(2))) {
                  if (constint->GetImm() != constint2->GetImm()) continue;
                }
              }
            }
            return false;
          }
        }
        return true;
      };

      auto check = [this, &check_inst_range](SSAInstruction* source, LoadInst* target) {
        // ?????????source???????????????target????????????????????????????????????????????????????????????store?????????????????????????????????????????????
        // source?????????load???store target?????????load
        // source->Print(std::cout);
        // target->Print(std::cout);
        auto source_bb = source->GetParent();
        auto target_bb = target->GetParent();
        // 1 if source and target in the same bb
        if (source_bb == target_bb) {
          auto inst_list = source_bb->GetInstList();
          auto st = ++std::find(inst_list.begin(), inst_list.end(), source),
               ed = std::find(st, inst_list.end(), target);
          return check_inst_range(st, ed, target);
        }
        // 2 in different bbs
        // NOTE: ?????????????????????????????? ?????? ???????????????????????????????????????
        else {
          // return false;
          // 2.1 check source bb
          auto src_inst_list = source_bb->GetInstList();
          if (!check_inst_range(++std::find(src_inst_list.begin(), src_inst_list.end(), source), src_inst_list.end(),
                                target))
            return false;
          // std::cout << 1 << std::endl;
          // 2.2 check target bb
          auto tar_inst_list = target_bb->GetInstList();
          if (!check_inst_range(tar_inst_list.begin(), std::find(tar_inst_list.begin(), tar_inst_list.end(), target),
                                target))
            return false;
          // std::cout << 2 << std::endl;
          // 2.3 check all reached bb in road
          std::unordered_set<SSABasicBlock*> src_reach;
          std::unordered_set<SSABasicBlock*> tar_reach;
          std::queue<SSABasicBlock*> q;
          std::unordered_set<SSABasicBlock*> vis;
          auto dfs = [&q, &vis](std::unordered_set<SSABasicBlock*>& reach, bool succ = true) {
            while (!q.empty()) {
              auto front = q.front();
              reach.insert(front);
              q.pop();
              for (auto next_bb : (succ ? front->GetSuccBB() : front->GetPredBB())) {
                if (vis.count(next_bb)) continue;
                q.push(next_bb);
                vis.insert(next_bb);
              }
            }
            vis.clear();
          };

          // 2.3.1 fill src_reach
          q.push(source_bb);
          vis.insert(source_bb);
          dfs(src_reach, true);

          // 2.3.2 fill tar_reach
          q.push(target_bb);
          vis.insert(target_bb);
          dfs(tar_reach, false);

          // 2.3.3 compute cross set
          std::unordered_set<SSABasicBlock*> reached_bb;
          for (auto reach_bb : src_reach)
            if (tar_reach.count(reach_bb)) reached_bb.insert(reach_bb);

          // 2.3.4 determine source_bb and target_bb whether in road or not(just end point).
          if (!bbs_in_loop.count(source_bb)) reached_bb.erase(source_bb);
          if (!bbs_in_loop.count(target_bb)) reached_bb.erase(target_bb);

          // 2.3.5 check all bbs in road
          for (auto bb : reached_bb) {
            auto inst_list = bb->GetInstList();
            if (!check_inst_range(inst_list.begin(), inst_list.end(), target)) return false;
          }

          // std::cout << 3 << std::endl;
          return true;
        }
      };

      // NOTE: reverse find ???????????????????????????
      auto vec_it = std::find_if(exp_val_vec.rbegin(), exp_val_vec.rend(), find_if_cmp);
      if (vec_it == exp_val_vec.rend()) {
        exp_val_vec.push_back({exp, src_inst});
        ++it;
      } else {
        // check????????????replace if check true, perform replace. otherwise add exp to exp_val_vec
        if (check((*vec_it).first.inst_, src_inst)) {
          replace(src_inst, (*vec_it).second);
        } else {
          exp_val_vec.push_back({exp, src_inst});
          ++it;
        }
      }
    }
    // Store?????? ??????????????????load?????? ??????????????????kill????????????????????????????????????????????????load??????
    else if (auto src_inst = dynamic_cast<StoreInst*>(inst)) {
      exp.op_ = Expression::LOAD;
      for (int i = 1; i < src_inst->GetNumOperands(); ++i) {
        exp.operands_.push_back(src_inst->GetOperand(i));
      }

      // ??????????????? ????????????????????? ?????????????????????load exp ??????????????????(??????)???????????????
      exp_val_vec.push_back({exp, src_inst->GetOperand(0)});
      ++it;
    }
    // ????????????
    else {
      ++it;
    }
  }

  // traverse dom tree
  for (auto dom : bb->GetDoms()) {
    CommenExpressionEliminate(dom);
  }

  // erase now bb exps from exp_val_vec
  exp_val_vec.erase(exp_val_vec.begin() + src_vec_size, exp_val_vec.end());
}

GlobalValueNumbering::Expression::operator std::string() const {
  std::string res = "";
  for (auto opn : operands_) {
    auto constint = dynamic_cast<ConstantInt*>(opn);
    if (nullptr != constint)
      res += " " + std::to_string(constint->GetImm());
    else
      res += " " + opn->GetName();
  }
  return res;
}

void GlobalValueNumbering::DFS(std::vector<SSABasicBlock*>& po, std::unordered_set<SSABasicBlock*>& vis,
                               SSABasicBlock* bb, bool succ) {
  MyAssert(nullptr != bb);
  if (vis.find(bb) == vis.end()) {
    vis.insert(bb);
    for (auto next_bb : (succ ? bb->GetSuccBB() : bb->GetPredBB())) {
      MyAssert(nullptr != next_bb);
      DFS(po, vis, next_bb);
    }
    po.push_back(bb);
  }
}

std::vector<SSABasicBlock*> GlobalValueNumbering::ComputeRPO(SSAFunction* func) {
  std::vector<SSABasicBlock*> po;
  std::unordered_set<SSABasicBlock*> vis;
  if (func->GetBBList().size() > 0) DFS(po, vis, func->GetBBList().front());
  std::reverse(po.begin(), po.end());
  return po;
}