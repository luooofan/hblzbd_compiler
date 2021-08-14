#include "../../include/Pass/dead_code_eliminate.h"

#include <unordered_map>
#include <unordered_set>

#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"

// #define DEBUG_DEAD_CODE_ELIMINATE_PROCESS
// #define DEBUG_DEAD_STORE_ELIMINATE_PROCESS

void DeadCodeEliminate::Run() {
  auto m = dynamic_cast<SSAModule*>(*(this->m_));
  MyAssert(nullptr != m);
  DeleteDeadFunc(m);
  FindNoSideEffectFunc(m, no_side_effect_funcs);
  for (auto func : m->GetFuncList()) {
    DeleteDeadInst(func);
    func->MaintainCallFunc();  // 有可能删除call语句
    DeleteDeadStore(func);
  }
  DeleteDeadFunc(m);
}

void DeadCodeEliminate::DeleteDeadFunc(SSAModule* m) {
#ifdef DEBUG_DEAD_CODE_ELIMINATE_PROCESS
  std::cout << "Try To Remove Function: " << std::endl;
#endif
  std::unordered_set<SSAFunction*> worklist;
  for (auto func : m->GetFuncList()) {
    if ("main" == func->GetFuncName()) continue;
    worklist.insert(func);
  }
  while (!worklist.empty()) {
    auto func = *worklist.begin();
    worklist.erase(func);
    if (!func->IsCalled()) {
      // 应该把该函数调用的函数加入工作表
      for (auto call_func : func->GetCallFuncList()) {
        if ("main" == call_func->GetFuncName()) continue;
        worklist.insert(call_func);
      }
      // 说明该函数没被调用过 是dead function可以被删除 应该维护的信息全交由Remove函数完成
#ifdef DEBUG_DEAD_CODE_ELIMINATE_PROCESS
      std::cout << "  Remove Function: " << func->GetFuncName() << std::endl;
#endif
      func->Remove();
      delete func;
    }
  }
}

bool DeadCodeEliminate::IsSideEffect(Value* val) {
  auto inst_val = dynamic_cast<SSAInstruction*>(val);
  return IsSideEffect(inst_val);
}
bool DeadCodeEliminate::IsSideEffect(SSAInstruction* inst) {
  if (nullptr == inst) return true;
  // 除callinst外 如果本身无定值[no used]视为有副作用
  if (dynamic_cast<BinaryOperator*>(inst) || dynamic_cast<UnaryOperator*>(inst) || dynamic_cast<LoadInst*>(inst) ||
      dynamic_cast<MovInst*>(inst) || dynamic_cast<PhiInst*>(inst)) {
    return false;
  } else if (auto call_inst = dynamic_cast<CallInst*>(inst)) {
    // 如果call语句无副作用 即便没有定值 也可删除
    auto func_val = dynamic_cast<FunctionValue*>(call_inst->GetOperand(0));
    MyAssert(nullptr != func_val);
    if (no_side_effect_funcs.find(func_val->GetFunction()) != no_side_effect_funcs.end()) return false;
  }
  return true;
}

void DeadCodeEliminate::DeleteDeadInst(SSAFunction* func) {
#ifdef DEBUG_DEAD_CODE_ELIMINATE_PROCESS
  std::cout << "Try To Remove Instruction in function: " << func->GetFuncName() << std::endl;
#endif
  // 没有使用过的一个定值value 如果其定值语句没有副作用 就可删除
  std::unordered_set<SSAInstruction*> worklist;
  for (auto bb : func->GetBasicBlocks()) {
    for (auto inst : bb->GetInstList()) {
      // 把有定值的 无副作用的语句加入到工作表中 有的指令是一次定值(在ssa.h中指定了[used]的) 有的不是([no used])
      if (!IsSideEffect(inst)) worklist.insert({inst});
    }
  }

  while (!worklist.empty()) {
    auto inst = *worklist.begin();
    auto bb = inst->GetParent();
    worklist.erase(inst);
    if (inst->IsUsed()) continue;
#ifdef DEBUG_DEAD_CODE_ELIMINATE_PROCESS
    std::cout << "  remove: ";
    inst->Print(std::cout);
#endif
    // 其使用可能优化
    for (auto& use : inst->GetOperands()) {
      auto src_inst = dynamic_cast<SSAInstruction*>(use.Get());
      if (!IsSideEffect(src_inst)) {
        worklist.insert(src_inst);
      }
    }
    bb->RemoveInstruction(inst);  // FIXME 只是从指令列表中移除 并未释放内存
    delete inst;  // delete会删除inst的所有操作数 移除其操作数对应的使用 还会维护所有被使用信息
  }
}

void DeadCodeEliminate::RemoveFromNoSideEffectFuncs(SSAFunction* func,
                                                    std::unordered_set<SSAFunction*>& no_side_effect_funcs) {
  // 如果已经删了说明已经访问过直接返回
  if (no_side_effect_funcs.find(func) == no_side_effect_funcs.end()) return;
  // 先把func删了
  no_side_effect_funcs.erase(func);
  // 再把func的每个caller都删了
  for (auto use : func->GetValue()->GetUses()) {
    auto call_inst = dynamic_cast<CallInst*>(use->GetUser());
    MyAssert(nullptr != call_inst);
    auto caller = call_inst->GetParent()->GetFunction();
    // no_side_effect_funcs.erase(caller);
    RemoveFromNoSideEffectFuncs(caller, no_side_effect_funcs);
  }
}

