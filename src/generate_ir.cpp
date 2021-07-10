#include <cstdio>
#include <stdexcept>

#include "../include/ast.h"
#include "../include/ir.h"
#include "parser.hpp"  // VOID INT

#define IMM_0_OPN \
  { ir::Opn::Type::Imm, 0 }
#define IMM_1_OPN \
  { ir::Opn::Type::Imm, 1 }
#define LABEL_OPN(label_name) \
  { ir::Opn::Type::Label, label_name }

namespace ast {

void Root::GenerateIR() {
  //把系统库中的函数添加到函数表中
  ir::gFuncTable.insert({"getint", {INT}});
  ir::gFuncTable.insert({"getch", {INT}});
  ir::FuncTableItem func_item = {INT};
  func_item.shape_list_.push_back({-1});
  ir::gFuncTable.insert({"getarray", func_item});
  ir::FuncTableItem func_item_void = {VOID};
  func_item_void.shape_list_.push_back({});
  ir::gFuncTable.insert({"putint", func_item_void});
  ir::gFuncTable.insert({"putch", func_item_void});
  func_item_void.shape_list_.push_back({-1});
  ir::gFuncTable.insert({"putarray", func_item_void});
  // TODO
  // ir::gFuncTable.insert({"putf", {VOID}});
  ir::gFuncTable.insert({"starttime", {VOID}});
  ir::gFuncTable.insert({"stoptime", {VOID}});
  //创建一张全局符号表
  ir::gScopes.push_back({0, -1});
  ir::gContextInfo.current_scope_id_ = 0;
  for (const auto &ele : this->compunit_list_) {
    ele->GenerateIR();
    // std::cout << "OK" << std::endl;
  }
  // 检查是否有main函数，main函数的返回值是否是int
  std::string sss = "main";
  auto func_iter = ir::gFuncTable.find(sss);
  if (func_iter == ir::gFuncTable.end()) {
    std::cout << "没有找到一个函数名为main的函数\n";
  } else {
    int ret_type = (*func_iter).second.ret_type_;
    if (ret_type == VOID) {
      std::cout << "main函数的返回值应该为int\n";
    }
  }
}
void Number::GenerateIR() {
  //生成一个opn 维护shape 不生成目标代码
  ir::Opn opn = ir::Opn(ir::Opn::Type::Imm, value_);
  ir::gContextInfo.opn_ = opn;
  ir::gContextInfo.shape_.clear();
}

void Identifier::GenerateIR() {
  ir::SymbolTableItem *s;
  //如果是函数名就不需要查变量表
  s = ir::FindSymbol(ir::gContextInfo.current_scope_id_, name_);
  if (!s) {
    ir::SemanticError(line_no_, name_ + ": undefined variable");
  }

  //这里如果是函数名的话s就未被初始化，所以函数名还是不要调用这个函数了
  ir::gContextInfo.shape_ = s->shape_;
  ir::Opn opn =
      ir::Opn(ir::Opn::Type::Var, name_, ir::gContextInfo.current_scope_id_);
  ir::gContextInfo.opn_ = opn;
}

// 这里有个点需要注意：如果是普通情况下取数组元素则shape_list_中的维度必须与符号表中维度相同，
// 但如果是函数调用中使用数组则可以传递指针shape_list_维度小于符号表中的维度即可
// ？？？语义检查时是否区分这两种情况
// 下面的实现按照不区分两种情况来写
void ArrayIdentifier::GenerateIR() {
  ir::SymbolTableItem *s;
  s = ir::FindSymbol(ir::gContextInfo.current_scope_id_, name_.name_);
  if (!s) {
    ir::SemanticError(line_no_, name_.name_ + ": undefined variable");
  }
  if (shape_list_.size() > s->shape_.size()) {
    ir::SemanticError(line_no_,
                      name_.name_ + ": the dimension of array is not correct");
  }

  ir::gContextInfo.shape_ =
      std::vector<int>(s->shape_.begin() + shape_list_.size(), s->shape_.end());

  // for(int i = 0; i < s->width_.size(); ++i)
  // {
  //   printf("width: %d\n", s->width_[i]);
  // }

  BinaryExpression *res = (BinaryExpression *)(new Number(0, 0));
  BinaryExpression *add_exp;
  BinaryExpression *mul_exp;
  // 计算数组取值的偏移
  for (int i = 0; i < shape_list_.size(); ++i) {
    // shape_list_[i]->GenerateIR();

    // // 计算当前维度偏移
    // std::string mul_temp = ir::NewTemp();
    // ir::gScopes[ir::gContextInfo.current_scope_id_].symbol_table_.insert(
    //     {mul_temp,
    //     {false, false,
    //     ir::gScopes[ir::gContextInfo.current_scope_id_].size_}});
    // ir::gScopes[ir::gContextInfo.current_scope_id_].size_ +=
    // ir::kIntWidth; ir::Opn mul_opn = ir::Opn(ir::Opn::Type::Var, nul_temp,
    // ir::gContextInfo.current_scope_id_); IR mul_ir = IR(ir::IR::OpKind::MUL,
    // ir::gContextInfo.opn_, ir::Opn(ir::Opn::Type::Imm, s->width_[i]),
    // mul_opn); ir::gIRList.push_back(mul_ir);

    // //加到总偏移中
    // std::string add_temp = ir::NewTemp();
    // ir::gScopes[ir::gContextInfo.current_scope_id_].symbol_table_.insert(
    //     {add_temp,
    //     {false, false,
    //     ir::gScopes[ir::gContextInfo.current_scope_id_].size_}});
    // ir::gScopes[ir::gContextInfo.current_scope_id_].size_ +=
    // ir::kIntWidth; ir::Opn add_opn = ir::Opn(ir::Opn::Type::Var,)

    //构造一个二元表达式
    Number *width = new Number(0, s->width_[i + 1]);
    // printf("width: %d\n", s->width_[i + 1]);
    mul_exp = new BinaryExpression(0, MUL, *width, *shape_list_[i]);
    add_exp = new BinaryExpression(0, ADD, *mul_exp, *res);
    res = add_exp;
    if (i == shape_list_.size() - 1) {
      res->GenerateIR();
      // res -> PrintNode();
    }
  }
  // a[b+1]
  // ir::gContextInfo.array_offset_ = index;
  // if(ir::gContextInfo.opn_.type_ == ir::Opn::Type::Imm)
  // printf("******************************%d", ir::gContextInfo.opn_.imm_num_);

  ir::Opn opn;
  ir::Opn *offset = new ir::Opn();
  offset->type_ = ir::gContextInfo.opn_.type_;
  offset->imm_num_ = ir::gContextInfo.opn_.imm_num_;
  offset->name_ = ir::gContextInfo.opn_.name_;
  offset->scope_id_ = ir::gContextInfo.opn_.scope_id_;
  offset->offset_ = ir::gContextInfo.opn_.offset_;
  // if(offset -> type_ == ir::Opn::Type::Imm) opn = ir::Opn(ir::Opn::Type::Imm,
  // s -> initval_[offset -> imm_num_ / 4]); else opn =
  // ir::Opn(ir::Opn::Type::Array, name_.name_,
  // ir::gContextInfo.current_scope_id_, offset);
  opn = ir::Opn(ir::Opn::Type::Array, name_.name_,
                ir::gContextInfo.current_scope_id_, offset);
  ir::gContextInfo.opn_ = opn;
}

void BinaryExpression::GenerateIR() {
  ir::Opn opn1, opn2, temp;
  ir::IR::OpKind op;

  lhs_.GenerateIR();
  opn1 = ir::gContextInfo.opn_;
  if (!ir::gContextInfo.shape_.empty()) {
    ir::SemanticError(this->line_no_,
                      ir::gContextInfo.opn_.name_ + ": lhs exp type not int");
  }
  rhs_.GenerateIR();
  opn2 = ir::gContextInfo.opn_;
  if (!ir::gContextInfo.shape_.empty()) {
    ir::SemanticError(this->line_no_,
                      ir::gContextInfo.opn_.name_ + ": rhs exp type not int");
  }

  switch (op_) {
    case ADD:
      op = ir::IR::OpKind::ADD;
      break;
    case SUB:
      op = ir::IR::OpKind::SUB;
      break;
    case MUL:
      op = ir::IR::OpKind::MUL;
      break;
    case DIV:
      op = ir::IR::OpKind::DIV;
      break;
    case MOD:
      op = ir::IR::OpKind::MOD;
      break;
    default:
      printf("mystery operator code: %d", op_);
      break;
  }

  if (opn1.type_ == ir::Opn::Type::Imm && opn2.type_ == ir::Opn::Type::Imm) {
    int result;
    int num1 = opn1.imm_num_, num2 = opn2.imm_num_;
    switch (op_) {
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
    temp = ir::Opn(ir::Opn::Type::Imm, result);
    ir::gContextInfo.opn_ = temp;
  } else {
    std::string res_temp_var = ir::NewTemp();
    ir::gScopes[ir::gContextInfo.current_scope_id_].symbol_table_.insert(
        {res_temp_var,
         {false, false,
          ir::gScopes[ir::gContextInfo.current_scope_id_].size_}});
    ir::gScopes[ir::gContextInfo.current_scope_id_].size_ += ir::kIntWidth;
    temp = ir::Opn(ir::Opn::Type::Var, res_temp_var,
                   ir::gContextInfo.current_scope_id_);
    ir::gContextInfo.opn_ = temp;
    ir::IR ir = ir::IR(op, opn1, opn2, temp);
    ir::gIRList.push_back(ir);
  }
}

void UnaryExpression::GenerateIR() {
  ir::Opn opn1, temp;
  ir::IR::OpKind op;

  rhs_.GenerateIR();
  opn1 = ir::gContextInfo.opn_;
  if (!ir::gContextInfo.shape_.empty()) {
    ir::SemanticError(this->line_no_,
                      ir::gContextInfo.opn_.name_ + ": rhs exp type not int");
  }

  switch (op_) {
    case ADD:
      op = ir::IR::OpKind::POS;
      break;
    case SUB:
      op = ir::IR::OpKind::NEG;
      break;
    case NOT:
      op = ir::IR::OpKind::NOT;
      break;
    default:
      printf("mystery operator code: %d", op_);
      break;
  }

  if (opn1.type_ == ir::Opn::Type::Imm) {
    int result;
    int num1 = opn1.imm_num_;
    switch (op_) {
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
    temp = ir::Opn(ir::Opn::Type::Imm, result);
    ir::gContextInfo.opn_ = temp;
  } else {
    auto scope_id = ir::gContextInfo.current_scope_id_;
    std::string rhs_temp_var = ir::NewTemp();
    ir::gScopes[scope_id].symbol_table_.insert(
        {rhs_temp_var, {false, false, ir::gScopes[scope_id].size_}});
    ir::gScopes[scope_id].size_ += ir::kIntWidth;
    ir::Opn temp = ir::Opn(ir::Opn::Type::Var, rhs_temp_var,
                           ir::gContextInfo.current_scope_id_);
    ir::gContextInfo.opn_ = temp;
    ir::IR ir = ir::IR(op, opn1, temp);
    ir::gIRList.push_back(ir);
  }
}

void FunctionCall::GenerateIR() {
  ir::Opn opn1, opn2;
  std::vector<ir::IR> param_list;
  if (ir::gFuncTable.find(name_.name_) == ir::gFuncTable.end()) {
    ir::SemanticError(line_no_, "调用的函数不存在");
    return;
  }
  if (ir::gFuncTable[name_.name_].shape_list_.size() !=
      args_.arg_list_.size()) {
    ir::SemanticError(line_no_, "参数个数不匹配");
    return;
  }

  for (int i = args_.arg_list_.size() - 1; i >= 0; --i) {
    args_.arg_list_[i]->GenerateIR();
    if (ir::gContextInfo.shape_.size() !=
        ir::gFuncTable[name_.name_].shape_list_[i].size()) {
      std::cout << ir::gContextInfo.shape_.size() << ' '
                << ir::gFuncTable[name_.name_].shape_list_[i].size() << '\n';
      ir::SemanticError(line_no_, "函数调用参数类型不匹配");
      return;
    } else {
      for (int j = 1; j < ir::gContextInfo.shape_.size(); j++) {
        if (ir::gContextInfo.shape_[j] !=
            ir::gFuncTable[name_.name_].shape_list_[i][j]) {
          ir::SemanticError(line_no_, "函数调用参数类型不匹配");
          // printf("/*****************\n");
          return;
        }
      }
    }

    ir::SymbolTableItem *s;
    //如果是函数名就不需要查变量表
    s = ir::FindSymbol(ir::gContextInfo.current_scope_id_,
                       ir::gContextInfo.opn_.name_);
    if (s) {
      if (s->is_array_ && s->is_const_) {
        ir::SemanticError(line_no_, "实参不能是const数组");
        return;
      }
    }

    opn1 = ir::gContextInfo.opn_;
    ir::Opn *offset = new ir::Opn(ir::Opn::Type::Imm, 0);
    if (s && s->is_array_) {
      opn1 = ir::Opn(ir::Opn::Type::Array, opn1.name_,
                     ir::gContextInfo.current_scope_id_, offset);
      // ir::gContextInfo.shape_ = s -> shape_;
    }

    ir::IR ir = ir::IR(ir::IR::OpKind::PARAM, opn1);
    param_list.push_back(ir);
  }

  for (auto &param : param_list) {
    ir::gIRList.push_back(param);
  }

  // printf("%d\n", ir::gContextInfo.is_func_);
  // ir::gContextInfo.is_func_ = !ir::gContextInfo.is_func_;
  // name_.GenerateIR();
  // printf("%d\n", ir::gContextInfo.is_func_);
  // opn1 = ir::gContextInfo.opn_;
  // ir::gContextInfo.is_func_ = !ir::gContextInfo.is_func_;

  opn1 = ir::Opn(ir::Opn::Type::Var, name_.name_,
                 ir::gContextInfo.current_scope_id_);
  opn2 = ir::Opn(ir::Opn::Type::Imm,
                 ir::gFuncTable[name_.name_].shape_list_.size());

  ir::IR ir;
  if (ir::gFuncTable[name_.name_].ret_type_ == INT) {
    auto scope_id = ir::gContextInfo.current_scope_id_;
    std::string ret_var = ir::NewTemp();
    ir::gScopes[scope_id].symbol_table_.insert(
        {ret_var, {false, false, ir::gScopes[scope_id].size_}});
    ir::gScopes[scope_id].size_ += ir::kIntWidth;
    ir::Opn temp = ir::Opn(ir::Opn::Type::Var, ret_var,
                           ir::gContextInfo.current_scope_id_);
    ir::gContextInfo.opn_ = temp;
    ir::gContextInfo.shape_.clear();
    ir = ir::IR(ir::IR::OpKind::CALL, opn1, opn2, temp);
  } else {
    ir::Opn temp = ir::Opn(ir::Opn::Type::Null);
    ir::gContextInfo.opn_ = temp;
    ir = ir::IR(ir::IR::OpKind::CALL, opn1, opn2, temp);
  }
  // printf("/*********************\n");
  ir::gIRList.push_back(ir);
}

void VariableDefine::GenerateIR() {
  auto &scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_);
  // 得往上递归地找
  // int scope_id=ir::gContextInfo.current_scope_id_;
  // Scope scope;
  // std::unordered_map<std::string,ir::SymbolTableItem> symbol_table;
  // while(scope_id!=-1)
  // {
  //   scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
  //   symbol_table = scope.symbol_table_;
  //   const auto &var_iter = symbol_table.find(this->name_.name_);
  //   if(var_iter == symbol_table.end())scope_id=scope.parent_scope_id_;
  // }
  if (var_iter == symbol_table.end()) {
    auto tmp = new ir::SymbolTableItem(false, false, scope.size_);
    if (ir::gContextInfo.current_scope_id_ == 0) tmp->initval_.push_back(0);
    symbol_table.insert({this->name_.name_, *tmp});
    scope.size_ += ir::kIntWidth;
  } else {
    ir::SemanticError(this->line_no_,
                      this->name_.name_ + ": variable redefined");
  }
}
void VariableDefineWithInit::GenerateIR() {
  auto &scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_);
  if (var_iter == symbol_table.end()) {
    auto tmp = new ir::SymbolTableItem(false, is_const_, scope.size_);
    scope.size_ += ir::kIntWidth;
    if (ir::gContextInfo.current_scope_id_ == 0 || is_const_) {
      value_.Evaluate();
      tmp->initval_.push_back(ir::gContextInfo.opn_.imm_num_);
    } else {
      value_.GenerateIR();
      ir::Opn lhs_opn = ir::Opn(ir::Opn::Type::Var, name_.name_,
                                ir::gContextInfo.current_scope_id_);
      ir::gIRList.push_back(
          {ir::IR::OpKind::ASSIGN, ir::gContextInfo.opn_, lhs_opn});
    }
    symbol_table.insert({this->name_.name_, *tmp});
  } else {
    ir::SemanticError(this->line_no_,
                      this->name_.name_ + ": variable redefined");
  }
}
void ArrayDefine::GenerateIR() {
  auto &scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_.name_);
  if (var_iter == symbol_table.end()) {
    auto tmp = new ir::SymbolTableItem(true, false, scope.size_);
    //计算shape和width
    for (const auto &shape : name_.shape_list_) {
      shape->Evaluate();
      tmp->shape_.push_back(
          ir::gContextInfo.opn_
              .imm_num_);  // shape的类型必定为常量表达式，所以opn一定是立即数
    }
    tmp->width_.resize(tmp->shape_.size() + 1);
    tmp->width_[tmp->width_.size() - 1] = ir::kIntWidth;
    for (int i = tmp->width_.size() - 2; i >= 0; i--)
      tmp->width_[i] = tmp->width_[i + 1] * tmp->shape_[i];
    scope.size_ += tmp->width_[0];
    if (ir::gContextInfo.current_scope_id_ == 0) {
      int mul = 1;
      for (int i = 0; i < tmp->shape_.size(); i++) mul *= tmp->shape_[i];
      tmp->initval_.resize(mul);
    }
    // symbol_table[name_.name_.name_]=*tmp;
    symbol_table.insert({name_.name_.name_, *tmp});
  } else {
    ir::SemanticError(this->line_no_,
                      this->name_.name_.name_ + ": variable redefined");
  }
}
void ArrayDefineWithInit::GenerateIR() {
  auto &name = this->name_.name_.name_;
  auto &scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &array_iter = symbol_table.find(name);
  if (array_iter == symbol_table.end()) {
    ir::SymbolTableItem symbol_item(true, this->is_const_, scope.size_);
    // 填shape
    for (const auto &shape : this->name_.shape_list_) {
      shape->Evaluate();
      if (ir::gContextInfo.opn_.type_ != ir::Opn::Type::Imm) {
        // 语义错误 数组维度应为常量表达式
        ir::SemanticError(this->line_no_, "array dim should be constexp.");
        return;
      } else {
        symbol_item.shape_.push_back(ir::gContextInfo.opn_.imm_num_);
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
    ir::gContextInfo.dim_total_num_.clear();
    for (const auto &width : symbol_item.width_) {
      ir::gContextInfo.dim_total_num_.push_back(width / 4);
    }
    ir::gContextInfo.dim_total_num_.push_back(1);
    ir::gContextInfo.array_name_ = name;
    ir::gContextInfo.array_offset_ = 0;
    ir::gContextInfo.brace_num_ = 1;
    if (ir::gContextInfo.current_scope_id_ == 0) {
      this->value_.Evaluate();
    } else if (this->is_const_) {
      // 局部常量
      this->value_.Evaluate();
      ir::gContextInfo.array_offset_ = 0;
      ir::gContextInfo.brace_num_ = 1;
      this->value_.GenerateIR();
    } else {
      this->value_.GenerateIR();
    }
  }
}

void FunctionDefine::GenerateIR() {
  const auto &func_iter = ir::gFuncTable.find(this->name_.name_);
  if (func_iter == ir::gFuncTable.end()) {
    auto tmp = new ir::FuncTableItem(return_type_);
    ir::gContextInfo.current_func_name_ = this->name_.name_;
    ir::gIRList.push_back(
        {ir::IR::OpKind::LABEL, LABEL_OPN(this->name_.name_)});
    int parent_scope_id = 0;
    ir::gContextInfo.current_scope_id_ = ir::gScopes.size();
    ir::gScopes.push_back(
        {ir::gContextInfo.current_scope_id_, parent_scope_id});
    ir::gContextInfo.xingcan = true;
    for (const auto &arg : args_.arg_list_) {
      if (Identifier *ident = dynamic_cast<Identifier *>(&arg->name_)) {
        auto &scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
        auto &symbol_table = scope.symbol_table_;
        const auto &var_iter = symbol_table.find(ident->name_);
        if (var_iter == symbol_table.end()) {
          symbol_table.insert({ident->name_, {false, false, scope.size_}});
          scope.size_ += ir::kIntWidth;
        } else {
          ir::SemanticError(this->line_no_,
                            ident->name_ + ": variable redefined");
        }
        std::vector<int> tmp1;
        tmp->shape_list_.push_back(tmp1);
      } else if (ArrayIdentifier *arrident =
                     dynamic_cast<ArrayIdentifier *>(&arg->name_)) {
        auto &scope = ir::gScopes[ir::gContextInfo.current_scope_id_];
        auto &symbol_table = scope.symbol_table_;
        const auto &var_iter = symbol_table.find(arrident->name_.name_);
        if (var_iter == symbol_table.end()) {
          auto tmp1 = new ir::SymbolTableItem(true, false, scope.size_);
          //计算shape和width
          tmp1->shape_.push_back(
              -1);  //第一维因为文法规定必须留空，所以我这里记个-1
          for (const auto &shape : arrident->shape_list_) {
            shape->Evaluate();
            tmp1->shape_.push_back(
                ir::gContextInfo.opn_
                    .imm_num_);  // shape的类型必定为常量表达式，所以opn一定是立即数
          }
          tmp1->width_.resize(tmp1->shape_.size() + 1);
          if (tmp1->shape_.size()) {
            tmp1->width_[tmp1->width_.size() - 1] = ir::kIntWidth;
            for (int i = tmp1->width_.size() - 2; i >= 0; i--)
              tmp1->width_[i] = tmp1->width_[i + 1] * tmp1->shape_[i + 1];
          }
          scope.size_ += tmp1->width_[0];
          symbol_table.insert({arrident->name_.name_, *tmp1});
          tmp->shape_list_.push_back(tmp1->shape_);
        } else {
          ir::SemanticError(this->line_no_,
                            arrident->name_.name_ + ": variable redefined");
        }
      }
      // std::cout<<"haha"<<ir::gContextInfo.shape_.size()<<'\n';
      // tmp->shape_list_.push_back(ir::gContextInfo.shape_);
    }
    ir::gFuncTable.insert({name_.name_, *tmp});
    ir::gContextInfo.has_return = false;
    this->body_.GenerateIR();
    // 以下判断只有在返回类型为int并且无return语句才有用，如果返回类型为int并且写了个return;则bison
    // 会报段错误（不知道为啥）
    if (return_type_ == 289 &&
        ir::gContextInfo.has_return == false)  // 这是parser.hpp中INT的值
      ir::SemanticError(
          this->line_no_,
          this->name_.name_ +
              ": function's return type is INT,but does not return anything");
  } else {
    ir::SemanticError(this->line_no_,
                      this->name_.name_ + ": function redefined");
  }

  // printf("1*******************\n");
}

void AssignStatement::GenerateIR() {
  this->rhs_.GenerateIR();
  if (!ir::gContextInfo.shape_.empty()) {
    // not int
    // NOTE 语义错误 类型错误 表达式结果非int类型
    ir::SemanticError(this->line_no_, ir::gContextInfo.opn_.name_ +
                                          ": rhs exp res type not int");
  }
  ir::Opn rhs_opn = ir::gContextInfo.opn_;

  this->lhs_.GenerateIR();
  ir::Opn &lhs_opn = ir::gContextInfo.opn_;
  // if (lhs_opn.type_ == ir::Opn::Type::Var) {
  ir::SymbolTableItem *target_symbol_item =
      ir::FindSymbol(ir::gContextInfo.current_scope_id_, lhs_opn.name_);
  if (nullptr == target_symbol_item) {
    // NOTE 语义错误 变量未定义
    ir::SemanticError(this->line_no_,
                      "use undefined variable: " + lhs_opn.name_);
  } else {
    // type check
    if (!ir::gContextInfo.shape_.empty() ||
        ir::gContextInfo.opn_.type_ == ir::Opn::Type::Null) {
      // NOTE 语义错误 类型错误 左值非int类型
      ir::SemanticError(this->line_no_,
                        lhs_opn.name_ + ": leftvalue type not int");
    }
    // const check
    if (target_symbol_item->is_const_) {
      // NOTE 语义错误 给const量赋值
      ir::SemanticError(this->line_no_,
                        lhs_opn.name_ + ": constant can't be assigned");
    }
  }

  // genir(assign,opn1,-,res)
  ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, rhs_opn, lhs_opn});
}

void IfElseStatement::GenerateIR() {
  std::string label_next = ir::NewLabel();
  if (nullptr == this->elsestmt_) {
    // if then
    ir::gContextInfo.true_label_.push("null");
    ir::gContextInfo.false_label_.push(label_next);
    this->cond_.GenerateIR();
    ir::gContextInfo.true_label_.pop();
    ir::gContextInfo.false_label_.pop();
    this->thenstmt_.GenerateIR();
  } else {
    // if then else
    std::string label_else = ir::NewLabel();
    ir::gContextInfo.true_label_.push("null");
    ir::gContextInfo.false_label_.push(label_else);
    this->cond_.GenerateIR();
    ir::gContextInfo.true_label_.pop();
    ir::gContextInfo.false_label_.pop();
    this->thenstmt_.GenerateIR();
    // genir(goto,next,-,-)
    ir::gIRList.push_back({ir::IR::OpKind::GOTO, LABEL_OPN(label_next)});
    // genir(label,else,-,-)
    ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(label_else)});
    this->elsestmt_->GenerateIR();
  }
  ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(label_next)});
}

void WhileStatement::GenerateIR() {
  std::string label_next = ir::NewLabel();
  std::string label_begin = ir::NewLabel();
  // genir(label,begin,-,-)
  ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(label_begin)});
  ir::gContextInfo.true_label_.push("null");
  ir::gContextInfo.false_label_.push(label_next);
  this->cond_.GenerateIR();
  ir::gContextInfo.true_label_.pop();
  ir::gContextInfo.false_label_.pop();

  ir::gContextInfo.break_label_.push(label_next);
  ir::gContextInfo.continue_label_.push(label_begin);

  this->bodystmt_.GenerateIR();
  // genir(goto,begin,-,-)
  ir::gIRList.push_back({ir::IR::OpKind::GOTO, LABEL_OPN(label_begin)});

  ir::gContextInfo.continue_label_.pop();
  ir::gContextInfo.break_label_.pop();

  ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(label_next)});
}

void BreakStatement::GenerateIR() {
  // 检查是否在while循环中
  if (ir::gContextInfo.break_label_.empty()) {  // not in while loop
    // NOTE: 语义错误 break语句位置错误
    ir::SemanticError(this->line_no_, "'break' not in while loop");
  } else {
    // genir (goto,breaklabel,-,-)
    ir::gIRList.push_back(
        {ir::IR::OpKind::GOTO,
         {ir::Opn::Type::Label, ir::gContextInfo.break_label_.top()}});
  }
}

void ContinueStatement::GenerateIR() {
  // 检查是否在while循环中
  if (ir::gContextInfo.continue_label_.empty()) {  // not in while loop
    // NOTE: 语义错误 break语句位置错误
    ir::SemanticError(this->line_no_, "'continue' not in while loop");
  } else {
    // genir (goto,continuelabel,-,-)
    ir::gIRList.push_back(
        {ir::IR::OpKind::GOTO,
         {ir::Opn::Type::Label, ir::gContextInfo.continue_label_.top()}});
  }
}

void ReturnStatement::GenerateIR() {
  // [return]语句必然出现在函数定义中
  // 返回类型检查
  auto func_iter = ir::gFuncTable.find(ir::gContextInfo.current_func_name_);
  if (func_iter == ir::gFuncTable.end()) {
    // NOTE 语义错误 函数未声明定义
    ir::SemanticError(this->line_no_,
                      "return statement in undeclared function: " +
                          ir::gContextInfo.current_func_name_);
  } else {
    int ret_type = (*func_iter).second.ret_type_;
    if (ret_type == VOID) {
      // return ;
      if (nullptr == this->value_) {
        // genir (ret,-,-,-)
        ir::gIRList.push_back({ir::IR::OpKind::RET});
      } else {
        // NOTE 语义错误 函数返回类型不匹配 应为void
        ir::SemanticError(
            this->line_no_,
            "function ret type should be VOID according to function: " +
                ir::gContextInfo.current_func_name_);
      }
    } else {
      // return exp(int);
      this->value_->GenerateIR();
      if (ir::gContextInfo.shape_.empty()) {
        // exp type is int
        // genir (ret,opn,-,-)
        ir::gIRList.push_back({ir::IR::OpKind::RET, ir::gContextInfo.opn_});
      } else {
        // NOTE 语义错误 函数返回类型不匹配 应为int
        ir::SemanticError(
            this->line_no_,
            "function ret type should be INT according to function: " +
                ir::gContextInfo.current_func_name_);
      }
    }
    ir::gContextInfo.has_return = true;
  }
}

void VoidStatement::GenerateIR() {}

void EvalStatement::GenerateIR() { this->value_.GenerateIR(); }

void Block::GenerateIR() {
  // new scope
  int parent_scope_id = ir::gContextInfo.current_scope_id_;
  ;
  if (ir::gContextInfo.xingcan == false) {
    ir::gContextInfo.current_scope_id_ = ir::gScopes.size();
    ir::gScopes.push_back(
        {ir::gContextInfo.current_scope_id_, parent_scope_id});
  }
  ir::gContextInfo.xingcan = false;
  for (const auto &stmt : this->statement_list_) {
    stmt->GenerateIR();
  }
  // src scope
  ir::gContextInfo.current_scope_id_ = parent_scope_id;
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
    if (!ir::gContextInfo.shape_
             .empty()) {  // NOTE 语义错误 类型错误 值非int类型
      ir::SemanticError(this->line_no_,
                        ir::gContextInfo.opn_.name_ + ": exp type not int");
    }
    // genir([]=,expvalue,-,arrayname offset)
    // ir::gIRList.push_back({ir::IR::OpKind::OFFSET_ASSIGN,
    //                    ir::gContextInfo.opn_,
    //                    {ir::Opn::Type::Var, ir::gContextInfo.array_name_},
    //                    ir::gContextInfo.array_offset_ * 4});
    ir::gIRList.push_back({ir::IR::OpKind::/*OFFSET_*/ ASSIGN,
                           ir::gContextInfo.opn_,
                           {ir::Opn::Type::Array, ir::gContextInfo.array_name_,
                            ir::gContextInfo.current_scope_id_,
                            new ir::Opn(ir::Opn::Type::Imm,
                                        ir::gContextInfo.array_offset_ * 4)}});

    ir::gContextInfo.array_offset_ += 1;
  } else {
    // 此时该结点表示一对大括号 {}
    int &offset = ir::gContextInfo.array_offset_;
    int &brace_num = ir::gContextInfo.brace_num_;

    // 根据offset和brace_num算出finaloffset
    int temp_level = 0;
    const auto &dim_total_num = ir::gContextInfo.dim_total_num_;
    for (int i = 0; i < dim_total_num.size(); ++i) {
      if (offset % dim_total_num[i] == 0) {
        temp_level = i;
        break;
      }
    }
    temp_level += brace_num;

    if (temp_level > dim_total_num.size()) {  // 语义错误 大括号过多
      ir::SemanticError(this->line_no_, "多余大括号");
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
        // ir::gIRList.push_back({ir::IR::OpKind::OFFSET_ASSIGN,
        //                    IMM_0_OPN,
        //                    {ir::Opn::Type::Var,
        //                    ir::gContextInfo.array_name_}, (offset++ * 4)});
        ir::gIRList.push_back(
            {ir::IR::OpKind::/*OFFSET_*/ ASSIGN,
             IMM_0_OPN,
             {ir::Opn::Type::Array, ir::gContextInfo.array_name_,
              ir::gContextInfo.current_scope_id_,
              new ir::Opn(ir::Opn::Type::Imm, (offset++ * 4))}});
      }
      // 语义检查
      if (offset > final_offset) {  // 语义错误 初始值设定项值太多
        ir::SemanticError(this->line_no_, "初始值设定项值太多");
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
  std::string &true_label = ir::gContextInfo.true_label_.top();
  std::string &false_label = ir::gContextInfo.false_label_.top();
  ir::Opn true_label_opn = LABEL_OPN(true_label);
  ir::Opn false_label_opn = LABEL_OPN(false_label);

  if (lhs_is_cond && rhs_is_cond) {
    // lhs_和rhs_都是CondExp
    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // truelabel:null falselabel:src
          ir::gContextInfo.true_label_.push("null");
          this->lhs_.GenerateIR();
          ir::gContextInfo.true_label_.pop();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();
        } else if ("null" != true_label) {
          // truelabel:null falselabel:new
          std::string new_false_label = ir::NewLabel();
          ir::gContextInfo.true_label_.push("null");
          ir::gContextInfo.false_label_.push(new_false_label);
          this->lhs_.GenerateIR();
          ir::gContextInfo.false_label_.pop();
          ir::gContextInfo.true_label_.pop();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();

          // genir(label,newfalselabel,-,-)
          ir::gIRList.push_back(
              {ir::IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" == true_label && "null" != false_label) {
          // truelabel:new falselabel:null
          std::string new_true_label = ir::NewLabel();
          ir::gContextInfo.true_label_.push(new_true_label);
          ir::gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          ir::gContextInfo.false_label_.pop();
          ir::gContextInfo.true_label_.pop();

          // tl:src fl:src
          this->rhs_.GenerateIR();

          // genir(label,newtruelabel,-,-)
          ir::gIRList.push_back(
              {ir::IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
        } else if ("null" != true_label && "null" == false_label) {
          // truelabel:src falselabel:null
          this->lhs_.GenerateIR();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();
        } else if ("null" != true_label && "null" != false_label) {
          // truelabel:src falselabel:null
          ir::gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          ir::gContextInfo.false_label_.pop();

          // truelabel:src falselabel:src
          this->rhs_.GenerateIR();
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      default: {
        std::string lhs_temp_var = ir::NewTemp();
        // insert symboltable 之前一定没有
        int scope_id = ir::gContextInfo.current_scope_id_;
        ir::gScopes[scope_id].symbol_table_.insert(
            {lhs_temp_var, {false, false, ir::gScopes[scope_id].size_}});
        ir::gScopes[scope_id].size_ += ir::kIntWidth;
        ir::Opn lhs_opn = {ir::Opn::Type::Var, lhs_temp_var, scope_id};

        // genir(assign,0,-,lhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_0_OPN, lhs_opn});

        // tl:null fl:rhslabel
        std::string rhs_label = ir::NewLabel();
        ir::gContextInfo.true_label_.push("null");
        ir::gContextInfo.false_label_.push(rhs_label);
        this->lhs_.GenerateIR();
        ir::gContextInfo.false_label_.pop();
        ir::gContextInfo.true_label_.pop();

        // genir(assign,1,-,lhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_1_OPN, lhs_opn});
        // genir(label,rhslabel,-,-)
        ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(rhs_label)});

        std::string rhs_temp_var = ir::NewTemp();
        // insert symboltable 之前一定没有
        ir::gScopes[scope_id].symbol_table_.insert(
            {rhs_temp_var, {false, false, ir::gScopes[scope_id].size_}});
        ir::gScopes[scope_id].size_ += ir::kIntWidth;
        ir::Opn rhs_opn = {ir::Opn::Type::Var, rhs_temp_var, scope_id};

        // genir(assign,0,-,rhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_0_OPN, rhs_opn});

        // tl:null fl:nextlabel
        std::string next_label = ir::NewLabel();
        ir::gContextInfo.true_label_.push("null");
        ir::gContextInfo.false_label_.push(next_label);
        this->lhs_.GenerateIR();
        ir::gContextInfo.false_label_.pop();
        ir::gContextInfo.true_label_.pop();

        // genir(assign,1,-,rhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_1_OPN, rhs_opn});
        // genir(label,nextlabel,-,-)
        ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(next_label)});

        if ("null" != false_label) {
          // genir(jreverseop,lhstempvar,rhstempvar,falselabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn,
                                 rhs_opn, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn,
                                 rhs_opn, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError(
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
          ir::gContextInfo.true_label_.push("null");
          this->lhs_.GenerateIR();
          ir::gContextInfo.true_label_.pop();

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, ir::gContextInfo.opn_,
                                 IMM_0_OPN, false_label_opn});

          if ("null" != true_label) {
            // genir(goto, truelabel)
            ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // truelabel:null falselabel:new
          std::string new_false_label = ir::NewLabel();
          ir::gContextInfo.true_label_.push("null");
          ir::gContextInfo.false_label_.push(new_false_label);
          this->lhs_.GenerateIR();
          ir::gContextInfo.false_label_.pop();
          ir::gContextInfo.true_label_.pop();

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          ir::gIRList.push_back({ir::IR::OpKind::JNE, ir::gContextInfo.opn_,
                                 IMM_0_OPN, true_label_opn});

          // genir(label,newfalselabel,-,-)
          ir::gIRList.push_back(
              {ir::IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" == true_label && "null" != false_label) {
          // truelabel:new falselabel:null
          std::string new_true_label = ir::NewLabel();
          ir::gContextInfo.true_label_.push(new_true_label);
          ir::gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          ir::gContextInfo.false_label_.pop();
          ir::gContextInfo.true_label_.pop();

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, ir::gContextInfo.opn_,
                                 IMM_0_OPN, false_label_opn});

          // genir(label,newtruelabel,-,-)
          ir::gIRList.push_back(
              {ir::IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
        } else if ("null" != true_label && "null" == false_label) {
          // truelabel:src falselabel:null
          this->lhs_.GenerateIR();

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          ir::gIRList.push_back({ir::IR::OpKind::JNE, ir::gContextInfo.opn_,
                                 IMM_0_OPN, true_label_opn});
        } else if ("null" != true_label && "null" != false_label) {
          // truelabel:src falselabel:null
          ir::gContextInfo.false_label_.push("null");
          this->lhs_.GenerateIR();
          ir::gContextInfo.false_label_.pop();

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, ir::gContextInfo.opn_,
                                 IMM_0_OPN, false_label_opn});
          // genir(goto,truelabel)
          ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      default: {
        std::string lhs_temp_var = ir::NewTemp();
        // insert symboltable 之前一定没有
        int scope_id = ir::gContextInfo.current_scope_id_;
        ir::gScopes[scope_id].symbol_table_.insert(
            {lhs_temp_var, {false, false, ir::gScopes[scope_id].size_}});
        ir::gScopes[scope_id].size_ += ir::kIntWidth;
        ir::Opn lhs_opn = {ir::Opn::Type::Var, lhs_temp_var, scope_id};

        // genir(assign,0,-,lhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_0_OPN, lhs_opn});

        // tl:null fl:rhslabel
        std::string rhs_label = ir::NewLabel();
        ir::gContextInfo.true_label_.push("null");
        ir::gContextInfo.false_label_.push(rhs_label);
        this->lhs_.GenerateIR();
        ir::gContextInfo.false_label_.pop();
        ir::gContextInfo.true_label_.pop();

        // genir(assign,1,-,lhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_1_OPN, lhs_opn});
        // genir(label,rhslabel,-,-)
        ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(rhs_label)});

        this->rhs_.GenerateIR();
        // type check
        if (!ir::gContextInfo.shape_.empty()) {
          // NOTE 语义错误 类型错误 条件表达式右边非int类型
          ir::SemanticError(
              this->line_no_,
              ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
        }

        if ("null" != false_label) {
          // genir(jreverseop,lhstempvar,rhs,falselabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn,
                                 ir::gContextInfo.opn_, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn,
                                 ir::gContextInfo.opn_, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }

        break;
      }
    }
  } else if (!lhs_is_cond && rhs_is_cond) {
    this->lhs_.GenerateIR();
    // type check
    if (!ir::gContextInfo.shape_.empty()) {
      // NOTE 语义错误 类型错误 条件表达式左边非int类型
      ir::SemanticError(this->line_no_, ir::gContextInfo.opn_.name_ +
                                            ": condexp lhs exp type not int");
    }
    ir::Opn lhs_opn = ir::gContextInfo.opn_;

    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // genir(jeq,lhs,0,falselabel)
          ir::gIRList.push_back(
              {ir::IR::OpKind::JEQ, lhs_opn, IMM_0_OPN, false_label_opn});

          // tl:src fl:src
          this->rhs_.GenerateIR();
        } else if ("null" != true_label) {
          // genir(jeq,lhs,0,newfalselabel)
          std::string new_false_label = ir::NewLabel();
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, lhs_opn, IMM_0_OPN,
                                 LABEL_OPN(new_false_label)});

          // tl:src fl:src
          this->rhs_.GenerateIR();

          // genir(label,newfalselabel,-,-)
          ir::gIRList.push_back(
              {ir::IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" != false_label) {
          if ("null" == true_label) {
            std::string new_true_label = ir::NewLabel();

            // genir(jne,lhs,0,newtruelabel)
            ir::gIRList.push_back({ir::IR::OpKind::JNE, lhs_opn, IMM_0_OPN,
                                   LABEL_OPN(new_true_label)});

            // tl:null fl:src
            this->rhs_.GenerateIR();

            // genir(label,newtruelabel,-,-)
            ir::gIRList.push_back(
                {ir::IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
          } else {
            // genir(jne,lhs,0,truelabel)
            ir::gIRList.push_back(
                {ir::IR::OpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});

            // tl:src fl:src
            this->rhs_.GenerateIR();
          }
        } else if ("null" != true_label) {
          // genir(jne,lhs,0,truelabel)
          ir::gIRList.push_back(
              {ir::IR::OpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});

          // tl:src fl:src&null
          this->rhs_.GenerateIR();
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      default: {
        std::string rhs_temp_var = ir::NewTemp();
        // insert symboltable 之前一定没有
        int scope_id = ir::gContextInfo.current_scope_id_;
        ir::gScopes[scope_id].symbol_table_.insert(
            {rhs_temp_var, {false, false, ir::gScopes[scope_id].size_}});
        ir::gScopes[scope_id].size_ += ir::kIntWidth;
        ir::Opn rhs_opn = {ir::Opn::Type::Var, rhs_temp_var, scope_id};

        // genir(assign,0,-,rhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_0_OPN, rhs_opn});

        // tl:null fl:nextlabel
        std::string next_label = ir::NewLabel();
        ir::gContextInfo.true_label_.push("null");
        ir::gContextInfo.false_label_.push(next_label);
        this->lhs_.GenerateIR();
        ir::gContextInfo.false_label_.pop();
        ir::gContextInfo.true_label_.pop();

        // genir(assign,1,-,rhstmpvar)
        ir::gIRList.push_back({ir::IR::OpKind::ASSIGN, IMM_1_OPN, rhs_opn});
        // genir(label,nextlabel,-,-)
        ir::gIRList.push_back({ir::IR::OpKind::LABEL, LABEL_OPN(next_label)});

        if ("null" != false_label) {
          // genir(jreverseop,lhs,rhs,falselabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn,
                                 rhs_opn, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn,
                                 rhs_opn, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError(
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
    if (!ir::gContextInfo.shape_.empty()) {
      // NOTE 语义错误 类型错误 条件表达式左边非int类型
      ir::SemanticError(this->line_no_, ir::gContextInfo.opn_.name_ +
                                            ": condexp lhs exp type not int");
    }
    ir::Opn lhs_opn = ir::gContextInfo.opn_;

    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // genir(jeq,lhs,0,falselabel)
          ir::gIRList.push_back(
              {ir::IR::OpKind::JEQ, lhs_opn, IMM_0_OPN, false_label_opn});

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, ir::gContextInfo.opn_,
                                 IMM_0_OPN, false_label_opn});

          if ("null" != true_label) {
            // genir(goto, truelabel)
            ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jeq,lhs,0,newfalselabel)
          std::string new_false_label = ir::NewLabel();
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, lhs_opn, IMM_0_OPN,
                                 LABEL_OPN(new_false_label)});

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          ir::gIRList.push_back({ir::IR::OpKind::JNE, ir::gContextInfo.opn_,
                                 IMM_0_OPN, true_label_opn});

          // genir(label,newfalselabel,-,-)
          ir::gIRList.push_back(
              {ir::IR::OpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
      case OR: {
        if ("null" != false_label) {
          std::string new_true_label = "";
          if ("null" == true_label) {
            new_true_label = ir::NewLabel();
          } else {
            new_true_label = true_label;
          }
          // genir(jne,lhs,0,newtruelabel)
          ir::gIRList.push_back({ir::IR::OpKind::JNE, lhs_opn, IMM_0_OPN,
                                 LABEL_OPN(new_true_label)});

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jeq,rhs,0,falselabel)
          ir::gIRList.push_back({ir::IR::OpKind::JEQ, ir::gContextInfo.opn_,
                                 IMM_0_OPN, false_label_opn});

          if ("null" == true_label) {
            // genir(label,newtruelabel,-,-)
            ir::gIRList.push_back(
                {ir::IR::OpKind::LABEL, LABEL_OPN(new_true_label)});
          } else {
            // genir(goto,newtruelabel,-,-)
            ir::gIRList.push_back(
                {ir::IR::OpKind::GOTO, LABEL_OPN(new_true_label)});
          }
        } else if ("null" != true_label) {
          // genir(jne,lhs,0,truelabel)
          ir::gIRList.push_back(
              {ir::IR::OpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});

          this->rhs_.GenerateIR();
          if (!ir::gContextInfo.shape_.empty()) {
            // NOTE 语义错误 类型错误 条件表达式右边非int类型
            ir::SemanticError(
                this->line_no_,
                ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
          }

          // genir(jne,rhs,0,truelabel)
          ir::gIRList.push_back({ir::IR::OpKind::JNE, ir::gContextInfo.opn_,
                                 IMM_0_OPN, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }

        break;
      }
      default: {  // relop eqop
        this->rhs_.GenerateIR();
        if (!ir::gContextInfo.shape_.empty()) {
          // NOTE 语义错误 类型错误 条件表达式右边非int类型
          ir::SemanticError(
              this->line_no_,
              ir::gContextInfo.opn_.name_ + ": condexp rhs exp type not int");
        }

        if ("null" != false_label) {
          // genir:if False lhs op rhs goto falselabel
          // (jreverseop, lhs, rhs, falselabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn,
                                 ir::gContextInfo.opn_, false_label_opn});

          if ("null" != true_label) {
            // genir(goto, truelabel)
            ir::gIRList.push_back({ir::IR::OpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhs,rhs,truelabel)
          ir::gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn,
                                 ir::gContextInfo.opn_, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError(
              "both true_label and false_label are 'null' in "
              "'ConditionExpression::GenerateIR()'");
        }
        break;
      }
    }
  }
}

}  // namespace ast
