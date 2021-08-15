#include "../../include/Pass/constant_propagation.h"
#include "../../include/Pass/reach_define.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"

void ConstantPropagation::Run(){
  auto m = dynamic_cast<IRModule *>(*m_);

  MyAssert(nullptr != m);
  
  bool changed = true;

  while(changed){
    ReachDefine reachdefine = ReachDefine(m_);
    reachdefine.Run();
    changed = false;
    for(auto func : m->func_list_){
      for(auto bb : func->bb_list_){
        for(int i = 0; i < bb->ir_list_.size(); ++i){
          auto ir = bb->ir_list_[i];
          auto map_this_ir = bb->use_def_chains_[i];

          auto process_opn = [&bb, &ir, &map_this_ir, &changed](ir::Opn *opn, int opn_num){
            if(opn->type_ == ir::Opn::Type::Var){
              auto opn_set = map_this_ir[opn];
              if(opn_set.size() == 1){
                auto ir_opn = *opn_set.begin();
                if(ir_opn->op_ == ir::IR::OpKind::ASSIGN && ir_opn->opn1_.type_ == ir::Opn::Type::Imm){
                  std::cout << "将变量: " << opn->name_ + "_#" + std::to_string(opn->scope_id_) << " 变为常数" << ir_opn->opn1_.imm_num_ << std::endl; 
                  changed = true;
                  if(opn_num == 1){
                    ir->opn1_ = ir_opn->opn1_;
                  }else if(opn_num == 2){
                    ir->opn2_ = ir_opn->opn1_;
                  }
                }
              }
            }
          };

          process_opn(&(ir->opn1_), 1);

          process_opn(&(ir->opn2_), 2);

          if(ir::IR::OpKind::ADD == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm && ir->opn2_.type_ == ir::Opn::Type::Imm){
              int res = ir->opn1_.imm_num_ + ir->opn2_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
              // if(changed) printf("/*******\n");
            }
          }else if(ir::IR::OpKind::SUB == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm && ir->opn2_.type_ == ir::Opn::Type::Imm){
              int res = ir->opn1_.imm_num_ - ir->opn2_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
            }            
          }else if(ir::IR::OpKind::MUL == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm && ir->opn2_.type_ == ir::Opn::Type::Imm){
              int res = ir->opn1_.imm_num_ * ir->opn2_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
            }
          }else if(ir::IR::OpKind::DIV == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm && ir->opn2_.type_ == ir::Opn::Type::Imm){
              // 没有加对除0的限制，应该没有必要
              int res = ir->opn1_.imm_num_ / ir->opn2_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
            }
          }else if(ir::IR::OpKind::MOD == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm && ir->opn2_.type_ == ir::Opn::Type::Imm){
              int res = ir->opn1_.imm_num_ % ir->opn2_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
            }
          }else if(ir::IR::OpKind::NOT == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm){
              int res = !ir->opn1_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
            }
          }else if(ir::IR::OpKind::NEG == ir->op_){
            if(ir->opn1_.type_ == ir::Opn::Type::Imm){
              int res = -ir->opn1_.imm_num_;
              ir::Opn opn1 = ir::Opn(ir::Opn::Type::Imm, res);
              ir::Opn res_opn = ir->res_;
              bb->ir_list_[i] = new ir::IR(ir::IR::OpKind::ASSIGN, opn1, res_opn);
              changed = true;
            }
          }
        }
      }
    }
    // if(changed) printf("/*******\n");
  }
}


#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif