#include <cstdio>
#include <stdexcept>

#include "../include/ast.h"
#include "../include/ir.h"
#include "parser.hpp"  // VOID INT

#define IMM_0_OPN \
  { Opn::Type::Imm, 0 }
#define IMM_1_OPN \
  { Opn::Type::Imm, 1 }
#define LABEL_OPN(label_name) \
  { Opn::Type::Label, label_name }

namespace ast {

void Root::GenerateIR() {
  // TODO 未完成
  //创建一张全局符号表
  gSymbolTables.push_back({0, -1});
  gContextInfo.current_scope_id_ = 0;
  for (const auto &ele : this->compunit_list_) {
    ele->GenerateIR();
    // std::cout << "OK" << std::endl;
  }
  // 检查是否有main函数，main函数的返回值是否是int
  std::string sss="main";
  auto func_iter = gFuncTable.find(sss);
  if(func_iter==gFuncTable.end())
  {
    std::cout<<"没有找到一个函数名为main的函数\n";
  }
  else
  {
    int ret_type = (*func_iter).second.ret_type_;
    if(ret_type==VOID)
    {
      std::cout<<"main函数的返回值应该为int\n";
    }
  }
}
void Number::GenerateIR() {
  //生成一个opn 维护shape 不生成目标代码
  Opn opn = Opn(Opn::Type::Imm, value_);
  gContextInfo.opn_ = opn;
  gContextInfo.shape_.clear();
}

void Identifier::GenerateIR() {
  SymbolTableItem *s;
  //如果是函数名就不需要查变量表
  if (!gContextInfo.is_func_) {
    s = FindSymbol(gContextInfo.current_scope_id_, name_);
    if (!s) {
      SemanticError(line_no_, name_ + ": undefined variable");
    }
  }

  gContextInfo.shape_ = s->shape_;
  Opn opn = Opn(Opn::Type::Var, name_, gContextInfo.current_scope_id_);
  gContextInfo.opn_ = opn;
}


// 这里有个点需要注意：如果是普通情况下取数组元素则shape_list_中的维度必须与符号表中维度相同，
// 但如果是函数调用中使用数组则可以传递指针shape_list_维度小于符号表中的维度即可
// ？？？语义检查时是否区分这两种情况
// 下面的实现按照区分两种情况来写
void ArrayIdentifier::GenerateIR() {
  SymbolTableItem *s;
  s = FindSymbol(gContextInfo.current_scope_id_, name_.name_);
  if (!s) {
    SemanticError(line_no_, name_.name_ + ": undefined variable");
  }
  if(gContextInfo.is_func_)
  {
    if (shape_list_.size() > s->shape_.size()) {
      SemanticError(line_no_,
                    name_.name_ + ": the dimension of array is not correct");
    }
  }
  else
  {
    if (shape_list_.size() != s->shape_.size()) {
      SemanticError(line_no_,
                    name_.name_ + ": the dimension of array is not correct");
    }   
  }

  gContextInfo.shape_ =
      std::vector<int>(s->shape_.begin() + shape_list_.size(), s->shape_.end());

  // for(int i = 0; i < s->width_.size(); ++i)
  // {
  //   printf("width: %d\n", s->width_[i]);
  // }

  BinaryExpression *res = (BinaryExpression*)(new Number(0, 0));
  BinaryExpression *add_exp;
  BinaryExpression *mul_exp;
  // 计算数组取值的偏移
  for(int i = 0; i < shape_list_.size(); ++i)
  {
    // shape_list_[i]->GenerateIR();

    // // 计算当前维度偏移
    // std::string mul_temp = NewTemp();
    // gSymbolTables[gContextInfo.current_scope_id_].symbol_table_.insert(
    //     {mul_temp,
    //     {false, false, gSymbolTables[gContextInfo.current_scope_id_].size_}});
    // gSymbolTables[gContextInfo.current_scope_id_].size_ += kIntWidth;   
    // Opn mul_opn = Opn(Opn::Type::Var, nul_temp, gContextInfo.current_scope_id_);
    // IR mul_ir = IR(IR::OpKind::MUL, gContextInfo.opn_, Opn(Opn::Type::Imm, s->width_[i]), mul_opn);
    // gIRList.push_back(mul_ir);
    
    // //加到总偏移中
    // std::string add_temp = NewTemp();
    // gSymbolTables[gContextInfo.current_scope_id_].symbol_table_.insert(
    //     {add_temp,
    //     {false, false, gSymbolTables[gContextInfo.current_scope_id_].size_}});
    // gSymbolTables[gContextInfo.current_scope_id_].size_ += kIntWidth;   
    // Opn add_opn = Opn(Opn::Type::Var,)

    //构造一个二元表达式
    Number *width = new Number(0, s->width_[i + 1]);
    // printf("width: %d\n", s->width_[i + 1]);
    mul_exp = new BinaryExpression(0, MUL, *width, *shape_list_[i]);
    add_exp = new BinaryExpression(0, ADD, *mul_exp, *res);
    res = add_exp;
    if(i == shape_list_.size() - 1)
    {
      res -> GenerateIR();
      // res -> PrintNode();
    }
  }
  // a[b+1]
  // gContextInfo.array_offset_ = index;
  // if(gContextInfo.opn_.type_ == Opn::Type::Imm) printf("******************************%d", gContextInfo.opn_.imm_num_);
  Opn *offset = new Opn();
  offset -> type_ = gContextInfo.opn_.type_;
  offset -> imm_num_ = gContextInfo.opn_.imm_num_;
  offset -> name_ = gContextInfo.opn_.name_;
  offset -> scope_id_ = gContextInfo.opn_.scope_id_;
  offset -> offset_ = gContextInfo.opn_.offset_;
  Opn opn = Opn(Opn::Type::Array, name_.name_, gContextInfo.current_scope_id_, offset);
  gContextInfo.opn_ = opn;
}

void BinaryExpression::GenerateIR() {
  Opn opn1, opn2, temp;
  IR::OpKind op;

  lhs_.GenerateIR();
  opn1 = gContextInfo.opn_;
  if (!gContextInfo.shape_.empty()) {
    SemanticError(this->line_no_,
                  gContextInfo.opn_.name_ + ": lhs exp type not int");
  }
  rhs_.GenerateIR();
  opn2 = gContextInfo.opn_;
  if (!gContextInfo.shape_.empty()) {
    SemanticError(this->line_no_,
                  gContextInfo.opn_.name_ + ": rhs exp type not int");
  }

  switch (op_) {
    case ADD:
      op = IR::OpKind::ADD;
      break;
    case SUB:
      op = IR::OpKind::SUB;
      break;
    case MUL:
      op = IR::OpKind::MUL;
      break;
    case DIV:
      op = IR::OpKind::DIV;
      break;
    case MOD:
      op = IR::OpKind::MOD;
      break;
    default:
      printf("mystery operator code: %d", op_);
      break;
  }

  if(opn1.type_ == Opn::Type::Imm && opn2.type_ == Opn::Type::Imm)
  {
    int result;
    int num1 = opn1.imm_num_, num2 = opn2.imm_num_;
    switch (op_) 
    {
      case ADD:
        result = num1 + num2;
        break;
      case SUB:
        result = num1 - num2;
        break;
      case MUL:
        result = num1 * num2;
        break;
      case DIV:
        result = num1 / num2;
        break;
      case MOD:
        result = num1 % num2;
        break;
      default:
        printf("mystery operator code: %d", op_);
        break;
    }
    temp = Opn(Opn::Type::Imm, result);
    gContextInfo.opn_ = temp;
  }
  else
  {
    std::string res_temp_var = NewTemp();
    gSymbolTables[gContextInfo.current_scope_id_].symbol_table_.insert(
        {res_temp_var,
        {false, false, gSymbolTables[gContextInfo.current_scope_id_].size_}});
    gSymbolTables[gContextInfo.current_scope_id_].size_ += kIntWidth;
    temp = Opn(Opn::Type::Var, res_temp_var, gContextInfo.current_scope_id_);
    gContextInfo.opn_ = temp;
    IR ir = IR(op, opn1, opn2, temp);
    gIRList.push_back(ir);
  }

  
}

void UnaryExpression::GenerateIR() {
  Opn opn1, temp;
  IR::OpKind op;

  rhs_.GenerateIR();
  opn1 = gContextInfo.opn_;
  if (!gContextInfo.shape_.empty()) {
    SemanticError(this->line_no_,
                  gContextInfo.opn_.name_ + ": rhs exp type not int");
  }

  switch (op_) {
    case ADD:
      op = IR::OpKind::POS;
      break;
    case SUB:
      op = IR::OpKind::NEG;
      break;
    case NOT:
      op = IR::OpKind::NOT;
      break;
    default:
      printf("mystery operator code: %d", op_);
      break;
  }

  if(opn1.type_ == Opn::Type::Imm)
  {
    int result;
    int num1 = opn1.imm_num_;
    switch (op_) 
    {
      case ADD:
        result = num1;
        break;
      case SUB:
        result = -num1;
        break;
      case NOT:
        result = !num1;
        break;
      default:
        printf("mystery operator code: %d", op_);
        break;
    }
    temp = Opn(Opn::Type::Imm, result);
  }
  else
  {
    auto scope_id = gContextInfo.current_scope_id_;
    std::string rhs_temp_var = NewTemp();
    gSymbolTables[scope_id].symbol_table_.insert(
        {rhs_temp_var, {false, false, gSymbolTables[scope_id].size_}});
    gSymbolTables[scope_id].size_ += kIntWidth;
    Opn temp = Opn(Opn::Type::Var, rhs_temp_var, gContextInfo.current_scope_id_);
    gContextInfo.opn_ = temp;
  }

  IR ir = IR(op, opn1, temp);
  gIRList.push_back(ir);
}


void FunctionCall::GenerateIR() {
  Opn opn1, opn2;
  if (gFuncTable.find(name_.name_) == gFuncTable.end()) {
    SemanticError(line_no_, "调用的函数不存在");
    return;
  }
  if(gFuncTable[name_.name_].shape_list_.size()!=args_.arg_list_.size()){
    SemanticError(line_no_,"参数个数不匹配");
    return ;
  }
  int i = 0;
  for (auto &arg : args_.arg_list_) {
    arg->GenerateIR();
    // mxd把下面的改了，下面注释的是原代码，改成了if else
    // if (gContextInfo.shape_ == gFuncTable[name_.name_].shape_list_[i++]) {
    //   SemanticError(line_no_, "函数调用参数不正确");
    //   return;
    // }
    if(gContextInfo.shape_.size()!=gFuncTable[name_.name_].shape_list_[i].size())
    {
      std::cout<<gContextInfo.shape_.size()<<' '<<gFuncTable[name_.name_].shape_list_[i].size()<<'\n';
      SemanticError(line_no_, "函数调用参数类型不匹配");
      return;
    }
    else
    {
      for(int j=1;j<gContextInfo.shape_.size();j++)
      {
        if(gContextInfo.shape_[j]!=gFuncTable[name_.name_].shape_list_[i][j])
        {
          SemanticError(line_no_, "函数调用参数类型不匹配");
          return;
        }
      }
    }

    SymbolTableItem *s;
    //如果是函数名就不需要查变量表
    s = FindSymbol(gContextInfo.current_scope_id_, gContextInfo.opn_.name_);
    if (s) {
      if(s->is_array_ && s->is_const_)
      {
        SemanticError(line_no_, "实参不能是const数组");
        return;
      }
    }

    opn1 = gContextInfo.opn_;

    IR ir = IR(IR::OpKind::PARAM, opn1);
    gIRList.push_back(ir);
    i++;
  }

  gContextInfo.is_func_ = !gContextInfo.is_func_;
  name_.GenerateIR();
  opn1 = gContextInfo.opn_;
  gContextInfo.is_func_ = !gContextInfo.is_func_;
  opn2 = Opn(Opn::Type::Imm, gFuncTable[name_.name_].shape_list_.size());

  IR ir;
  if(gFuncTable[name_.name_].ret_type_ == INT)
  {
    auto scope_id = gContextInfo.current_scope_id_;
    std::string ret_var = NewTemp();
    gSymbolTables[scope_id].symbol_table_.insert(
        {ret_var, {false, false, gSymbolTables[scope_id].size_}});
    gSymbolTables[scope_id].size_ += kIntWidth;
    Opn temp = Opn(Opn::Type::Var, ret_var, gContextInfo.current_scope_id_);
    gContextInfo.opn_ = temp;
    gContextInfo.shape_.clear();
    ir = IR(IR::OpKind::CALL, opn1, opn2, temp);
  }
  else
  {
    Opn temp = Opn(Opn::Type::Null);
    gContextInfo.opn_ = temp;
    ir = IR(IR::OpKind::CALL, opn1, opn2, temp);
  }

  gIRList.push_back(ir);
}

void VariableDefine::GenerateIR() {
  auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_);
  // 得往上递归地找
  // int scope_id=gContextInfo.current_scope_id_;
  // SymbolTable scope;
  // std::unordered_map<std::string,SymbolTableItem> symbol_table;
  // while(scope_id!=-1)
  // {
  //   scope = gSymbolTables[gContextInfo.current_scope_id_];
  //   symbol_table = scope.symbol_table_;
  //   const auto &var_iter = symbol_table.find(this->name_.name_);
  //   if(var_iter == symbol_table.end())scope_id=scope.parent_scope_id_;
  // }
  if (var_iter==symbol_table.end()) {
    auto tmp = new SymbolTableItem(false, false, scope.size_);
    if (gContextInfo.current_scope_id_ == 0) tmp->initval_.push_back(0);
    symbol_table.insert({this->name_.name_, *tmp});
    scope.size_ += kIntWidth;
  } else {
    SemanticError(this->line_no_, this->name_.name_ + ": variable redefined");
  }
}
void VariableDefineWithInit::GenerateIR() {
  auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_);
  if (var_iter == symbol_table.end()) {
    auto tmp = new SymbolTableItem(false, is_const_, scope.size_);
    scope.size_ += kIntWidth;
    if (gContextInfo.current_scope_id_ == 0 || is_const_) {
      value_.Evaluate();
      tmp->initval_.push_back(gContextInfo.opn_.imm_num_);
    } else {
      value_.GenerateIR();
      Opn lhs_opn = Opn(Opn::Type::Var, name_.name_, gContextInfo.current_scope_id_);
      gIRList.push_back({IR::OpKind::ASSIGN, gContextInfo.opn_, lhs_opn});
    }
    symbol_table.insert({this->name_.name_, *tmp});
  } else {
    SemanticError(this->line_no_, this->name_.name_ + ": variable redefined");
  }
}
void ArrayDefine::GenerateIR() {
  auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_.name_);
  if (var_iter == symbol_table.end()) {
    auto tmp = new SymbolTableItem(true, false, scope.size_);
    //计算shape和width
    for (const auto &shape : name_.shape_list_) {
      shape->Evaluate();
      tmp->shape_.push_back(
          gContextInfo.opn_
              .imm_num_);  // shape的类型必定为常量表达式，所以opn一定是立即数
    }
    tmp->width_.resize(tmp->shape_.size() + 1);
    tmp->width_[tmp->width_.size() - 1] = kIntWidth;
    for (int i = tmp->width_.size() - 2; i >= 0; i--)
      tmp->width_[i] = tmp->width_[i + 1] * tmp->shape_[i];
    scope.size_ += tmp->width_[0];
    // symbol_table[name_.name_.name_]=*tmp;
    symbol_table.insert({name_.name_.name_, *tmp});
  } else {
    SemanticError(this->line_no_,
                  this->name_.name_.name_ + ": variable redefined");
  }
}
void ArrayDefineWithInit::GenerateIR() {
  auto &name = this->name_.name_.name_;
  auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &array_iter = symbol_table.find(name);
  if (array_iter == symbol_table.end()) {
    SymbolTableItem symbol_item(true, this->is_const_, scope.size_);
    // 填shape
    for (const auto &shape : this->name_.shape_list_) {
      shape->Evaluate();
      if (gContextInfo.opn_.type_ != Opn::Type::Imm) {
        // 语义错误 数组维度应为常量表达式
        SemanticError(this->line_no_,"array dim should be constexp.");
        return;
      } else {
        symbol_item.shape_.push_back(gContextInfo.opn_.imm_num_);
      }
    }
    // 填width和dim total num
    std::stack<int> temp_width;
    int width = 4;
    temp_width.push(width);
    for (auto reiter = symbol_item.shape_.rbegin();
         reiter != symbol_item.shape_.rend(); ++reiter) {
      width *= *reiter;
      temp_width.push(width);
    }
    scope.size_ += temp_width.top();
    while (!temp_width.empty()) {
      symbol_item.width_.push_back(temp_width.top());
      temp_width.pop();
    }
    symbol_table.insert({name, symbol_item});

    // 完成赋值操作 或者 填写initval(global or const)
    // for arrayinitval
    gContextInfo.dim_total_num_.clear();
    for (const auto &width : symbol_item.width_) {
      gContextInfo.dim_total_num_.push_back(width / 4);
    }
    gContextInfo.dim_total_num_.push_back(1);
    gContextInfo.array_name_ = name;
    gContextInfo.array_offset_ = 0;
    gContextInfo.brace_num_ = 1;
    if (gContextInfo.current_scope_id_ == 0) {
      this->value_.Evaluate();
    } else if (this->is_const_) {
      // 局部常量
      this->value_.Evaluate();
      gContextInfo.array_offset_ = 0;
      gContextInfo.brace_num_ = 1;
      this->value_.GenerateIR();
    } else {
      this->value_.GenerateIR();
    }
  }
}

void FunctionDefine::GenerateIR() {
  const auto &func_iter = gFuncTable.find(this->name_.name_);
  if (func_iter == gFuncTable.end()) {
    auto tmp = new FuncTableItem(return_type_);
    gContextInfo.current_func_name_ = this->name_.name_;
    gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(this->name_.name_)});
    int parent_scope_id = 0;
    gContextInfo.current_scope_id_ = gSymbolTables.size();
    gSymbolTables.push_back({gContextInfo.current_scope_id_, parent_scope_id});
    gContextInfo.xingcan = true;
    for (const auto &arg : args_.arg_list_) {
      if (Identifier *ident = dynamic_cast<Identifier *>(&arg->name_)) {
        auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
        auto &symbol_table = scope.symbol_table_;
        const auto &var_iter = symbol_table.find(ident->name_);
        if (var_iter == symbol_table.end()) {
          symbol_table.insert({ident->name_, {false, false, scope.size_}});
          scope.size_ += kIntWidth;
        } else {
          SemanticError(this->line_no_, ident->name_ + ": variable redefined");
        }
        std::vector<int> tmp1;
        tmp->shape_list_.push_back(tmp1);
      } else if (ArrayIdentifier *arrident =
                     dynamic_cast<ArrayIdentifier *>(&arg->name_)) {
        auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
        auto &symbol_table = scope.symbol_table_;
        const auto &var_iter = symbol_table.find(arrident->name_.name_);
        if (var_iter == symbol_table.end()) {
          auto tmp1 = new SymbolTableItem(true, false, scope.size_);
          //计算shape和width
          tmp1->shape_.push_back(
              -1);  //第一维因为文法规定必须留空，所以我这里记个-1
          for (const auto &shape : arrident->shape_list_) {
            shape->Evaluate();
            tmp1->shape_.push_back(
                gContextInfo.opn_
                    .imm_num_);  // shape的类型必定为常量表达式，所以opn一定是立即数
          }
          tmp1->width_.resize(tmp1->shape_.size());
          if (tmp1->shape_.size()) {
            tmp1->width_[tmp1->width_.size() - 1] = kIntWidth;
            for (int i = tmp1->width_.size() - 2; i >= 0; i--)
              tmp1->width_[i] = tmp1->width_[i + 1] * tmp1->shape_[i + 1];
          }
          scope.size_ += tmp1->width_[0];
          symbol_table.insert({arrident->name_.name_, *tmp1});
          tmp->shape_list_.push_back(tmp1->shape_);
        } else {
          SemanticError(this->line_no_,
                        arrident->name_.name_ + ": variable redefined");
        }
      }
      // std::cout<<"haha"<<gContextInfo.shape_.size()<<'\n';
      // tmp->shape_list_.push_back(gContextInfo.shape_);
    }
    gFuncTable.insert({name_.name_, *tmp});
    gContextInfo.has_return=false;
    this->body_.GenerateIR();
    // 以下判断只有在返回类型为int并且无return语句才有用，如果返回类型为int并且写了个return;则bison
    // 会报段错误（不知道为啥）
    if(return_type_==289 && gContextInfo.has_return==false) // 这是parser.hpp中INT的值
      SemanticError(this->line_no_, this->name_.name_ + 
      ": function's return type is INT,but does not return anything");
  } else {
    SemanticError(this->line_no_, this->name_.name_ + ": function redefined");
  }
}

