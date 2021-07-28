#include "../../include/Pass/ir_liveness_analysis.h"

#include <cassert>
#define ASSERT_ENABLE
// assert(res);
#ifdef ASSERT_ENABLE
#define MyAssert(res)                                                    \
  if (!(res)) {                                                          \
    std::cerr << "Assert: " << __FILE__ << " " << __LINE__ << std::endl; \
    exit(255);                                                           \
  }
#else
#define MyAssert(res) ;
#endif

std::pair<std::vector<Opn*>, std::vector<Opn*>> GetDefUse(IR *ir)
{
  std::vector<Opn*> def;
  std::vector<Opn*> use;

  auto process_opn = [&use](Opn *op){
    if(nullptr == op) return;
    if(op->type_ == Opn::Type::Imm || op->type_ == Opn::Type::Label || op->type_ == Opn::Type::Func || op->type_ == Opn::Type::Null) return;
    use.push_back(op);
    if(op->type_ == Opn::Type::Array && op->offset_->type_ == Opn::Type::Var){
      use.push_back(op->offset_);
    }
  };

  auto process_res = [&def, &use](Opn *res){
    if(nullptr == res) return;
    if(res->type_ == Opn::Type::Label || res->type_ == Opn::Type::Func || res->type_ == Opn::Type::Null) return;
    def.push_back(res);
    if(res->type_ == Opn::Type::Array){
      if(res->offset_->type_ == Opn::Type::Imm || res->offset_->type_ == Opn::Type::Label || res->offset_->type_ == Opn::Type::Func || res->offset_->type_ == Opn::Type::Null) return;
      use.push_back(res->offset_);
    }
  };

  if(ir->op_ == IR::OpKind::ADD ||
     ir->op_ == IR::OpKind::SUB ||
     ir->op_ == IR::OpKind::MUL ||
     ir->op_ == IR::OpKind::DIV ||
     ir->op_ == IR::OpKind::MOD){
       process_opn(&(ir->opn1_));
       process_opn(&(ir->opn2_));
       process_res(&(ir->res_));
     }else if(ir->op_ == IR::OpKind::NOT ||
              ir->op_ == IR::OpKind::NEG){
        process_opn(&(ir->opn1_));
        process_res(&(ir->res_));
      }else if(ir->op_ == IR::OpKind::LABEL){
        return;
      }else if(ir->op_ == IR::OpKind::PARAM){
        process_opn(&(ir->opn1_));
      }else if(ir->op_ == IR::OpKind::CALL){
        process_res(&(ir->res_));
      }else if(ir->op_ == IR::OpKind::RET){
        process_opn(&(ir->opn1_));
      }else if(ir->op_ == IR::OpKind::GOTO){
        return;
      }else if(ir->op_ == IR::OpKind::ASSIGN){
        process_opn(&(ir->opn1_));
        process_res(&(ir->res_));
      }else if(ir->op_ == IR::OpKind::JEQ ||
               ir->op_ == IR::OpKind::JNE ||
               ir->op_ == IR::OpKind::JLT ||
               ir->op_ == IR::OpKind::JLE ||
               ir->op_ == IR::OpKind::JGT ||
               ir->op_ == IR::OpKind::JGE){
        process_opn(&(ir->opn1_));
        process_opn(&(ir->opn2_));
      }else if(ir->op_ == IR::OpKind::VOID){
        return;
      }else if(ir->op_ == IR::OpKind::ASSIGN_OFFSET){
        process_opn(&(ir->opn1_));
        process_res(&(ir->res_));
      }else{
        MyAssert(0);
      }
  
  return {def, use};
}


void IRLivenessAnalysis::Run4Func(IRFunction *f){
  for(auto bb : f -> bb_list_){
    bb -> livein_.clear();
    bb -> liveout_.clear();
    bb -> def_.clear();
    bb -> use_.clear();

    for(auto ir : bb -> ir_list_){
      auto [def, use] = GetDefUse(ir);

      for(auto &u : use){
        if(bb -> def_.find(u) == bb -> def_.end()){// 如果在use之前没有def，则将其插入use中，说明该变量在bb中首次出现是以use的形式出现
          bb -> use_.insert(u);
        }
      }// for use

      for(auto &d : def){
        if(bb -> use_.find(d) == bb -> use_.end()){// 如果在def之前没有use，则将其插入def中，说明该变量首次出现是以def形式出现
          bb -> def_.insert(d);
        }
      }// for def
    }// for ir
    bb -> livein_ = bb -> use_;
  }// for bb


  bool changed = true;
  while(changed){
    changed = false;
    for(auto bb : f -> bb_list_){
      std::unordered_set<Opn*> new_out;

      for(auto succ : bb -> succ_){// 每个bb的liveout是其后继bb的livein的并集
        new_out.insert(succ -> livein_.begin(), succ -> livein.end());
      }

      if(new_out != bb -> liveout_){
        changed = true;
        bb -> liveout_ = new_out;

        for(auto s : bb -> liveout_){//IN[B] = use[B]U(out[B] - def[B])
          if(bb -> def_.find(s) == bb -> def_.end()){
            bb -> livein_.insert(s);
          }
        }
      }
    }// for bb
  }// while changed
}


void IRLivenessAnalysis::Run()
{
  auto m = dynamic_cast<IRModule *>(*(this->_m));

  MyAssert(nullptr != m);

  for(auto func : m -> func_list_)
  {
    this -> Run4Func(func);
  }
}

#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif