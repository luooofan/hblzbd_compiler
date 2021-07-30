#include "../include/DAG.h"
#include "../include/myassert.h"

DAG_node* createOpNode(ir::Opn *opn){

}

DAG_node* createOpnNode(ir::Opn* opn){
  switch(opn->type_){
    case ir::Opn::Type::Var:
      DAG_node *node = new DAG_node(DAG_node::Type::Var, opn->name_, opn->scope_id_);
      return node;
      break;
    case ir::Opn::Type::Imm:
      DAG_node *node = new DAG_node(DAG_node::Type::Imm, opn->imm_num_);
      return node;
      break;
    case ir::Opn::Type::Label:
      DAG_node *node = new DAG_node(DAG_node::Type::Label, opn->name_);
      return node;
      break;
    case ir::Opn::Type::Func:
      DAG_node *node = new DAG_node(DAG_node::Type::Func, opn->name_);
      return node;
      break;
    case ir::Opn::Type::Null:
      DAG_node *node = new DAG_node(DAG_node::Type::Null);
      return node;
      break;
    case ir::Opn::Type::Array:
      break;
    default:
      break;
  }
}

//对每一条指令：查找是否有相同的节点，将变量加入到那个节点的定值变量表中
//如果没有则创造新的节点，并将该变量在之前的定制变量表中删除（如果有的话）
void DAG_analysis::Run4bb(IRBasicBlock *bb){
  for(auto ir : bb->ir_list_){
    if(ir->op_ == ir::IR::OpKind::ADD ||
       ir->op_ == ir::IR::OpKind::SUB ||
       ir->op_ == ir::IR::OpKind::MUL ||
       ir->op_ == ir::IR::OpKind::DIV ||
       ir->op_ == ir::IR::OpKind::MOD ){
      //TODO检查DAG中是否有相同的节点

    }else if(ir->op_ == ir::IR::OpKind::NOT ||
             ir->op_ == ir::IR::OpKind::NEG){
      
    }else if(ir->op_ == ir::IR::OpKind::LABEL){

    }else if(ir->op_ == ir::IR::OpKind::PARAM){

    }else if(ir->op_ == ir::IR::OpKind::CALL){

    }else if(ir->op_ == ir::IR::OpKind::RET){

    }else if(ir->op_ == ir::IR::OpKind::GOTO){

    }else if(ir->op_ == ir::IR::OpKind::ASSIGN){

    }else if(ir->op_ == ir::IR::OpKind::JEQ ||
             ir->op_ == ir::IR::OpKind::JNE ||
             ir->op_ == ir::IR::OpKind::JLT ||
             ir->op_ == ir::IR::OpKind::JLE ||
             ir->op_ == ir::IR::OpKind::JGT ||
             ir->op_ == ir::IR::OpKind::JGE){

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