void AssignStatement::GenerateIR() {
  this->rhs_.GenerateIR();
  if (!gContextInfo.shape_.empty()) {
    // not int
    // NOTE 语义错误 类型错误 表达式结果非int类型
    SemanticError(this->line_no_,
                  gContextInfo.opn_.name_ + ": rhs exp res type not int");
  }
  Opn rhs_opn = gContextInfo.opn_;

  this->lhs_.GenerateIR();
  Opn &lhs_opn = gContextInfo.opn_;
  // if (lhs_opn.type_ == Opn::Type::Var) {
  SymbolTableItem *target_symbol_item =
      FindSymbol(gContextInfo.current_scope_id_, lhs_opn.name_);
  if (nullptr == target_symbol_item) {
    // NOTE 语义错误 变量未定义
    SemanticError(this->line_no_, "use undefined variable: " + lhs_opn.name_);
  } else {
    // type check
    if (!gContextInfo.shape_.empty()||gContextInfo.opn_.type_==Opn::Type::Null) {
      // NOTE 语义错误 类型错误 左值非int类型
      SemanticError(this->line_no_, lhs_opn.name_ + ": leftvalue type not int");
    }
    // const check
    if (target_symbol_item->is_const_) {
      // NOTE 语义错误 给const量赋值
      SemanticError(this->line_no_,
                    lhs_opn.name_ + ": constant can't be assigned");
    }
  }

  // genir(assign,opn1,-,res)
  gIRList.push_back({IR::OpKind::ASSIGN, rhs_opn, lhs_opn});
}

