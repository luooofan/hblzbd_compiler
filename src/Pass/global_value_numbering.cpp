#include "../../include/Pass/global_value_numbering.h"

#include <algorithm>
#include <string>
#include <unordered_set>

#include "../../include/Pass/dead_code_eliminate.h"
#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"

#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_GVN_PROCESS

void GlobalValueNumbering::Run() {
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
    exp_val_vec.clear();
    Run4Func(func);
  }
}

void GlobalValueNumbering::Run4Func(SSAFunction* func) {
#ifdef DEBUG_GVN_PROCESS
  std::cout << "CommenExpressionEliminate for function: " << func->GetFuncName() << std::endl;
#endif

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
        // 满足交换律的加和乘操作 假设最多只有一个操作数是常数 把常数操作数放后面(之前执行SimpleOptimizePass即满足假设)
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
        // 比较每个操作数是否一致 对于常数来说比较的是立即数值 对于其他value来说比较的是指针
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

    // 二元运算语句
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
    // 一元运算语句
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
    // Call语句 需要关注call的函数是否有副作用 有副作用则不可删 kill掉原来的
    else if (auto src_inst = dynamic_cast<CallInst*>(inst)) {
      auto func_val = dynamic_cast<FunctionValue*>(src_inst->GetOperand(0));
      MyAssert(nullptr != func_val);
      // 如果调用了有副作用的函数 不用处理该语句
      if (no_side_effect_funcs.find(func_val->GetFunction()) == no_side_effect_funcs.end()) {
        // TODO: 这里可以考虑把该基本块内的load exp都kill掉
        ++it;
        continue;
      }
      // 调用了void函数也可以算作表达式 虽然没有使用 但是可以删除一条多余的call
      // 如果前面做了死代码消除 不应该出现调用无副作用void函数的call语句

      // Expression exp(Expression::CALL);
      exp.op_ = Expression::CALL;
      for (int i = 0; i < src_inst->GetNumOperands(); ++i) {
        exp.operands_.push_back(src_inst->GetOperand(i));
      }

      find_and_replace(src_inst);
    }
    // Load语句 如果找到一条公共的load语句 需要检查从该语句开始到当前语句的所有执行路径上都没有对target的store语句
    else if (auto src_inst = dynamic_cast<LoadInst*>(inst)) {
      exp.op_ = Expression::LOAD;
      for (int i = 0; i < src_inst->GetNumOperands(); ++i) {
        exp.operands_.push_back(src_inst->GetOperand(i));
      }

      auto check_inst_range = [this](std::list<SSAInstruction*>::iterator begin,
                                     std::list<SSAInstruction*>::iterator end, Value* memory) {
        for (; begin != end; ++begin) {
          auto store_inst = dynamic_cast<StoreInst*>(*begin);
          // 是对同一块内存区域的store语句
          if (nullptr != store_inst && store_inst->GetOperand(1) == memory) {
            // TODO: 如果load和store语句的偏移都是常量的话可以比较立即数 如果不同 则跳过这条store语句
            return false;
          }
          auto call_inst = dynamic_cast<CallInst*>(*begin);
          if (nullptr != call_inst) {
            auto func_val = dynamic_cast<FunctionValue*>(call_inst->GetOperand(0));
            if (no_side_effect_funcs.find(func_val->GetFunction()) == no_side_effect_funcs.end()) {
              // 调用了副作用函数
              return false;
            }
          }
        }
        return true;
      };

      auto check = [this, &check_inst_range](SSAInstruction* source, LoadInst* target) {
        // 检查从source指令开始到target指定的所有路径上有没有对同一块内存区域的store语句或者对副作用函数的调用语句
        // source可能是load或store target一定是load
        auto source_bb = source->GetParent();
        auto target_bb = target->GetParent();
        auto memory = target->GetOperand(0);
        // 1 if source and target in the same bb
        if (source_bb == target_bb) {
          auto inst_list = source_bb->GetInstList();
          auto st = ++std::find(inst_list.begin(), inst_list.end(), source),
               ed = std::find(st, inst_list.end(), target);
          return check_inst_range(st, ed, memory);
        }
        // 2 in different bbs
        else {
          // 2.1 check source bb
          auto src_inst_list = source_bb->GetInstList();
          if (!check_inst_range(++std::find(src_inst_list.begin(), src_inst_list.end(), source), src_inst_list.end(),
                                memory))
            return false;
          // 2.2 check target bb
          auto tar_inst_list = target_bb->GetInstList();
          if (!check_inst_range(tar_inst_list.begin(), std::find(tar_inst_list.begin(), tar_inst_list.end(), target),
                                memory))
            return false;
          // 2.3 dfs all road
          // dfs get all roads
          std::unordered_set<SSABasicBlock*> vis;
          std::vector<SSABasicBlock*> road;
          std::vector<std::vector<SSABasicBlock*>> roads;
          GetAllRoads(source_bb, target_bb, road, roads, vis);
          // check all roads
          std::unordered_set<SSABasicBlock*> check_true_bbs;
          for (auto& road : roads) {
            for (auto bb : road) {
              if (check_true_bbs.count(bb)) continue;
              auto inst_list = bb->GetInstList();
              if (!check_inst_range(inst_list.begin(), inst_list.end(), memory))
                return false;
              else
                check_true_bbs.insert(bb);
            }
          }
          return true;
        }
      };

      // NOTE: reverse find 保证找到最新的一个
      auto vec_it = std::find_if(exp_val_vec.rbegin(), exp_val_vec.rend(), find_if_cmp);
      if (vec_it == exp_val_vec.rend()) {
        exp_val_vec.push_back({exp, src_inst});
        ++it;
      } else {
        // check决定能否replace if check true, perform replace. otherwise add exp to exp_val_vec
        if (check((*vec_it).first.inst_, src_inst)) {
          replace(src_inst, (*vec_it).second);
        } else {
          exp_val_vec.push_back({exp, src_inst});
          ++it;
        }
      }
    }
    // Store语句 可以看成一条load语句 同时可以kill掉该基本块内的对同一块内存操作的load语句
    else if (auto src_inst = dynamic_cast<StoreInst*>(inst)) {
      exp.op_ = Expression::LOAD;
      for (int i = 1; i < src_inst->GetNumOperands(); ++i) {
        exp.operands_.push_back(src_inst->GetOperand(i));
      }

      // 不管有没有 直接加进去即可 之后再找相同的load exp 保证找到最新(靠后)的一个就行
      exp_val_vec.push_back({exp, src_inst->GetOperand(0)});
      ++it;
    }
    // 其他语句
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

void GlobalValueNumbering::GetAllRoads(SSABasicBlock* source, SSABasicBlock* target, std::vector<SSABasicBlock*>& road,
                                       std::vector<std::vector<SSABasicBlock*>>& roads,
                                       std::unordered_set<SSABasicBlock*>& vis) {
  // source一定没有被访问过
  MyAssert(!vis.count(source));

  if (source == target) {
    roads.push_back(road);
    return;
  }

  road.push_back(source);
  vis.insert(source);
  for (auto succ : source->GetSuccBB()) {
    if (vis.count(succ)) continue;
    GetAllRoads(succ, target, road, roads, vis);
  }
  vis.erase(source);
  road.pop_back();
  return;
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
                               SSABasicBlock* bb) {
  MyAssert(nullptr != bb);
  if (vis.find(bb) == vis.end()) {
    vis.insert(bb);
    for (auto succ : bb->GetSuccBB()) {
      MyAssert(nullptr != succ);
      DFS(po, vis, succ);
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