#include "../include/DAG.h"
#include "../include/myassert.h"

DAG_node* createOpNode(ir::Opn *res, ir::IR::OpKind op){
  DAG_node* node = new DAG_node(DAG_node::Type::Op, op);
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

bool isSameOpn(ir::Opn opn, DAG_node *node){
  if(opn.type_ == ir::Opn::Type::Var && node->type_ == DAG_node::Type::Var){
    if(opn.name_ == node->name_ && opn.scope_id_ == node->scope_id_){
      return true;
    }
  }else if(opn.type_ == ir::Opn::Type::Imm && node->type_ == DAG_node::Type::Imm){
    if(opn.imm_num_ == node ->imm_num_){
      return true;
    }
  }else if(opn.type_ == ir::Opn::Type::Array && node->type_ == DAG_node::Type::Array){
    std::cerr << "关于数组的部分尚未完成\n";
  }else if(opn.type_ == ir::Opn::Type::Func && node->type_ == DAG_node::Type::Func){
    if(opn.name_ == node->name_){
      return true;
    }
  }else if(opn.type_ == ir::Opn::Type::Label && node->type_ == DAG_node::Type::Label){
    if(opn.name_ == node->name_){
      return true;
    }
  }else if(opn.type_ == ir::Opn::Type::Null && node->type_ == DAG_node::Type::Null){
    std::cout << "我就不信你能生成null类型的DAG_node\n";
  }

  return false;
}

DAG_node* DAG_analysis::FindSameOpNode(IRBasicBlock *bb, ir::IR *ir){
  for(auto node : bb->node_list_){
    if(node->type_ == DAG_node::Type::Op){
      if(node->op_ == ir->op_){
        auto left = node->left_;
        auto right = node->right_;
        auto mediate = node->mediate_;

        if(ir::Opn::Type::Null != ir->opn1_.type_ && left != nullptr){
          if(isSameOpn(ir->opn1_, left)){
            if(ir::Opn::Type::Null != ir->opn2_.type_ && nullptr != right){
              if(isSameOpn(ir->opn2_, right)){
                return node;
              }
            }
          }
        }//if left
      }
    }
  }
  return nullptr;
}

DAG_node* DAG_analysis::FindSameOpnNode(IRBasicBlock *bb, ir::Opn *opn){
  for(auto node : bb->node_list_){
    if(node->type_ != DAG_node::Type::Op){
      if(isSameOpn(*opn, node)){
        return node;
      }
    }
  }
  return nullptr;
}

void DAG_analysis::DeleteVar(IRBasicBlock *bb, std::string var){
  for(auto node : bb->node_list_){
    auto iter = node->var_list_.find(var);
    if(iter != node->var_list_.end()){
      node->var_list_.erase(iter);
    }
  }
}

// TODO: 1. 检查DAG中是否有相同的节点
//       2. 检查其他节点的定值变量表中是否有该变量，若有，则将该变量从那个定值变量表删除
//       3. 把从每个语句中创建的DAG节点加入到基本块中
//       4. 表达式中的数组元素
// 如果要将某个变量加入到定值变量表中，则必然要先将其从其他定值变量表中删除
void DAG_analysis::Run4bb(IRBasicBlock *bb){
  for(auto iter = bb->ir_list_.begin(); iter != bb->ir_list_.end(); ++iter){
    auto ir = *iter;
    if(ir->op_ == ir::IR::OpKind::ADD ||
       ir->op_ == ir::IR::OpKind::SUB ||
       ir->op_ == ir::IR::OpKind::MUL ||
       ir->op_ == ir::IR::OpKind::DIV ||
       ir->op_ == ir::IR::OpKind::MOD ){
      DAG_node *same_node = FindSameOpNode(bb, ir);
      std::string res_var = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);
      std::string opn1_var = ir->opn1_.name_ + "_#" + std::to_string(ir->opn1_.scope_id_);
      std::string opn2_var = ir->opn2_.name_ + "_#" + std::to_string(ir->opn2_.scope_id_);

      if(nullptr != same_node){//如果DAG图中有完全一样的节点，则直接将res加到该节点的定值变量表中
        DeleteVar(bb, res_var);
        same_node->var_list_.insert(res_var);
      }else{//如果没有完全一样的节点，则需要新创一个并把res从其他定值变量表中删除
        DAG_node* node = new DAG_node(DAG_node::Type::Op, ir->op_);
        DeleteVar(bb, res_var);
        node -> var_list_.insert(res_var);
        bb->node_list_.push_back(node);// 将新的节点加入到基本块的节点列表中

        DAG_node *same_opn1_node = FindSameOpnNode(bb, &(ir->opn1_));
        if(nullptr != same_opn1_node){
          node->left_ = same_opn1_node;
        }else{
          DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
          DeleteVar(bb, opn1_var);
          opn1_node->var_list_.insert(opn1_var);

          node->left_ = opn1_node;
          bb->node_list_.push_back(opn1_node);
        }

        DAG_node *same_opn2_node = FindSameOpnNode(bb, &(ir->opn2_));
        if(nullptr != same_opn2_node){
          node->right_ = same_opn2_node;
        }else{
          DAG_node *opn2_node = createOpnNode(&(ir->opn2_));
          DeleteVar(bb, opn2_var);
          opn2_node->var_list_.insert(opn2_var);

          node->left_ = opn2_node;
          bb->node_list_.push_back(opn2_node);
        }
      }
    }else if(ir->op_ == ir::IR::OpKind::NOT ||
             ir->op_ == ir::IR::OpKind::NEG){
      DAG_node *same_node = FindSameOpNode(bb, ir);
      std::string res_var = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);
      std::string opn1_var = ir->opn1_.name_ + "_#" + std::to_string(ir->opn1_.scope_id_);

      if(nullptr != same_node){//如果DAG图中有完全一样的节点，则直接将res加到该节点的定值变量表中
        DeleteVar(bb, res_var);
        same_node->var_list_.insert(res_var);
      }else{//如果没有完全一样的节点，则需要新创一个并把res从其他定值变量表中删除
        DAG_node* node = new DAG_node(DAG_node::Type::Op, ir->op_);
        DeleteVar(bb, res_var);
        node -> var_list_.insert(res_var);
        bb->node_list_.push_back(node);// 将新的节点加入到基本块的节点列表中

        DAG_node *same_opn1_node = FindSameOpnNode(bb, &(ir->opn1_));
        if(nullptr != same_opn1_node){
          node->left_ = same_opn1_node;
        }else{
          DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
          DeleteVar(bb, opn1_var);
          opn1_node->var_list_.insert(opn1_var);

          node->left_ = opn1_node;
          bb->node_list_.push_back(opn1_node);
        }
      }
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
      DAG_node *ret_exp_node = FindSameOpnNode(bb, &(ir->opn1_));
      if(ir->opn1_.type_ != ir::Opn::Type::Null){
        if(ret_exp_node != nullptr){
          ret_node->left_ = ret_exp_node;
        }else{
          ret_exp_node = createOpnNode(&(ir->opn1_));
          ret_node->left_ = ret_exp_node;
          bb->node_list_.push_back(ret_exp_node);
        }  
      }
      bb->node_list_.push_back(ret_node);
    }else if(ir->op_ == ir::IR::OpKind::GOTO){
      DAG_node *goto_node = new DAG_node(DAG_node::Type::Op, ir::IR::OpKind::GOTO);
      DAG_node *goto_label_node = createOpnNode(&(ir->opn1_));
      goto_node->left_ = goto_label_node;

      bb->node_list_.push_back(goto_node);
      bb->node_list_.push_back(goto_label_node);
    }else if(ir->op_ == ir::IR::OpKind::ASSIGN){
      DAG_node *same_node = FindSameOpNode(bb, ir);
      std::string res_var = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);
      std::string opn1_var = ir->opn1_.name_ + "_#" + std::to_string(ir->opn1_.scope_id_);

      if(nullptr != same_node){
        DeleteVar(bb, res_var);
        same_node->var_list_.insert(res_var);
      }else{
        DAG_node *node = new DAG_node(DAG_node::Type::Op, ir->op_);
        DeleteVar(bb, res_var);
        node->var_list_.insert(res_var);
        bb->node_list_.push_back(node);

        DAG_node *same_opn1_node = FindSameOpnNode(bb, &(ir->opn1_));
        if(nullptr != same_opn1_node){
          node->left_ = same_opn1_node;
        }else{
          DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
          DeleteVar(bb, opn1_var);
          opn1_node->var_list_.insert(opn1_var);

          node->left_ = opn1_node;
          bb->node_list_.push_back(opn1_node);
        }
      }
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
      DAG_node *same_node = FindSameOpNode(bb, ir);
      std::string res_var = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);
      std::string opn1_var = ir->opn1_.name_ + "_#" + std::to_string(ir->opn1_.scope_id_);

      if(nullptr != same_node){
        DeleteVar(bb, res_var);
        same_node->var_list_.insert(res_var);
      }else{
        DAG_node *node = new DAG_node(DAG_node::Type::Op, ir->op_);
        DeleteVar(bb, res_var);
        node->var_list_.insert(res_var);
        bb->node_list_.push_back(node);

        DAG_node *same_opn1_node = FindSameOpnNode(bb, &(ir->opn1_));
        if(nullptr != same_opn1_node){
          node->left_ = same_opn1_node;
        }else{
          DAG_node *opn1_node = createOpnNode(&(ir->opn1_));
          DeleteVar(bb, opn1_var);
          opn1_node->var_list_.insert(opn1_var);

          node->left_ = opn1_node;
          bb->node_list_.push_back(opn1_node);
        }
      }
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