void IfElseStatement::GenerateIR() {
  std::string label_next = NewLabel();
  if (nullptr == this->elsestmt_) {
    // if then
    gContextInfo.true_label_.push("null");
    gContextInfo.false_label_.push(label_next);
    this->cond_.GenerateIR();
    gContextInfo.true_label_.pop();
    gContextInfo.false_label_.pop();
    this->thenstmt_.GenerateIR();
  } else {
    // if then else
    std::string label_else = NewLabel();
    gContextInfo.true_label_.push("null");
    gContextInfo.false_label_.push(label_else);
    this->cond_.GenerateIR();
    gContextInfo.true_label_.pop();
    gContextInfo.false_label_.pop();
    this->thenstmt_.GenerateIR();
    // genir(goto,next,-,-)
    gIRList.push_back({IR::OpKind::GOTO, LABEL_OPN(label_next)});
    // genir(label,else,-,-)
    gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(label_else)});
    this->elsestmt_->GenerateIR();
  }
  gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(label_next)});
}

void WhileStatement::GenerateIR() {
  std::string label_next = NewLabel();
  std::string label_begin = NewLabel();
  // genir(label,begin,-,-)
  gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(label_begin)});
  gContextInfo.true_label_.push("null");
  gContextInfo.false_label_.push(label_next);
  this->cond_.GenerateIR();
  gContextInfo.true_label_.pop();
  gContextInfo.false_label_.pop();
  this->bodystmt_.GenerateIR();
  // genir(goto,begin,-,-)
  gIRList.push_back({IR::OpKind::GOTO, LABEL_OPN(label_begin)});
  gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(label_next)});
}

void BreakStatement::GenerateIR() {
  // 检查是否在while循环中
  if (gContextInfo.break_label_.empty()) {  // not in while loop
    // NOTE: 语义错误 break语句位置错误
    SemanticError(this->line_no_, "'break' not in while loop");
  } else {
    // genir (goto,breaklabel,-,-)
    gIRList.push_back({IR::OpKind::GOTO,
                       {Opn::Type::Label, gContextInfo.break_label_.top()}});
  }
}

void ContinueStatement::GenerateIR() {
  // 检查是否在while循环中
  if (gContextInfo.continue_label_.empty()) {  // not in while loop
    // NOTE: 语义错误 break语句位置错误
    SemanticError(this->line_no_, "'continue' not in while loop");
  } else {
    // genir (goto,continuelabel,-,-)
    gIRList.push_back({IR::OpKind::GOTO,
                       {Opn::Type::Label, gContextInfo.continue_label_.top()}});
  }
}

void ReturnStatement::GenerateIR() {
  // [return]语句必然出现在函数定义中
  // 返回类型检查
  auto func_iter = gFuncTable.find(gContextInfo.current_func_name_);
  if (func_iter == gFuncTable.end()) {
    // NOTE 语义错误 函数未声明定义
    SemanticError(this->line_no_, "return statement in undeclared function: " +
                                      gContextInfo.current_func_name_);
  } else {
    int ret_type = (*func_iter).second.ret_type_;
    if (ret_type == VOID) {
      // return ;
      if (nullptr == this->value_) {
        // genir (ret,-,-,-)
        gIRList.push_back({IR::OpKind::RET});
      } else {
        // NOTE 语义错误 函数返回类型不匹配 应为void
        SemanticError(
            this->line_no_,
            "function ret type should be VOID according to function: " +
                gContextInfo.current_func_name_);
      }
    } else {
      // return exp(int);
      this->value_->GenerateIR();
      if (gContextInfo.shape_.empty()) {
        // exp type is int
        // genir (ret,opn,-,-)
        gIRList.push_back({IR::OpKind::RET, gContextInfo.opn_});
      } else {
        // NOTE 语义错误 函数返回类型不匹配 应为int
        SemanticError(
            this->line_no_,
            "function ret type should be INT according to function: " +
                gContextInfo.current_func_name_);
      }
    }
    gContextInfo.has_return = true;
  }
}

void VoidStatement::GenerateIR() {}

void EvalStatement::GenerateIR() { this->value_.GenerateIR(); }

void Block::GenerateIR() {
  // new scope
  int parent_scope_id = gContextInfo.current_scope_id_;
  ;
  if (gContextInfo.xingcan == false) {
    gContextInfo.current_scope_id_ = gSymbolTables.size();
    gSymbolTables.push_back({gContextInfo.current_scope_id_, parent_scope_id});
  }
  gContextInfo.xingcan = false;
  for (const auto &stmt : this->statement_list_) {
    stmt->GenerateIR();
  }
  // src scope
  gContextInfo.current_scope_id_ = parent_scope_id;
}

void DeclareStatement::GenerateIR() {
  // TODO 未完成
  for (const auto &def : this->define_list_) {
    def->GenerateIR();
  }
}

// const int c[2][3][4] = {1, {2}, 3,4,{5, 6, 7, 8},9, 10,  {11}, {12}, {13, 14,
// {15}, 16, {17}, {21}}};
// int c[2][3][4]={1,{2},3,4,{5},9,10,{11},{12},{13, 14, {15},16,{17},{21}}};
// int c[2][3][4]={{1},{13, 14, {15},16,{17},{21}}};

void ArrayInitVal::GenerateIR() {
  if (this->is_exp_) {
    this->value_->GenerateIR();
    if (!gContextInfo.shape_.empty()) {  // NOTE 语义错误 类型错误 值非int类型
      SemanticError(this->line_no_,
                    gContextInfo.opn_.name_ + ": exp type not int");
    }
    // genir([]=,expvalue,-,arrayname offset)
    // gIRList.push_back({IR::OpKind::OFFSET_ASSIGN,
    //                    gContextInfo.opn_,
    //                    {Opn::Type::Var, gContextInfo.array_name_},
    //                    gContextInfo.array_offset_ * 4});
    gIRList.push_back({IR::OpKind::OFFSET_ASSIGN,
                       gContextInfo.opn_,
                       {Opn::Type::Array,
                        gContextInfo.array_name_,
                        gContextInfo.current_scope_id_,
                        new Opn(Opn::Type::Imm, gContextInfo.array_offset_ * 4)}});

    gContextInfo.array_offset_ += 1;
  } else {
    // 此时该结点表示一对大括号 {}
    int &offset = gContextInfo.array_offset_;
    int &brace_num = gContextInfo.brace_num_;

    // 根据offset和brace_num算出finaloffset
    int temp_level = 0;
    const auto &dim_total_num = gContextInfo.dim_total_num_;
    for (int i = 0; i < dim_total_num.size(); ++i) {
      if (offset % dim_total_num[i] == 0) {
        temp_level = i;
        break;
      }
    }
    temp_level += brace_num;

    if (temp_level > dim_total_num.size()) {  // 语义错误 大括号过多
      SemanticError(this->line_no_, "多余大括号");
    } else {
      int final_offset = offset + dim_total_num[temp_level - 1];
      if (!this->initval_list_.empty()) {
        ++brace_num;
        this->initval_list_[0]->GenerateIR();

        for (int i = 1; i < this->initval_list_.size(); ++i) {
          brace_num = 1;
          this->initval_list_[i]->GenerateIR();
        }
      }
      // 补0补到finaloffset
      while (offset < final_offset) {
        // genir([]=,0,-,arrayname offset)
        // gIRList.push_back({IR::OpKind::OFFSET_ASSIGN,
        //                    IMM_0_OPN,
        //                    {Opn::Type::Var, gContextInfo.array_name_},
        //                    (offset++ * 4)});
        gIRList.push_back({IR::OpKind::OFFSET_ASSIGN,
                           IMM_0_OPN,
                           {Opn::Type::Array,
                            gContextInfo.array_name_,
                            gContextInfo.current_scope_id_,
                            new Opn(Opn::Type::Imm, (offset++ * 4))}});
      }
      // 语义检查
      if (offset > final_offset) {  // 语义错误 初始值设定项值太多
        SemanticError(this->line_no_, "初始值设定项值太多");
      }
    }
  }
}
void FunctionFormalParameter::GenerateIR() {}
void FunctionFormalParameterList::GenerateIR() {}
void FunctionActualParameterList::GenerateIR() {}
// int main(){if(1<=1){2;}}

// if ("null" == true_label && "null" != false_label) {
// } else if ("null" != true_label && "null" == false_label) {
// } else if ("null" != true_label && "null" != false_label) {
// } else {
//   // ERROR
// }

