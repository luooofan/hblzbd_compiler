#include "../../include/Pass/dead_function_eliminate.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"
#include "../../include/ast.h"
#include "../parser.hpp"
#include "../../include/Pass/ir_liveness_analysis.h"
#include <algorithm>

// #define PRINT_DEAD_RET_FUNCTION
// #define PRINT_CALL_IR

void print_set(std::unordered_set<std::string> func_set){
  for(auto name : func_set){
    std::cout << name << std::endl;
  }
}

bool isLiveness(IRBasicBlock *bb, ir::IR *end_ir, ir::Opn opn){
  std::unordered_set<std::string> keepalive = bb->liveout_;
  auto it = find(bb->ir_list_.begin(), bb->ir_list_.end(), end_ir);
  std::string opn_str = opn.name_ + "." + std::to_string(opn.scope_id_);

  for(auto iter = it + 1; iter != bb->ir_list_.end(); ++iter){
    auto ir = *iter;

    auto [def, use] = GetDefUse(ir);
 
    if(find(def.begin(), def.end(), opn_str) != def.end()){
      return false;
    }
    
    if(find(use.begin(), use.end(), opn_str) != use.end()){
      return true;
    }
  }

  if(keepalive.find(opn_str) != keepalive.end()){
    return true;
  }else{
    return false;
  }
}

void DeadFunctionEliminate::Run(){
  auto m = dynamic_cast<IRModule *>(*m_);
  IRLivenessAnalysis livenessanalysis = IRLivenessAnalysis(m_);
  livenessanalysis.Run();

  std::unordered_set<std::string> dead_ret_function;

  for(auto func : m->func_list_){
    for(auto bb : func->bb_list_){
      for(auto ir : bb->ir_list_){
        if(ir->op_ == ir::IR::OpKind::CALL){
#ifdef PRINT_CALL_IR
          ir->PrintIR();
#endif
          auto func_table_item = ir::gFuncTable[ir->opn1_.name_];
          if(func_table_item.ret_type_ == INT){
            if(!isLiveness(bb, ir, ir->res_)){
              dead_ret_function.insert(ir->opn1_.name_);
            }else if(dead_ret_function.find(ir->opn1_.name_) != dead_ret_function.end()){
              dead_ret_function.erase(ir->opn1_.name_);
            }
          }
        }
      }// for ir
    }// for bb
  }// for func

#ifdef PRINT_DEAD_RET_FUNCTION
  print_set(dead_ret_function);
#endif

  for(auto func : m->func_list_){
    if(dead_ret_function.find(func->name_) != dead_ret_function.end()){
      ir::gFuncTable[func->name_].ret_type_ = VOID;
      for(auto bb : func->bb_list_){
        for(int i = 0; i < bb->ir_list_.size(); ++i){
          auto ir = bb->ir_list_[i];

          if(ir->op_ == ir::IR::OpKind::RET){
            bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::RET);
          }
        }
      }
    }
  }
}


#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif