#include "../include/DAG.h"
#include "../include/myassert.h"

DAG_node* createOpNode(ir::Opn *res, ir::IR::OpKind op){
  DAG_node* node = new DAG_node(DAG_node::Type::Op, op, nullptr, nullptr);
  node -> var_list_.insert(res->name_ + "_#" + std::to_string(res->scope_id_));

  return node;
}

DAG_node* createOpnNode(ir::Opn* opn){
  DAG_node *node;
  switch(opn->type_){
    case ir::Opn::Type::Var:
      node = new DAG_node(DAG_node::Type::Var, opn->name_, opn->scope_id_);
      return node;
      break;
    case ir::Opn::Type::Imm:
      node = new DAG_node(DAG_node::Type::Imm, opn->imm_num_);
      return node;
      break;
    case ir::Opn::Type::Label:
      node = new DAG_node(DAG_node::Type::Label, opn->name_);
      return node;
      break;
    case ir::Opn::Type::Func:
      node = new DAG_node(DAG_node::Type::Func, opn->name_);
      return node;
      break;
    case ir::Opn::Type::Null:
      node = new DAG_node(DAG_node::Type::Null);
      return node;
      break;
    case ir::Opn::Type::Array:
      break;
    default:
      break;
  }
}

// TODO: 1. 检查DAG中是否有相同的节点
//       2. 检查其他节点的定值变量表中是否有该变量，若有，则将该变量从那个定值变量表删除
//       3. 
void DAG_analysis::Run4bb(IRBasicBlock *bb){
  for(auto iter = bb->ir_list_.begin(); iter != bb->ir_list_.end(); ++iter){
    auto ir = *iter;
    if(ir->op_ == ir::IR::OpKind::ADD ||
       ir->op_ == ir::IR::OpKind::SUB ||
       ir->op_ == ir::IR::OpKind::MUL ||
       ir->op_ == ir::IR::OpKind::DIV ||
       ir->op_ == ir::IR::OpKind::MOD ){
      //TODO检查DAG中是否有相同的节点
      DAG_node *res_node = createOpNode(&(ir->res_), ir->op_);
      DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
      DAG_node *opn2_node = createOpnNode(&(ir->opn2_));
      res_node->left_ = opn1_node;
      res_node->right_ = opn2_node;
    }else if(ir->op_ == ir::IR::OpKind::NOT ||
             ir->op_ == ir::IR::OpKind::NEG){
      DAG_node *res_node = createOpNode(&(ir->res_), ir->op_);
      DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
      res_node->left_ = opn1_node;
    }else if(ir->op_ == ir::IR::OpKind::LABEL){
      if(ir->opn1_.type_ == ir::Opn::Type::Label){
        bb->bb_label_ = ir->opn1_.name_ + "_#label";
      }else if(ir->opn1_.type_ == ir::Opn::Type::Func){
        bb->bb_label_ = ir->opn1_.name_ + "_#Func";
      }else{
        MyAssert(255);
      }
    }else if(ir->op_ == ir::IR::OpKind::PARAM){
    // 这里对param语句和call语句的处理办法是，将他们打包成一个语句块存起来
    // 但只对call语句生成DAG的节点，如果后面没有用到这些东西，直接把这个语句块删了就行
      continue;
    }else if(ir->op_ == ir::IR::OpKind::CALL){
      DAG_node *call_node = new DAG_node(DAG_node::Type::Op, ir->opn1_.name_, ir::IR::OpKind::CALL, ir->opn2_.imm_num_);
      // 处理call节点的参数传递和函数调用语句块
      for(int i = 0; i < ir->opn2_.imm_num_; ++i){
        call_node->param_and_call_.push_back(*(iter - i));
      }
      // 如果有返回值这需要将返回值加入到call节点的定值变量表中去
      if(ir->res_.type_ != ir::Opn::Type::Null){
        call_node->var_list_.insert(ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_));
      } 
    }else if(ir->op_ == ir::IR::OpKind::RET){
      DAG_node *ret_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::RET, nullptr, nullptr);
      DAG_node *ret_exp_node = nullptr;
      if(ir->opn1_.type_ != ir::Opn::Type::Null){
        ret_exp_node = createOpnNode(&(ir->opn1_));
      }
      ret_node->left_ = ret_exp_node;
    }else if(ir->op_ == ir::IR::OpKind::GOTO){
      DAG_node *goto_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::GOTO, nullptr, nullptr);
      DAG_node *goto_label_node = createOpnNode(&(ir->opn1_));
      goto_node->left_ = goto_label_node;
    }else if(ir->op_ == ir::IR::OpKind::ASSIGN){
      DAG_node *assign_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::ASSIGN, nullptr, nullptr);
      // 处理赋值语句的右值表达式
      assign_node->name_ = ir->opn1_.name_;
      assign_node->scope_id_=ir->opn1_.scope_id_;

      // 处理赋值语句的定值表达式
      assign_node->var_list_.insert(ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_));
    }else if(ir->op_ == ir::IR::OpKind::JEQ ||
             ir->op_ == ir::IR::OpKind::JNE ||
             ir->op_ == ir::IR::OpKind::JLT ||
             ir->op_ == ir::IR::OpKind::JLE ||
             ir->op_ == ir::IR::OpKind::JGT ||
             ir->op_ == ir::IR::OpKind::JGE){
    // 这些语句只会出现在基本块的最后一行，
    }else if(ir->op_ == ir::IR::OpKind::VOID){

    }else if(ir->op_ == ir::IR::OpKind::ASSIGN_OFFSET){

    }
  }
}

void DAG_analysis::Run(){
  auto m = dynamic_cast<IRModule *>(*(this->m_));

  MyAssert(nullptr != m);

  for(auto func : m->func_list_){
    for(auto bb : func->bb_list_){
      Run4bb(bb);
    }
  }
}