void DeadCodeEliminate::FindNoSideEffectFunc(SSAModule* m, std::unordered_set<SSAFunction*>& no_side_effect_funcs) {
  // NOTE: 这里并没有把副作用的信息记录到函数中
  // 没有副作用的函数为 1.函数自身没有对数组类型参数和全局量有赋值(store)行为 2.同时没有调用有副作用的函数
  // 具体来说 如果函数pointer类型的argument的使用没有store语句 用了的globalvariable的使用也只有load语句即满足第一点
  no_side_effect_funcs.clear();
  for (auto func : m->GetFuncList()) {
    no_side_effect_funcs.insert(func);
  }

  // 每个builtin函数都是有副作用的
  for (auto builtin_func : m->GetBuiltinFuncList()) {
    no_side_effect_funcs.insert(builtin_func);
    RemoveFromNoSideEffectFuncs(builtin_func, no_side_effect_funcs);
  }

  for (auto func : m->GetFuncList()) {
    if (no_side_effect_funcs.find(func) == no_side_effect_funcs.end()) continue;
    bool has_side_effect = false;
    for (Argument* arg : func->GetValue()->GetArgList()) {
      for (Use* use : arg->GetUses()) {
        if (nullptr != dynamic_cast<StoreInst*>(use->GetUser())) goto setflag;
      }
    }
    for (GlobalVariable* glo : func->GetUsedGlobVarList()) {
      for (Use* use : glo->GetUses()) {
        auto inst = dynamic_cast<SSAInstruction*>(use->GetUser());
        MyAssert(nullptr != inst);
        if (func == inst->GetParent()->GetFunction()) {
          if (nullptr != dynamic_cast<StoreInst*>(inst)) goto setflag;
        }
      }
    }
    continue;
  setflag:
    has_side_effect = true;
    // propagate to caller
    RemoveFromNoSideEffectFuncs(func, no_side_effect_funcs);
  }
#ifdef DEBUG_DEAD_CODE_ELIMINATE_PROCESS
  std::cout << "No side effect functions: ";
  for (auto func : no_side_effect_funcs) {
    std::cout << func->GetFuncName() << " ";
  }
  std::cout << std::endl;
#endif
}

void DeadCodeEliminate::DeleteDeadStore(SSAFunction* func) {
#ifdef DEBUG_DEAD_STORE_ELIMINATE_PROCESS
  std::cout << "Dead Store Eliminate for Function: " << func->GetFuncName() << std::endl;
#endif

  for (auto bb : func->GetBasicBlocks()) {
#ifdef DEBUG_DEAD_STORE_ELIMINATE_PROCESS
    std::cout << "  BB: " << bb->GetLabel() << std::endl;
#endif

    for (auto it = bb->GetInstList().begin(); it != bb->GetInstList().end(); ++it) {
      auto store_inst = dynamic_cast<StoreInst*>(*it);
      if (nullptr != store_inst) {
        auto it2 = ++it;
        --it;
        for (; it2 != bb->GetInstList().end(); ++it2) {
          // 如果是一条load指令 并且操作同一块内存区域 除非两个都是立即数且不相同 否则直接break
          if (auto src_inst = dynamic_cast<LoadInst*>(*it2)) {
            if (store_inst->GetNumOperands() - 1 == src_inst->GetNumOperands() &&
                src_inst->GetOperand(0) == store_inst->GetOperand(1)) {
              if (store_inst->GetNumOperands() == 3) {
                auto constint = dynamic_cast<ConstantInt*>(store_inst->GetOperand(2));
                auto constint2 = dynamic_cast<ConstantInt*>(src_inst->GetOperand(1));
                if (nullptr != constint && nullptr != constint2 && constint->GetImm() != constint2->GetImm()) {
                  continue;
                }
              }
              break;
            }
          }
          // 如果是一条call指令 并且调用的是有副作用的函数 直接break
          else if (auto src_inst = dynamic_cast<CallInst*>(*it2)) {
            if (!no_side_effect_funcs.count(dynamic_cast<FunctionValue*>(src_inst->GetOperand(0))->GetFunction()))
              break;
          }
          // 如果是一条操作同一内存位置的store指令 则前面这条store可删除
          else if (auto src_inst = dynamic_cast<StoreInst*>(*it2)) {
            if (store_inst->GetNumOperands() == src_inst->GetNumOperands() &&
                src_inst->GetOperand(1) == store_inst->GetOperand(1)) {
              if (store_inst->GetNumOperands() == 3) {
                auto constint = dynamic_cast<ConstantInt*>(store_inst->GetOperand(2));
                auto constint2 = dynamic_cast<ConstantInt*>(src_inst->GetOperand(2));
                if (store_inst->GetOperand(2) == src_inst->GetOperand(2) ||
                    (nullptr != constint && nullptr != constint2 && constint->GetImm() == constint2->GetImm())) {
#ifdef DEBUG_DEAD_STORE_ELIMINATE_PROCESS
                  std::cout << "  find dead store: ", store_inst->Print(std::cout);
                  std::cout << "               ==> ", src_inst->Print(std::cout);
#endif

                  it = bb->GetInstList().erase(it);
                  --it;
                  delete store_inst;
                  break;
                }
              } else {
#ifdef DEBUG_DEAD_STORE_ELIMINATE_PROCESS
                std::cout << "  find dead store: ", store_inst->Print(std::cout);
                std::cout << "               ==> ", src_inst->Print(std::cout);
#endif

                it = bb->GetInstList().erase(it);
                --it;
                delete store_inst;
                break;
              }
            }
          }
        }
      }
    }
  }
}