// if ("null" != false_label) {
// } else if ("null" != true_label) {
// } else {
//   // ERROR
// }
void ConditionExpression::GenerateIR() {
  bool lhs_is_cond = false;
  bool rhs_is_cond = false;
  try {
    dynamic_cast<ConditionExpression &>(this->lhs_);
    lhs_is_cond = true;
  } catch (std::bad_cast &e) {
  }
  try {
    dynamic_cast<ConditionExpression &>(this->rhs_);
    rhs_is_cond = true;
  } catch (std::bad_cast &e) {
  }
  std::string &true_label = gContextInfo.true_label_.top();
  std::string &false_label = gContextInfo.false_label_.top();
  Opn true_label_opn = LABEL_OPN(true_label);
  Opn false_label_opn = LABEL_OPN(false_label);

  if (lhs_is_cond && rhs_is_cond) {
    // lhs_和rhs_都是CondExp
    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // truelabel:null falselabel:src
          gContextInfo.true_label_.push("null");
          this->lhs_.GenerateIR();
          gContextInfo.true_label_.pop();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();
        } else if ("null" != true_label) {
          // truelabel:null falselabel:new
          std::string new_false_label = NewLabel();
          gContextInfo.true_label_.push("null");
          gContextInfo.false_label_.push(new_false_label);
          this->lhs_.GenerateIR();
          gContextInfo.false_label_.pop();
          gContextInfo.true_label_.pop();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" == true_label && "null" != false_label) {
          // truelabel:new falselabel:null
          std::string new_true_label = NewLabel();
          gContextInfo.true_label_.push(new_true_label);
          gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          gContextInfo.false_label_.pop();
          gContextInfo.true_label_.pop();

          // tl:src fl:src
          this->rhs_.GenerateIR();

          // genir(label,newtruelabel,-,-)
          gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
        } else if ("null" != true_label && "null" == false_label) {
          // truelabel:src falselabel:null
          this->lhs_.GenerateIR();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();
        } else if ("null" != true_label && "null" != false_label) {
          // truelabel:src falselabel:null
          gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          gContextInfo.false_label_.pop();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      default: {
        std::string lhs_temp_var = NewTemp();
        // insert symboltable 之前一定没有
        int scope_id = gContextInfo.current_scope_id_;
        gSymbolTables[scope_id].symbol_table_.insert(
            {lhs_temp_var, {false, false, gSymbolTables[scope_id].size_}});
        gSymbolTables[scope_id].size_ += kIntWidth;
        Opn lhs_opn = {Opn::Type::Var, lhs_temp_var, scope_id};

        // genir(assign,0,-,lhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_0_OPN, lhs_opn});

        // tl:null fl:rhslabel
        std::string rhs_label = NewLabel();
        gContextInfo.true_label_.push("null");
        gContextInfo.false_label_.push(rhs_label);
        this->lhs_.GenerateIR();
        gContextInfo.false_label_.pop();
        gContextInfo.true_label_.pop();

        // genir(assign,1,-,lhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_1_OPN, lhs_opn});
        // genir(label,rhslabel,-,-)
        gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(rhs_label)});

        std::string rhs_temp_var = NewTemp();
        // insert symboltable 之前一定没有
        gSymbolTables[scope_id].symbol_table_.insert(
            {rhs_temp_var, {false, false, gSymbolTables[scope_id].size_}});
        gSymbolTables[scope_id].size_ += kIntWidth;
        Opn rhs_opn = {Opn::Type::Var, rhs_temp_var, scope_id};

        // genir(assign,0,-,rhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_0_OPN, rhs_opn});

        // tl:null fl:nextlabel
        std::string next_label = NewLabel();
        gContextInfo.true_label_.push("null");
        gContextInfo.false_label_.push(next_label);
        this->lhs_.GenerateIR();
        gContextInfo.false_label_.pop();
        gContextInfo.true_label_.pop();

        // genir(assign,1,-,rhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_1_OPN, rhs_opn});
        // genir(label,nextlabel,-,-)
        gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(next_label)});

        if ("null" != false_label) {
          // genir(jreverseop,lhstempvar,rhstempvar,falselabel)
          gIRList.push_back(
              {GetOpKind(this->op_, true), lhs_opn, rhs_opn, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          gIRList.push_back(
              {GetOpKind(this->op_, false), lhs_opn, rhs_opn, true_label_opn});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
    }
  } else if (lhs_is_cond && !rhs_is_cond) {
    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // truelabel:null falselabel:src
          gContextInfo.true_label_.push("null");
          this->lhs_.GenerateIR();
          gContextInfo.true_label_.pop();

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, gContextInfo.opn_, IMM_0_OPN, false_label_opn});

          if ("null" != true_label) {
            // genir(goto, truelabel)
            gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // truelabel:null falselabel:new
          std::string new_false_label = NewLabel();
          gContextInfo.true_label_.push("null");
          gContextInfo.false_label_.push(new_false_label);
          this->lhs_.GenerateIR();
          gContextInfo.false_label_.pop();
          gContextInfo.true_label_.pop();

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, gContextInfo.opn_, IMM_0_OPN, true_label_opn});

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" == true_label && "null" != false_label) {
          // truelabel:new falselabel:null
          std::string new_true_label = NewLabel();
          gContextInfo.true_label_.push(new_true_label);
          gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          gContextInfo.false_label_.pop();
          gContextInfo.true_label_.pop();

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, gContextInfo.opn_, IMM_0_OPN, false_label_opn});

          // genir(label,newtruelabel,-,-)
          gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
        } else if ("null" != true_label && "null" == false_label) {
          // truelabel:src falselabel:null
          this->lhs_.GenerateIR();

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, gContextInfo.opn_, IMM_0_OPN, true_label_opn});
        } else if ("null" != true_label && "null" != false_label) {
          // truelabel:src falselabel:null
          gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          gContextInfo.false_label_.pop();

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, gContextInfo.opn_, IMM_0_OPN, false_label_opn});
          // genir(goto,truelabel)
          gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      default: {
        std::string lhs_temp_var = NewTemp();
        // insert symboltable 之前一定没有
        int scope_id = gContextInfo.current_scope_id_;
        gSymbolTables[scope_id].symbol_table_.insert(
            {lhs_temp_var, {false, false, gSymbolTables[scope_id].size_}});
        gSymbolTables[scope_id].size_ += kIntWidth;
        Opn lhs_opn = {Opn::Type::Var, lhs_temp_var, scope_id};

        // genir(assign,0,-,lhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_0_OPN, lhs_opn});

        // tl:null fl:rhslabel
        std::string rhs_label = NewLabel();
        gContextInfo.true_label_.push("null");
        gContextInfo.false_label_.push(rhs_label);
        this->lhs_.GenerateIR();
        gContextInfo.false_label_.pop();
        gContextInfo.true_label_.pop();

        // genir(assign,1,-,lhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_1_OPN, lhs_opn});
        // genir(label,rhslabel,-,-)
        gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(rhs_label)});

        this->rhs_.GenerateIR();
        // type check
        if (!gContextInfo.shape_.empty()) {
          // NOTE 语义错误 类型错误 条件表达式右边非int类型
          SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                            ": condexp rhs exp type not int");
        }

        if ("null" != false_label) {
          // genir(jreverseop,lhstempvar,rhs,falselabel)
          gIRList.push_back({GetOpKind(this->op_, true), lhs_opn,
                             gContextInfo.opn_, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          gIRList.push_back({GetOpKind(this->op_, false), lhs_opn,
                             gContextInfo.opn_, true_label_opn});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }

        break;
      }
    }
  } else if (!lhs_is_cond && rhs_is_cond) {
    this->lhs_.GenerateIR();
    // type check
    if (!gContextInfo.shape_.empty()) {
      // NOTE 语义错误 类型错误 条件表达式左边非int类型
      SemanticError(this->line_no_,
                    gContextInfo.opn_.name_ + ": condexp lhs exp type not int");
    }
    Opn lhs_opn = gContextInfo.opn_;

    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // genir(jeq,lhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, lhs_opn, IMM_0_OPN, false_label_opn});

          // tl:src fl:src
          this->rhs_.GenerateIR();
        } else if ("null" != true_label) {
          // genir(jeq,lhs,0,newfalselabel)
          std::string new_false_label = NewLabel();
          gIRList.push_back({IR::OpKind::JEQ, lhs_opn, IMM_0_OPN,
                             LABEL_OPN(new_false_label)});

          // tl:src fl:src
          this->rhs_.GenerateIR();

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" != false_label) {
          if ("null" == true_label) {
            std::string new_true_label = NewLabel();

            // genir(jne,lhs,0,newtruelabel)
            gIRList.push_back({IR::OpKind::JNE, lhs_opn, IMM_0_OPN,
                               LABEL_OPN(new_true_label)});

            // tl:null fl:src
            this->rhs_.GenerateIR();

            // genir(label,newtruelabel,-,-)
            gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
          } else {
            // genir(jne,lhs,0,truelabel)
            gIRList.push_back(
                {IR::OpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});

            // tl:src fl:src
            this->rhs_.GenerateIR();
          }
        } else if ("null" != true_label) {
          // genir(jne,lhs,0,truelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});

          // tl:src fl:src&null
          this->rhs_.GenerateIR();
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      default: {
        std::string rhs_temp_var = NewTemp();
        // insert symboltable 之前一定没有
        int scope_id = gContextInfo.current_scope_id_;
        gSymbolTables[scope_id].symbol_table_.insert(
            {rhs_temp_var, {false, false, gSymbolTables[scope_id].size_}});
        gSymbolTables[scope_id].size_ += kIntWidth;
        Opn rhs_opn = {Opn::Type::Var, rhs_temp_var, scope_id};

        // genir(assign,0,-,rhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_0_OPN, rhs_opn});

        // tl:null fl:nextlabel
        std::string next_label = NewLabel();
        gContextInfo.true_label_.push("null");
        gContextInfo.false_label_.push(next_label);
        this->lhs_.GenerateIR();
        gContextInfo.false_label_.pop();
        gContextInfo.true_label_.pop();

        // genir(assign,1,-,rhstmpvar)
        gIRList.push_back({IR::OpKind::ASSIGN, IMM_1_OPN, rhs_opn});
        // genir(label,nextlabel,-,-)
        gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(next_label)});

        if ("null" != false_label) {
          // genir(jreverseop,lhs,rhs,falselabel)
          gIRList.push_back(
              {GetOpKind(this->op_, true), lhs_opn, rhs_opn, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          gIRList.push_back(
              {GetOpKind(this->op_, false), lhs_opn, rhs_opn, true_label_opn});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
    }
  } else {
    // lhs_和rhs_都不是CondExp
    this->lhs_.GenerateIR();
    // type check
    if (!gContextInfo.shape_.empty()) {
      // NOTE 语义错误 类型错误 条件表达式左边非int类型
      SemanticError(this->line_no_,
                    gContextInfo.opn_.name_ + ": condexp lhs exp type not int");
    }
    Opn lhs_opn = gContextInfo.opn_;

    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // genir(jeq,lhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, lhs_opn, IMM_0_OPN, false_label_opn});

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, gContextInfo.opn_, IMM_0_OPN, false_label_opn});

          if ("null" != true_label) {
            // genir(goto, truelabel)
            gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jeq,lhs,0,newfalselabel)
          std::string new_false_label = NewLabel();
          gIRList.push_back({IR::OpKind::JEQ, lhs_opn, IMM_0_OPN,
                             LABEL_OPN(new_false_label)});

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, gContextInfo.opn_, IMM_0_OPN, true_label_opn});

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" != false_label) {
          std::string new_true_label = "";
          if ("null" == true_label) {
            new_true_label = NewLabel();
          } else {
            new_true_label = true_label;
          }
          // genir(jne,lhs,0,newtruelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, lhs_opn, IMM_0_OPN, LABEL_OPN(new_true_label)});

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          gIRList.push_back(
              {IR::OpKind::JEQ, gContextInfo.opn_, IMM_0_OPN, false_label_opn});

          if ("null" == true_label) {
            // genir(label,newtruelabel,-,-)
            gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
          } else {
            // genir(goto,newtruelabel,-,-)
            gIRList.push_back({IR::OpKind::GOTO, LABEL_OPN(new_true_label)});
          }
        } else if ("null" != true_label) {
          // genir(jne,lhs,0,truelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});

          this->rhs_.GenerateIR();
          if (!gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                              ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          gIRList.push_back(
              {IR::OpKind::JNE, gContextInfo.opn_, IMM_0_OPN, true_label_opn});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }

        break;
      }
      default: {  // relop eqop
        this->rhs_.GenerateIR();
        if (!gContextInfo.shape_.empty()) {
          // NOTE 语义错误 类型错误 条件表达式右边非int类型
          SemanticError(this->line_no_, gContextInfo.opn_.name_ +
                                            ": condexp rhs exp type not int");
        }

        if ("null" != false_label) {
          // genir:if False lhs op rhs goto falselabel
          // (jreverseop, lhs, rhs, falselabel)
          gIRList.push_back({GetOpKind(this->op_, true), lhs_opn,
                             gContextInfo.opn_, false_label_opn});

          if ("null" != true_label) {
            // genir(goto, truelabel)
            gIRList.push_back({IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhs,rhs,truelabel)
          gIRList.push_back({GetOpKind(this->op_, false), lhs_opn,
                             gContextInfo.opn_, true_label_opn});
        } else {
          // ERROR
          RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
    }
  }
}

}  // namespace ast
