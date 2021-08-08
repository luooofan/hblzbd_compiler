#include "../include/DAG.h"
#include "../include/myassert.h"

DAG_node* createOpNode(ir::Opn *res, ir::IR::OpKind op){
  DAG_node* node = new DAG_node(DAG_node::Type::Op, op);
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
      node = new DAG_node(DAG_node::Type::Array, opn->name_, opn->scope_id_);
      DAG_node *offset_node = createOpnNode(opn->offset_);
      node->offset_ = offset_node;
      return node;
      break;
    default:
      break;
  }
}

// TODO: 1. 检查DAG中是否有相同的节点
//       2. 检查其他节点的定值变量表中是否有该变量，若有，则将该变量从那个定值变量表删除
//       3. 把从每个语句中创建的DAG节点加入到基本块中
//       4. 表达式中的数组元素
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
      
      bb->node_list_.push_back(res_node);
      bb->node_list_.push_back(opn1_node);
      bb->node_list_.push_back(opn2_node);
    }else if(ir->op_ == ir::IR::OpKind::NOT ||
             ir->op_ == ir::IR::OpKind::NEG){
      DAG_node *res_node = createOpNode(&(ir->res_), ir->op_);
      DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
      res_node->left_ = opn1_node;

      bb->node_list_.push_back(res_node);
      bb->node_list_.push_back(opn1_node);
    }else if(ir->op_ == ir::IR::OpKind::LABEL){
      // label语句只会出现在基本块的开头部分，所以对其的处理是在基本块中存储该基本块的label然后在重新生成四元式的时候再放到基本块的第一句就好了
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
        call_node->ir_list_.push_back(*(iter - i));
      }
      // 如果有返回值这需要将返回值加入到call节点的定值变量表中去
      if(ir->res_.type_ != ir::Opn::Type::Null){
        call_node->var_list_.insert(ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_));
      } 

      bb->node_list_.push_back(call_node);
    }else if(ir->op_ == ir::IR::OpKind::RET){
      DAG_node *ret_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::RET);
      DAG_node *ret_exp_node = nullptr;
      if(ir->opn1_.type_ != ir::Opn::Type::Null){
        ret_exp_node = createOpnNode(&(ir->opn1_));
      }
      ret_node->left_ = ret_exp_node;

      bb->node_list_.push_back(ret_node);
      bb->node_list_.push_back(ret_exp_node);
    }else if(ir->op_ == ir::IR::OpKind::GOTO){
      DAG_node *goto_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::GOTO);
      DAG_node *goto_label_node = createOpnNode(&(ir->opn1_));
      goto_node->left_ = goto_label_node;

      bb->node_list_.push_back(goto_node);
      bb->node_list_.push_back(goto_label_node);
    }else if(ir->op_ == ir::IR::OpKind::ASSIGN){
      DAG_node *assign_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::ASSIGN);
      assign_node->var_list_.insert(ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_));
      DAG_node *right_exp_node = createOpnNode(&(ir->opn1_));

      assign_node->left_ = right_exp_node;

      bb->node_list_.push_back(assign_node);
      bb->node_list_.push_back(right_exp_node);
    }else if(ir->op_ == ir::IR::OpKind::JEQ ||
             ir->op_ == ir::IR::OpKind::JNE ||
             ir->op_ == ir::IR::OpKind::JLT ||
             ir->op_ == ir::IR::OpKind::JLE ||
             ir->op_ == ir::IR::OpKind::JGT ||
             ir->op_ == ir::IR::OpKind::JGE){
    // 这些语句只会出现在基本块的最后一行，为其创建一个节点保存原来的ir语句
      DAG_node *jump_node = new DAG_node(DAG_node::Type::Op, ir->op_);
      jump_node->ir_list_.push_back(ir);

      bb->node_list_.push_back(jump_node);
    }else if(ir->op_ == ir::IR::OpKind::VOID){
    // 我们生成的四元式中应该没有VOID语句，这里在DAG节点中就不处理这个了  
    }else if(ir->op_ == ir::IR::OpKind::ASSIGN_OFFSET){
      // 数组赋值类型的节点
      DAG_node *array_assign = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::ASSIGN_OFFSET);
      array_assign->var_list_.insert(ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_));

      // left: array   mediate: offset   right: res
      DAG_node *array_node = createOpnNode(&(ir->opn1_));
      DAG_node *res_node = createOpnNode(&(ir->res_));
      array_assign->left_ = array_node;
      array_assign->right_ = res_node;

      bb->node_list_.push_back(array_assign);
      bb->node_list_.push_back(array_node);
      bb->node_list_.push_back(res_node);
    }else if(ir->op_ == ir::IR::OpKind::ALLOCA){
      // 这个语句可以为其生成一个节点，但重要的是重构的时候这个语句的位置
      // TODO：确定语句的位置
      DAG_node *alloca_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::ALLOCA);
      DAG_node *array_node = createOpnNode(&(ir->opn1_));
      DAG_node *res_node = createOpnNode(&(ir->res_));

      alloca_node->left_ = array_node;
      alloca_node->right_ = res_node;

      bb->node_list_.push_back(alloca_node);
      bb->node_list_.push_back(array_node);
      bb->node_list_.push_back(res_node);
    }else if(ir->op_ == ir::IR::OpKind::DECLARE){
      // 反正不会用该语句来优化什么
      // 对该语句的处理是：为其创造一个节点，然后将后面的几条打包进去
      DAG_node * declare_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::DECLARE);
      declare_node->ir_list_.push_back(ir);
      for(; iter!= bb->ir_list_.end() && (*iter)->op_ == ir::IR::OpKind::DECLARE; ++iter){
        declare_node->ir_list_.push_back(*iter);
      }
      --iter;

      bb->node_list_.push_back(declare_node);
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