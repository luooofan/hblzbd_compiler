#include "../include/ast.h"
#include "../include/ir.h"
#include "./parser.hpp"  // VOID INT

#define IMM_0_OPN \
  { OpnType::Imm, 0 }
#define IMM_1_OPN \
  { OpnType::Imm, 1 }
#define LABEL_OPN(label_name) \
  { OpnType::Label, label_name }
#define FUNC_OPN(label_name) \
  { OpnType::Func, label_name }
#define OPN_IS_NOT_INT (!ctx.shape_.empty() && ctx.opn_.type_ == OpnType::Null)
#define CHECK_OPN_INT(locstr)                                                            \
  if (OPN_IS_NOT_INT) {                                                                  \
    ir::SemanticError(this->line_no_, ctx.opn_.name_ + ": " + locstr + " type not int"); \
  }

#define ASSERT_ENABLE
#include "../include/myassert.h"

// #define NO_OPT
// useless. it is only work for opt_genarm_regalloc branch. used to generate arm code from quad-ir.

namespace ast {

using OpnType = ir::Opn::Type;
using IROpKind = ir::IR::OpKind;

void Root::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // 把系统库中的函数添加到函数表中
  // NOTE: 库函数size为0
  ir::gFuncTable.insert({"getint", {INT, -1}});
  ir::gFuncTable.insert({"getch", {INT, -1}});
  ir::FuncTableItem func_item = {INT, -1};
  func_item.shape_list_.push_back({-1});
  ir::gFuncTable.insert({"getarray", func_item});
  ir::FuncTableItem func_item_void = {VOID, -1};
  func_item_void.shape_list_.push_back({});
  ir::gFuncTable.insert({"putint", func_item_void});
  ir::gFuncTable.insert({"putch", func_item_void});
  ir::gFuncTable.insert({"_sysy_starttime", func_item_void});
  ir::gFuncTable.insert({"_sysy_stoptime", func_item_void});
  func_item_void.shape_list_.push_back({-1});
  ir::gFuncTable.insert({"putarray", func_item_void});
  ir::FuncTableItem memset_func_item = {VOID, -1};
  memset_func_item.shape_list_.push_back({-1});
  memset_func_item.shape_list_.push_back({});
  memset_func_item.shape_list_.push_back({});
  ir::gFuncTable.insert({"memset", memset_func_item});
  // TODO: string type and putf function
  // ir::gFuncTable.insert({"putf", {VOID}});

  // 创建一张全局符号表 全局作用域id为0 父作用域id为-1 当前dynamic_offset为0
  ir::gScopes.push_back({0, -1, 0});
  // auto &extern_glo_symtab = ir::gScopes[0].symbol_table_;
  // extern_glo_symtab.insert({"_sysy_idx", {false, false, 0}});
  // auto general_extern_array = ir::SymbolTableItem(true, false, 0);
  // general_extern_array.shape_.push_back(1024);
  // general_extern_array.width_.push_back(1024 * 4);
  // general_extern_array.width_.push_back(4);
  // general_extern_array.initval_ = std::vector<int>(1024, 0);
  // extern_glo_symtab.insert({"_sysy_l1", general_extern_array});
  // extern_glo_symtab.insert({"_sysy_l2", general_extern_array});
  // extern_glo_symtab.insert({"_sysy_h", general_extern_array});
  // extern_glo_symtab.insert({"_sysy_m", general_extern_array});
  // extern_glo_symtab.insert({"_sysy_s", general_extern_array});
  // extern_glo_symtab.insert({"_sysy_us", general_extern_array});

  // 初始化上下文信息中的当前作用域id
  ctx.scope_id_ = 0;
  ctx.is_assigned_ = false;

  // 对每个compunit语义分析并生成IR
  for (const auto &ele : this->compunit_list_) {
    ele->GenerateIR(ctx, gIRList);
  }

  // 检查是否有main函数 main函数的返回值是否是int
  auto func_iter = ir::gFuncTable.find("main");
  if (func_iter == ir::gFuncTable.end()) {
    ir::SemanticError(this->line_no_, "no main() function");
  } else if ((*func_iter).second.ret_type_ == VOID) {
    ir::SemanticError(this->line_no_, "main() return type not int");
  }
}

void Number::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // 生成一个imm opn 维护shape
  ctx.opn_ = ir::Opn(OpnType::Imm, value_);
  ctx.shape_.clear();
}

// NOTE: 对于函数名的检查在FunctionCall完成 不调用此函数
void Identifier::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  bool is_assigned = ctx.is_assigned_;
  ctx.is_assigned_ = false;
  ir::SymbolTableItem *s = nullptr;
  int scope_id = ir::FindSymbol(ctx.scope_id_, name_, s);
  if (!s) {
    ir::SemanticError(line_no_, name_ + ": undefined variable");
  }

  // NOTE: 常量直接替换为立即数 作用域为当前作用域
  if (!is_assigned && s->is_const_ && !s->is_array_) {
    ctx.opn_ = ir::Opn(OpnType::Imm, s->initval_[0]);
    ctx.shape_.clear();
  } else {
    ctx.shape_ = s->shape_;
    ctx.opn_ = ir::Opn(OpnType::Var, name_, scope_id);
  }
}

// NOTE:
// 如果是普通情况下取数组元素则shape_list_中的维度必须与符号表中维度相同，
// 但如果是函数调用中使用数组则可以传递指针shape_list_维度小于符号表中的维度即可
// 该检查交由caller来实现
void ArrayIdentifier::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  bool is_assigned = ctx.is_assigned_;
  ctx.is_assigned_ = false;
  ir::SymbolTableItem *s = nullptr;
  int scope_id = ir::FindSymbol(ctx.scope_id_, name_.name_, s);
  if (!s) {
    ir::SemanticError(line_no_, name_.name_ + ": undefined variable");
    return;
  }

  // 结点中存的shape(可能比应有的shape的size小)比符号表中存的shape的size大
  // eg. int a[2][3]; a[1][3][4]=1; 使用数组时维度错误
  if (shape_list_.size() > s->shape_.size()) {
    ir::SemanticError(line_no_, name_.name_ + ": the dimension of array is not correct");
    return;
  }

  // for(int i = 0; i < s->width_.size(); ++i)
  // {
  //   printf("width: %d\n", s->width_[i]);
  // }

  {
    BinaryExpression *res = nullptr;
    BinaryExpression *add_exp;
    BinaryExpression *mul_exp;
    // 计算数组取值的偏移
    if (shape_list_.size() > 0) {
      res = new BinaryExpression(line_no_, MUL, shape_list_[0], *(new Number(line_no_, s->width_[1])));
    }
    for (int i = 1; i < shape_list_.size(); ++i) {
      //构造一个二元表达式
      Number *width = new Number(line_no_, s->width_[i + 1]);
      // printf("width: %d\n", s->width_[i + 1]);
      mul_exp = new BinaryExpression(line_no_, MUL, shape_list_[i], *width);
      add_exp = new BinaryExpression(line_no_, ADD, std::shared_ptr<Expression>(res), *mul_exp);
      res = add_exp;
    }
    if (nullptr != res) {
      res->GenerateIR(ctx, gIRList);
      // res -> PrintNode();
      delete res;
    }
  }

  // 设置当前arrayidentifier的维度
  // eg. int a[2][3][4]; a[0]'s shape is [3,4].
  ctx.shape_.clear();
  ctx.shape_ = std::vector<int>(s->shape_.begin() + shape_list_.size(), s->shape_.end());
  // std::cout << s->shape_.size() << " " << shape_list_.size() << " " << ctx.shape_.size() << std::endl;

  ir::Opn *offset = new ir::Opn(ctx.opn_);
  ctx.opn_ = ir::Opn(OpnType::Array, name_.name_, scope_id, offset);
  if (!is_assigned && ctx.shape_.empty()) {
    if (s->is_const_ && offset->type_ == OpnType::Imm) {  // 常量数组的使用 直接返回一个立即数
      ctx.opn_ = ir::Opn(OpnType::Imm, s->initval_[offset->imm_num_ / 4]);
    } else {
      // 如果是int类型而不是int[] 则需要生成一条=[]ir以区分地址和取值
      std::string res_var = ir::NewTemp();
      int scope_id = ctx.scope_id_;
      ir::gScopes[scope_id].symbol_table_.insert({res_var, {false, false, -1}});
      ir::Opn temp = ir::Opn(OpnType::Var, res_var, scope_id);
      gIRList.push_back({IROpKind::ASSIGN_OFFSET, ctx.opn_, temp});
      ctx.opn_ = temp;
    }
  }
}

// TODO: var+0 0+var var-0 var*0 0*var var/0 var%0 var/1 var%1
void BinaryExpression::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  ir::Opn opn1, opn2;
  IROpKind op;

  lhs_->GenerateIR(ctx, gIRList);
  opn1 = ctx.opn_;
  CHECK_OPN_INT("lhs exp");

  rhs_.GenerateIR(ctx, gIRList);
  opn2 = ctx.opn_;
  CHECK_OPN_INT("rhs exp");

  if (opn1.type_ == OpnType::Imm && opn2.type_ == OpnType::Imm) {
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
        // printf("mystery operator code: %d", op_);
        MyAssert(0);
        break;
    }
    ctx.opn_ = ir::Opn(OpnType::Imm, result);
  } else {
    switch (op_) {
      case ADD:
        op = IROpKind::ADD;
        break;
      case SUB:
        op = IROpKind::SUB;
        break;
      case MUL:
        op = IROpKind::MUL;
        break;
      case DIV:
        op = IROpKind::DIV;
        break;
      case MOD:
        op = IROpKind::MOD;
        break;
      default:
        // printf("mystery operator code: %d", op_);
        MyAssert(0);
        break;
    }
    std::string res_temp_var = ir::NewTemp();
    // NOTE: 中间变量栈偏移-1 并且不维护scope的dynamic offset
    ir::gScopes[ctx.scope_id_].symbol_table_.insert({res_temp_var, {false, false, -1}});

    ir::Opn temp = ir::Opn(OpnType::Var, res_temp_var, ctx.scope_id_);
    ctx.opn_ = temp;
    gIRList.push_back(ir::IR(op, opn1, opn2, temp));
  }
}

// TODO: 根据语义分析 单目运算取非操作 只会出现在条件表达式中
void UnaryExpression::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  rhs_.GenerateIR(ctx, gIRList);
  ir::Opn opn1 = ctx.opn_;
  CHECK_OPN_INT("rhs exp");

  if (opn1.type_ == OpnType::Imm) {
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
        // printf("mystery operator code: %d", op_);
        MyAssert(0);
        break;
    }
    ctx.opn_ = ir::Opn(OpnType::Imm, result);
  } else {
    IROpKind op;
    switch (op_) {
      case ADD:
        // op = IROpKind::POS;
        return;
        break;
      case SUB:
        op = IROpKind::NEG;
        break;
      case NOT:
        op = IROpKind::NOT;
        break;
      default:
        // printf("mystery operator code: %d", op_);
        MyAssert(0);
        break;
    }
    std::string rhs_temp_var = ir::NewTemp();

    // NOTE: 中间变量栈偏移-1 并且不维护scope的dynamic offset
    ir::gScopes[ctx.scope_id_].symbol_table_.insert({rhs_temp_var, {false, false, -1}});

    ir::Opn temp = ir::Opn(OpnType::Var, rhs_temp_var, ctx.scope_id_);
    ctx.opn_ = temp;
    gIRList.push_back(ir::IR(op, opn1, temp));
  }
}

void FunctionCall::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  ir::Opn opn1, opn2;
  auto func_item_iter = ir::gFuncTable.find(name_.name_);
  if (func_item_iter == ir::gFuncTable.end()) {
    ir::SemanticError(line_no_, "调用的函数不存在");
    return;
  }
  auto &func_item = (*func_item_iter).second;
  if (func_item.shape_list_.size() != args_.arg_list_.size()) {
    ir::SemanticError(line_no_, "参数个数不匹配");
    return;
  }

  std::stack<ir::IR> param_list;
  // NOTE: 正着走
  // for (int i = args_.arg_list_.size() - 1; i >= 0; --i) {
  for (int i = 0; i < args_.arg_list_.size(); ++i) {
    args_.arg_list_[i]->GenerateIR(ctx, gIRList);
    if (name_.name_ != "putarray" && name_.name_ != "memset") {  // NOTE: putarray和memset不检查参数的合法性
      if (ctx.shape_.size() != func_item.shape_list_[i].size()) {
        // std::cerr << ctx.shape_.size() << ' ' << func_item.shape_list_[i].size() << std::endl;
        ir::SemanticError(line_no_, "函数调用参数类型不匹配");
        return;
      } else {
        for (int j = 1; j < ctx.shape_.size(); j++) {
          if (ctx.shape_[j] != func_item.shape_list_[i][j]) {
            ir::SemanticError(line_no_, "函数调用参数类型不匹配");
            return;
          }
        }
      }
    }

    ir::SymbolTableItem *s = nullptr;
    int scope_id = ir::FindSymbol(ctx.scope_id_, ctx.opn_.name_, s);
    if (name_.name_ != "putarray" && name_.name_ != "memset" && s && s->is_array_ && s->is_const_) {
      ir::SemanticError(line_no_, "实参不能是const数组");
      return;
    }

    opn1 = ctx.opn_;
    if (s && s->is_array_ && nullptr == opn1.offset_) {
      ir::Opn *offset = new ir::Opn(OpnType::Imm, 0);
      opn1 = ir::Opn(OpnType::Array, opn1.name_, opn1.scope_id_, offset);
    }

    // NOTE: 全局变量需要先赋值到一个temp中
    // FIX: 全局数组不用
    if (0 == scope_id && opn1.type_ == OpnType::Var) {
      std::string temp = ir::NewTemp();
      ir::gScopes[ctx.scope_id_].symbol_table_.insert({temp, {false, false, -1}});
      auto temp_opn = ir::Opn(OpnType::Var, temp, ctx.scope_id_);
      gIRList.push_back({IROpKind::ASSIGN, opn1, temp_opn});
      opn1 = temp_opn;
    }

    param_list.push(ir::IR(IROpKind::PARAM, opn1));
  }

  // NOTE: 倒着写Param语句
  while (!param_list.empty()) {
    gIRList.push_back(param_list.top());
    param_list.pop();
  }

  opn1 = ir::Opn(OpnType::Func, name_.name_);
  opn2 = ir::Opn(OpnType::Imm, func_item.shape_list_.size());

  ir::IR ir;
  // 当call调用的函数返回INT时 生成(call, func, arg-num, temp) 返回VOID时 (call, func, arg-num, -/NULL)
  if (func_item.ret_type_ == INT) {
    auto scope_id = ctx.scope_id_;

    std::string ret_var = ir::NewTemp();
    // NOTE: 中间变量栈偏移-1 并且不维护scope的dynamic offset
    ir::gScopes[scope_id].symbol_table_.insert({ret_var, {false, false, -1}});

    ir::Opn temp = ir::Opn(OpnType::Var, ret_var, scope_id);
    ctx.opn_ = temp;
    ctx.shape_.clear();
    ir = ir::IR(IROpKind::CALL, opn1, opn2, temp);
  } else {
    ir::Opn temp = ir::Opn(OpnType::Null);
    ctx.opn_ = temp;
    ctx.shape_.clear();
    ir = ir::IR(IROpKind::CALL, opn1, opn2, temp);
  }
  // printf("/*********************\n");
  gIRList.push_back(ir);
}

void VariableDefine::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  auto &scope = ir::gScopes[ctx.scope_id_];
  auto &symbol_table = scope.symbol_table_;
  // NOTE: 只需在当前作用域查找即可 找到则变量重定义
  const auto &var_iter = symbol_table.find(this->name_.name_);
  if (var_iter == symbol_table.end()) {
    auto tmp = new ir::SymbolTableItem(false, false, scope.dynamic_offset_);
    if (ctx.scope_id_ == 0) {
      tmp->initval_.push_back(0);
    } else {  // GenIR DECLARE
      // gIRList.push_back({IROpKind::ALLOCA, ir::Opn{OpnType::Var, this->name_.name_, ctx.scope_id_},
      //                        ir::Opn{OpnType::Imm, ir::kIntWidth}});
    }
    symbol_table.insert({this->name_.name_, *tmp});
#ifdef NO_OPT
    scope.dynamic_offset_ += ir::kIntWidth;
#endif
    delete tmp;
  } else {
    ir::SemanticError(this->line_no_, this->name_.name_ + ": variable redefined");
  }
}

void VariableDefineWithInit::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  auto &scope = ir::gScopes[ctx.scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_);
  if (var_iter == symbol_table.end()) {
    if (this->is_const_) {
      symbol_table.insert({this->name_.name_, ir::SymbolTableItem(false, true, -1)});
    } else {
      symbol_table.insert({this->name_.name_, ir::SymbolTableItem(false, false, scope.dynamic_offset_)});
#ifdef NO_OPT
      scope.dynamic_offset_ += ir::kIntWidth;
#endif
    }
    auto &tmp = symbol_table.find(this->name_.name_)->second;
    // NOTE: 如果是全局量或者常量 需要填写initval 如果是局部变量 生成一条赋值语句
    if (ctx.scope_id_ == 0 || this->is_const_) {
      value_.Evaluate(ctx, gIRList);
      tmp.initval_.push_back(ctx.opn_.imm_num_);
    } else {
      value_.GenerateIR(ctx, gIRList);
      ir::Opn lhs_opn = ir::Opn(OpnType::Var, name_.name_, ctx.scope_id_);
      // GenIR DECLARE
      // gIRList.push_back({IROpKind::ALLOCA, lhs_opn, ir::Opn{OpnType::Imm, ir::kIntWidth}});
      gIRList.push_back({IROpKind::ASSIGN, ctx.opn_, lhs_opn});
    }
  } else {
    ir::SemanticError(this->line_no_, this->name_.name_ + ": variable redefined");
  }
}

void ArrayDefine::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  auto &scope = ir::gScopes[ctx.scope_id_];
  auto &symbol_table = scope.symbol_table_;
  // NOTE: 只需在当前作用域查找即可 找到则重定义
  const auto &var_iter = symbol_table.find(this->name_.name_.name_);
  if (var_iter == symbol_table.end()) {
    auto tmp = new ir::SymbolTableItem(true, false, scope.dynamic_offset_);
    // 计算shape和width shape的类型必定为常量表达式，所以opn一定是立即数
    for (const auto &shape : name_.shape_list_) {
      shape->Evaluate(ctx, gIRList);
      tmp->shape_.push_back(ctx.opn_.imm_num_);
    }
    tmp->width_.resize(tmp->shape_.size() + 1);
    // NOTE: 数组的width比shape多一项 最后一项存InitWidth 4
    tmp->width_[tmp->width_.size() - 1] = ir::kIntWidth;
    for (int i = tmp->width_.size() - 2; i >= 0; i--) tmp->width_[i] = tmp->width_[i + 1] * tmp->shape_[i];
    scope.dynamic_offset_ += tmp->width_[0];
    // 全局数组量需要展开数组并填初值0
    if (ctx.scope_id_ == 0) {
      int mul = 1;
      for (int i = 0; i < tmp->shape_.size(); i++) mul *= tmp->shape_[i];
      tmp->initval_.resize(mul, 0);
    } else {  // GenIR ALLOCA
      gIRList.push_back({IROpKind::ALLOCA,
                         ir::Opn{OpnType::Array, this->name_.name_.name_, ctx.scope_id_, new ir::Opn(IMM_0_OPN)},
                         ir::Opn{OpnType::Imm, tmp->width_[0]}});
    }
    symbol_table.insert({name_.name_.name_, *tmp});
    delete tmp;
  } else {
    ir::SemanticError(this->line_no_, this->name_.name_.name_ + ": variable redefined");
  }
}

void ArrayDefineWithInit::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  auto &name = this->name_.name_.name_;
  auto &scope = ir::gScopes[ctx.scope_id_];
  auto &symbol_table = scope.symbol_table_;
  // NOTE: 只需在当前作用域查找即可 找到则重定义
  const auto &array_iter = symbol_table.find(name);
  if (array_iter == symbol_table.end()) {
    // NOTE: 常量数组不一定能删 因为可能会有变量下标
    // if (this->is_const_) {
    // symbol_table.insert({name, ir::SymbolTableItem(true, true, -1)});
    // } else {
    symbol_table.insert({name, ir::SymbolTableItem(true, this->is_const_, scope.dynamic_offset_)});
    // }
    auto &symbol_item = symbol_table.find(name)->second;
    // 填shape
    for (const auto &shape : this->name_.shape_list_) {
      shape->Evaluate(ctx, gIRList);
      // 如果Evaluate没有执行成功会直接退出程序
      // 如果执行成功说明返回的一定是立即数
      symbol_item.shape_.push_back(ctx.opn_.imm_num_);
    }

    // 填width和dim total num
    std::stack<int> temp_width;
    int width = ir::kIntWidth;
    temp_width.push(width);
    for (auto reiter = symbol_item.shape_.rbegin(); reiter != symbol_item.shape_.rend(); ++reiter) {
      width *= *reiter;
      temp_width.push(width);
    }
    // if (!this->is_const_) scope.dynamic_offset_ += temp_width.top();
    scope.dynamic_offset_ += temp_width.top();
    while (!temp_width.empty()) {
      symbol_item.width_.push_back(temp_width.top());
      temp_width.pop();
    }

    // 完成赋值操作 或者 填写initval(global or const)
    // prepare for arrayinitval
    ctx.dim_total_num_.clear();
    for (const auto width : symbol_item.width_) {
      ctx.dim_total_num_.push_back(width / ir::kIntWidth);
    }
    // ctx.dim_total_num_.push_back(1);
    ctx.array_name_ = name;
    ctx.array_offset_ = 0;
    ctx.brace_num_ = 1;

    if (ctx.scope_id_ == 0) {
      this->value_.Evaluate(ctx, gIRList);
    } else {
      // GenIR ALLOCA
      gIRList.push_back({IROpKind::ALLOCA, ir::Opn{OpnType::Array, name, ctx.scope_id_, new ir::Opn(OpnType::Imm, 0)},
                         ir::Opn{OpnType::Imm, symbol_item.width_[0]}});
      gIRList.push_back({IROpKind::PARAM, {OpnType::Imm, symbol_item.width_[0]}});
      gIRList.push_back({IROpKind::PARAM, IMM_0_OPN});
      gIRList.push_back({IROpKind::PARAM, {OpnType::Array, name, ctx.scope_id_, new ir::Opn(OpnType::Imm, 0)}});
      ir::Opn temp = ir::Opn(OpnType::Null);
      // ctx.opn_ = temp;
      // ctx.shape_.clear();
      // TODO: 这里用name会导致段错误
      auto opn1 = ir::Opn(OpnType::Func, std::string("memset"));
      auto opn2 = ir::Opn(OpnType::Imm, 3);
      gIRList.push_back({IROpKind::CALL, opn1, opn2, temp});
      if (this->is_const_) {
        ctx.array_offset_ = 0;
        ctx.brace_num_ = 1;
        this->value_.Evaluate(ctx, gIRList);
      }
      ctx.array_offset_ = 0;
      ctx.brace_num_ = 1;
      this->value_.GenerateIR(ctx, gIRList);
    }
  } else {
    ir::SemanticError(this->line_no_, this->name_.name_.name_ + ": variable redefined");
  }
}

void FunctionDefine::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // NOTE: 函数无法嵌套定义
  if (ctx.scope_id_ != 0) {
    ir::SemanticError(this->line_no_, "function nest define.");
    return;
  }
  const auto &func_iter = ir::gFuncTable.find(this->name_.name_);
  if (func_iter == ir::gFuncTable.end()) {
    ctx.current_func_name_ = this->name_.name_;
    ctx.scope_id_ = ir::gScopes.size();
    gIRList.push_back({IROpKind::LABEL, FUNC_OPN(this->name_.name_)});
    // 父作用域id一定为0 函数初始栈偏移为0
    ir::gScopes.push_back({ctx.scope_id_, 0, 0});
    auto tmp = new ir::FuncTableItem(return_type_, ctx.scope_id_);
    ctx.has_aug_scope = true;
    for (int i = 0; i < args_.arg_list_.size(); ++i) {
      // for (const auto &arg : args_.arg_list_) {
      const auto &arg = args_.arg_list_[i];
      auto &scope = ir::gScopes[ctx.scope_id_];
      auto &symbol_table = scope.symbol_table_;
      if (Identifier *ident = dynamic_cast<Identifier *>(&arg->name_)) {
        const auto &var_iter = symbol_table.find(ident->name_);
        if (var_iter == symbol_table.end()) {
          auto &&symbol_item = ir::SymbolTableItem(false, false, scope.dynamic_offset_);
          symbol_item.SetIsArg();
          symbol_table.insert({ident->name_, symbol_item});
          // #ifdef NO_OPT
          scope.dynamic_offset_ += ir::kIntWidth;
          // #endif
          // ADD DECLARE
          gIRList.push_back(
              {IROpKind::DECLARE, ir::Opn(OpnType::Var, ident->name_, ctx.scope_id_), ir::Opn(OpnType::Imm, i)});
        } else {
          ir::SemanticError(this->line_no_, ident->name_ + ": variable redefined");
        }
        tmp->shape_list_.push_back(std::vector<int>());
      } else if (ArrayIdentifier *arrident = dynamic_cast<ArrayIdentifier *>(&arg->name_)) {
        const auto &var_iter = symbol_table.find(arrident->name_.name_);
        if (var_iter == symbol_table.end()) {
          auto tmp1 = new ir::SymbolTableItem(true, false, scope.dynamic_offset_);
          tmp1->SetIsArg();
          // NOTE: 参数中的数组视为一个基址指针
          // #ifdef NO_OPT
          scope.dynamic_offset_ += ir::kIntWidth;
          // #endif
          // ADD DECLARE
          gIRList.push_back(
              {IROpKind::DECLARE,
               ir::Opn(OpnType::Array, arrident->name_.name_, ctx.scope_id_, new ir::Opn(OpnType::Imm, 0)),
               ir::Opn(OpnType::Imm, i)});

          // 计算shape和width shape:-1 2 3  width:? 24 12 4
          tmp1->shape_.push_back(-1);  //第一维因为文法规定必须留空，这里记-1
          for (const auto &shape : arrident->shape_list_) {
            shape->Evaluate(ctx, gIRList);
            tmp1->shape_.push_back(ctx.opn_.imm_num_);
          }
          tmp1->width_.resize(tmp1->shape_.size() + 1);
          tmp1->width_[tmp1->width_.size() - 1] = ir::kIntWidth;
          for (int i = tmp1->width_.size() - 2; i > 0; i--) tmp1->width_[i] = tmp1->width_[i + 1] * tmp1->shape_[i];
          tmp1->width_[0] = -1;  // useless
          symbol_table.insert({arrident->name_.name_, *tmp1});
          tmp->shape_list_.push_back(tmp1->shape_);
          delete tmp1;
        } else {
          ir::SemanticError(this->line_no_, arrident->name_.name_ + ": variable redefined");
        }
      }
      // std::cout<<"haha"<<ctx.shape_.size()<<'\n';
      // tmp->shape_list_.push_back(ctx.shape_);
    }
    ir::gFuncTable.insert({name_.name_, *tmp});
    delete tmp;

    ctx.has_return = false;
    this->body_.GenerateIR(ctx, gIRList);

    if (return_type_ == INT && ctx.has_return == false) {
      ir::SemanticError(this->line_no_,
                        this->name_.name_ + ": function's return type is INT,but does not return anything");
    }
    // 保证函数执行流最后会有一条ret语句 方便汇编生成
    if (return_type_ == VOID /*&& ctx.has_return == false*/) {
      gIRList.push_back({IROpKind::RET});
    }

    // 填写函数表项中的栈大小size
    const auto &iter = ir::gFuncTable.find(name_.name_);
    (*iter).second.size_ = ir::gScopes[ctx.scope_id_].dynamic_offset_;

    // Set current_scope_id
    ctx.scope_id_ = 0;
  } else {
    ir::SemanticError(this->line_no_, this->name_.name_ + ": function redefined");
  }
}

void AssignStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // 给函数调用赋值过不了语法分析
  this->rhs_.GenerateIR(ctx, gIRList);
  CHECK_OPN_INT("rhs exp");
  ir::Opn rhs_opn = ctx.opn_;

  ctx.is_assigned_ = true;
  this->lhs_.GenerateIR(ctx, gIRList);
  ctx.is_assigned_ = false;
  CHECK_OPN_INT("leftvalue");

  ir::Opn &lhs_opn = ctx.opn_;
  ir::SymbolTableItem *symbol = nullptr;
  ir::FindSymbol(ctx.scope_id_, lhs_opn.name_, symbol);

  // const check
  if (nullptr != symbol && symbol->is_const_) {
    ir::SemanticError(this->line_no_, lhs_opn.name_ + ": constant can't be assigned");
    return;
  }

  // genir(assign,opn1,-,res)
  gIRList.push_back({IROpKind::ASSIGN, rhs_opn, lhs_opn});
}

void IfElseStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  std::string label_next = ir::NewLabel();
  if (nullptr == this->elsestmt_) {
    // if then
    ctx.true_label_.push("null");
    ctx.false_label_.push(label_next);
    this->cond_.GenerateIR(ctx, gIRList);
    ctx.true_label_.pop();
    ctx.false_label_.pop();
    this->thenstmt_.GenerateIR(ctx, gIRList);
  } else {
    // if then else
    std::string label_else = ir::NewLabel();
    ctx.true_label_.push("null");
    ctx.false_label_.push(label_else);
    this->cond_.GenerateIR(ctx, gIRList);
    ctx.true_label_.pop();
    ctx.false_label_.pop();
    this->thenstmt_.GenerateIR(ctx, gIRList);
    // genir(goto,next,-,-)
    gIRList.push_back({IROpKind::GOTO, LABEL_OPN(label_next)});
    // genir(label,else,-,-)
    gIRList.push_back({IROpKind::LABEL, LABEL_OPN(label_else)});
    this->elsestmt_->GenerateIR(ctx, gIRList);
  }
  gIRList.push_back({IROpKind::LABEL, LABEL_OPN(label_next)});
}

void WhileStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  std::string label_next = ir::NewLabel();
  std::string label_begin = ir::NewLabel();
  // genir(label,begin,-,-)
  gIRList.push_back({IROpKind::LABEL, LABEL_OPN(label_begin)});
  ctx.true_label_.push("null");
  ctx.false_label_.push(label_next);
  this->cond_.GenerateIR(ctx, gIRList);
  ctx.true_label_.pop();
  ctx.false_label_.pop();

  // maybe useless
  ctx.break_label_.push(label_next);
  ctx.continue_label_.push(label_begin);

  this->bodystmt_.GenerateIR(ctx, gIRList);
  // genir(goto,begin,-,-)
  gIRList.push_back({IROpKind::GOTO, LABEL_OPN(label_begin)});

  ctx.continue_label_.pop();
  ctx.break_label_.pop();

  gIRList.push_back({IROpKind::LABEL, LABEL_OPN(label_next)});
}

void BreakStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // 检查是否在while循环中
  if (ctx.break_label_.empty()) {
    ir::SemanticError(this->line_no_, "'break' not in while loop");
  } else {
    // genir (goto,breaklabel,-,-)
    gIRList.push_back({IROpKind::GOTO, {OpnType::Label, ctx.break_label_.top()}});
  }
}

void ContinueStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // 检查是否在while循环中
  if (ctx.continue_label_.empty()) {
    ir::SemanticError(this->line_no_, "'continue' not in while loop");
  } else {
    // genir (goto,continuelabel,-,-)
    gIRList.push_back({IROpKind::GOTO, {OpnType::Label, ctx.continue_label_.top()}});
  }
}

void ReturnStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // [return]语句必然出现在函数定义中
  auto &func_name = ctx.current_func_name_;
  const auto &func_iter = ir::gFuncTable.find(func_name);
  if (func_iter == ir::gFuncTable.end()) {
    ir::RuntimeError("没有维护好上下文信息中的函数名");
  } else {
    // 返回类型检查
    int ret_type = (*func_iter).second.ret_type_;
    if (ret_type == VOID) {
      // return ;
      if (nullptr == this->value_) {
        // genir (ret,-,-,-)
        gIRList.push_back({IROpKind::RET});
      } else {
        ir::SemanticError(this->line_no_, "function ret type should be VOID: " + func_name);
      }
    } else {
      // return exp(int);
      if (nullptr == this->value_) {
        ir::SemanticError(this->line_no_, "function ret type should be INT: " + func_name);
        return;
      }
      this->value_->GenerateIR(ctx, gIRList);
      if (ctx.shape_.empty()) {
        // genir (ret,opn,-,-)
        gIRList.push_back({IROpKind::RET, ctx.opn_});
      } else {
        ir::SemanticError(this->line_no_, "function ret type should be INT: " + func_name);
      }
    }
    ctx.has_return = true;
    return;
  }
}

void VoidStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {}

void EvalStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  this->value_.GenerateIR(ctx, gIRList);
}

void Block::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  int parent_scope_id = ctx.scope_id_;
  if (!ctx.has_aug_scope) {
    // new scope
    ctx.scope_id_ = ir::gScopes.size();
    ir::gScopes.push_back({ctx.scope_id_, parent_scope_id, ir::gScopes[parent_scope_id].dynamic_offset_});
  }
  ctx.has_aug_scope = false;
  for (const auto &stmt : this->statement_list_) {
    stmt->GenerateIR(ctx, gIRList);
  }
  if (0 != parent_scope_id) {
    ir::gScopes[parent_scope_id].dynamic_offset_ = ir::gScopes[ctx.scope_id_].dynamic_offset_;
  }
  // src scope
  ctx.scope_id_ = parent_scope_id;
}

void DeclareStatement::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  for (const auto &def : this->define_list_) {
    def->GenerateIR(ctx, gIRList);
  }
}

void ArrayInitVal::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  if (this->is_exp_) {
    this->value_->GenerateIR(ctx, gIRList);
    CHECK_OPN_INT("exp")
    // genir([]=,expvalue,-,arrayname offset)
    if (ctx.opn_.type_ != OpnType::Imm || 0 != ctx.opn_.imm_num_) {
      gIRList.push_back(
          {IROpKind::/*OFFSET_*/ ASSIGN,
           ctx.opn_,
           {OpnType::Array, ctx.array_name_, ctx.scope_id_, new ir::Opn(OpnType::Imm, ctx.array_offset_ * 4)}});
    }
    ctx.array_offset_ += 1;
  } else {
    // 此时该结点表示一对大括号 {}
    int &offset = ctx.array_offset_;
    int &brace_num = ctx.brace_num_;

    // 根据offset和brace_num算出finaloffset
    int temp_level = 0;
    const auto &dim_total_num = ctx.dim_total_num_;
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
        this->initval_list_[0]->GenerateIR(ctx, gIRList);

        for (int i = 1; i < this->initval_list_.size(); ++i) {
          brace_num = 1;
          this->initval_list_[i]->GenerateIR(ctx, gIRList);
        }
      }
      if (offset > final_offset) {
        ir::SemanticError(this->line_no_, "初始值设定项值太多");
      }
      offset = final_offset;
    }
  }
}

void FunctionFormalParameter::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {}

void FunctionFormalParameterList::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {}

void FunctionActualParameterList::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {}

// NOTE:
// 只有if和while中会用到Cond
// 条件表达式并不会维护上下文中的Opn和Shape 所有的条件都会转化为跳转语句执行
// 由于考虑了
//  1.lhs和rhs是CondExp还是AddExp 如果是AddExp会生成结果赋值为0或1的四元式
//  2.操作符是And Or 还是其他
//  3.truelabel和falselabel是否可为空
//  4.对操作数是立即数的情况做了简化处理
// 导致代码量过多
void ConditionExpression::GenerateIR(ir::ContextInfo &ctx, std::vector<ir::IR> &gIRList) {
  // this->PrintNode(0, std::clog);
  bool lhs_is_cond = nullptr != dynamic_cast<ConditionExpression *>(&this->lhs_);
  bool rhs_is_cond = nullptr != dynamic_cast<ConditionExpression *>(&this->rhs_);

  std::string &true_label = ctx.true_label_.top();
  std::string &false_label = ctx.false_label_.top();
  ir::Opn true_label_opn = LABEL_OPN(true_label);
  ir::Opn false_label_opn = LABEL_OPN(false_label);

  auto gen_tlfl_ir = [&ctx, &gIRList](const std::string &tl, const std::string &fl, Expression &hs) {
    ctx.true_label_.push(tl);
    ctx.false_label_.push(fl);
    hs.GenerateIR(ctx, gIRList);
    ctx.true_label_.pop();
    ctx.false_label_.pop();
  };

  auto gen_tl_ir = [&ctx, &gIRList](const std::string &tl, Expression &hs) {
    ctx.true_label_.push(tl);
    hs.GenerateIR(ctx, gIRList);
    ctx.true_label_.pop();
  };

  auto gen_fl_ir = [&ctx, &gIRList](const std::string &fl, Expression &hs) {
    ctx.false_label_.push(fl);
    hs.GenerateIR(ctx, gIRList);
    ctx.false_label_.pop();
  };

  auto gen_ir = [&ctx, &gIRList](Expression &hs) { hs.GenerateIR(ctx, gIRList); };

  if (lhs_is_cond && rhs_is_cond) {
    // std::clog << "t t:" << true_label << " " << false_label << std::endl;
    // lhs_和rhs_都是CondExp
    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // truelabel:null falselabel:src
          gen_tl_ir("null", this->lhs_);

          // truelabel:src falselabel:src
          gen_ir(this->rhs_);
        } else if ("null" != true_label) {
          // truelabel:null falselabel:new
          std::string new_false_label = ir::NewLabel();
          gen_tlfl_ir("null", new_false_label, this->lhs_);

          // truelabel:src falselabel:src
          gen_ir(this->rhs_);

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          // ERROR
          ir::RuntimeError("both true_label and false_label are 'null' in case1-ADD");
        }
        break;
      }
      case OR: {
        if ("null" == true_label && "null" != false_label) {
          // truelabel:new falselabel:null
          std::string new_true_label = ir::NewLabel();
          gen_tlfl_ir(new_true_label, "null", this->lhs_);

          // tl:src fl:src
          gen_ir(this->rhs_);

          // genir(label,newtruelabel,-,-)
          gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_true_label)});
        } else if ("null" != true_label && "null" == false_label) {
          // truelabel:src falselabel:null
          gen_ir(this->lhs_);

          // truelabel:src falselabel:src
          gen_ir(this->rhs_);
        } else if ("null" != true_label && "null" != false_label) {
          // truelabel:src falselabel:null
          gen_fl_ir("null", this->lhs_);

          // truelabel:src falselabel:src
          gen_ir(this->rhs_);
        } else {
          // ERROR
          ir::RuntimeError("both true_label and false_label are 'null' in case1-OR");
        }
        break;
      }
      default: {
        int scope_id = ctx.scope_id_;
        // NOTE: lhs和rhs所在作用域一致
        auto &scope = ir::gScopes[scope_id];

        std::string lhs_temp_var = ir::NewTemp();
        scope.symbol_table_.insert({lhs_temp_var, {false, false, -1}});
        ir::Opn lhs_opn = {OpnType::Var, lhs_temp_var, scope_id};

        // genir(assign,0,-,lhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_0_OPN, lhs_opn});

        // tl:null fl:rhslabel
        std::string rhs_label = ir::NewLabel();
        gen_tlfl_ir("null", rhs_label, this->lhs_);

        // genir(assign,1,-,lhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_1_OPN, lhs_opn});
        // genir(label,rhslabel,-,-)
        gIRList.push_back({IROpKind::LABEL, LABEL_OPN(rhs_label)});

        std::string rhs_temp_var = ir::NewTemp();
        scope.symbol_table_.insert({rhs_temp_var, {false, false, -1}});
        ir::Opn rhs_opn = {OpnType::Var, rhs_temp_var, scope_id};

        // genir(assign,0,-,rhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_0_OPN, rhs_opn});

        // tl:null fl:nextlabel
        std::string next_label = ir::NewLabel();
        gen_tlfl_ir("null", next_label, this->rhs_);

        // genir(assign,1,-,rhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_1_OPN, rhs_opn});
        // genir(label,nextlabel,-,-)
        gIRList.push_back({IROpKind::LABEL, LABEL_OPN(next_label)});

        if ("null" != false_label) {
          // genir(jreverseop,lhstempvar,rhstempvar,falselabel)
          gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn, rhs_opn, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            gIRList.push_back({IROpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn, rhs_opn, true_label_opn});
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case1-Default");
        }
        break;
      }
    }
  } else if (lhs_is_cond && !rhs_is_cond) {
    // std::clog << "t f:" << true_label << " " << false_label << std::endl;
    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // truelabel:null falselabel:src
          gen_tl_ir("null", this->lhs_);

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp")
          auto &rhs_opn = ctx.opn_;

          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ == 0) {
              // genir(goto,falselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,rhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, rhs_opn, IMM_0_OPN, false_label_opn});
          }

          if ("null" != true_label) {
            // genir(goto, truelabel)
            gIRList.push_back({IROpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // truelabel:null falselabel:new
          std::string new_false_label = ir::NewLabel();
          gen_tlfl_ir("null", new_false_label, this->lhs_);

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp")
          auto &rhs_opn = ctx.opn_;

          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ != 0) {
              // genir(goto,truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            // genir(jne,rhs,0,truelabel)
            gIRList.push_back({IROpKind::JNE, rhs_opn, IMM_0_OPN, true_label_opn});
          }
          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case2-ADD");
        }
        break;
      }
      case OR: {
        if ("null" == true_label && "null" != false_label) {
          // truelabel:new falselabel:null
          std::string new_true_label = ir::NewLabel();
          gen_tlfl_ir(new_true_label, "null", this->lhs_);

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp")
          auto &rhs_opn = ctx.opn_;

          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ == 0) {
              // genir(goto,falselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,rhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, rhs_opn, IMM_0_OPN, false_label_opn});
          }

          // genir(label,newtruelabel,-,-)
          gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_true_label)});
        } else if ("null" != true_label && "null" == false_label) {
          // truelabel:src falselabel:null
          gen_ir(this->lhs_);

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp")
          auto &rhs_opn = ctx.opn_;

          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ != 0) {
              // genir(goto,truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            // genir(jne,rhs,0,truelabel)
            gIRList.push_back({IROpKind::JNE, rhs_opn, IMM_0_OPN, true_label_opn});
          }
        } else if ("null" != true_label && "null" != false_label) {
          // truelabel:src falselabel:null
          gen_fl_ir("null", this->lhs_);

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp")
          auto &rhs_opn = ctx.opn_;

          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ == 0) {
              // genir(goto,falselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,rhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, rhs_opn, IMM_0_OPN, false_label_opn});
          }

          // genir(goto,truelabel)
          gIRList.push_back({IROpKind::GOTO, true_label_opn});
        } else {
          // ERROR
          ir::RuntimeError("both true_label and false_label are 'null' in case2-OR");
        }
        break;
      }
      default: {
        std::string lhs_temp_var = ir::NewTemp();
        int scope_id = ctx.scope_id_;
        auto &scope = ir::gScopes[scope_id];
        scope.symbol_table_.insert({lhs_temp_var, {false, false, -1}});
        ir::Opn lhs_opn = {OpnType::Var, lhs_temp_var, scope_id};

        // genir(assign,0,-,lhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_0_OPN, lhs_opn});

        // tl:null fl:rhslabel
        std::string rhs_label = ir::NewLabel();
        gen_tlfl_ir("null", rhs_label, this->lhs_);

        // genir(assign,1,-,lhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_1_OPN, lhs_opn});
        // genir(label,rhslabel,-,-)
        gIRList.push_back({IROpKind::LABEL, LABEL_OPN(rhs_label)});

        gen_ir(this->rhs_);
        CHECK_OPN_INT("condexp rhs exp")

        if ("null" != false_label) {
          // genir(jreverseop,lhstempvar,rhs,falselabel)
          gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn, ctx.opn_, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            gIRList.push_back({IROpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn, ctx.opn_, true_label_opn});
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case2-Default");
        }
        break;
      }
    }
  } else if (!lhs_is_cond && rhs_is_cond) {
    // std::clog << "f t:" << true_label << " " << false_label << std::endl;
    gen_ir(this->lhs_);
    CHECK_OPN_INT("condexp lhs exp");
    ir::Opn lhs_opn = ctx.opn_;
    // maybe a imm

    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ == 0) {
              // genir(goto,flaselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,lhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, lhs_opn, IMM_0_OPN, false_label_opn});
          }

          // tl:src fl:src
          gen_ir(this->rhs_);
        } else if ("null" != true_label) {
          std::string new_false_label = ir::NewLabel();
          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ == 0) {
              // genir(goto,newflaselabel)
              gIRList.push_back({IROpKind::GOTO, LABEL_OPN(new_false_label)});
            }
          } else {
            // genir(jeq,lhs,0,newfalselabel)
            gIRList.push_back({IROpKind::JEQ, lhs_opn, IMM_0_OPN, LABEL_OPN(new_false_label)});
          }

          // tl:src fl:src
          gen_ir(this->rhs_);

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case3-ADD");
        }
        break;
      }
      case OR: {
        if ("null" != false_label) {
          if ("null" == true_label) {
            std::string new_true_label = ir::NewLabel();
            if (lhs_opn.type_ == OpnType::Imm) {
              if (lhs_opn.imm_num_ != 0) {
                // genir(goto,newtruelabel)
                gIRList.push_back({IROpKind::GOTO, LABEL_OPN(new_true_label)});
              }
            } else {
              // genir(jne,lhs,0,newtruelabel)
              gIRList.push_back({IROpKind::JNE, lhs_opn, IMM_0_OPN, LABEL_OPN(new_true_label)});
            }
            // tl:null fl:src
            gen_ir(this->rhs_);

            // genir(label,newtruelabel,-,-)
            gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_true_label)});
          } else {
            if (lhs_opn.type_ == OpnType::Imm) {
              if (lhs_opn.imm_num_ != 0) {
                // genir(goto,truelabel)
                gIRList.push_back({IROpKind::GOTO, true_label_opn});
              }
            } else {
              // genir(jne,lhs,0,truelabel)
              gIRList.push_back({IROpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});
            }
            // tl:src fl:src
            gen_ir(this->rhs_);
          }
        } else if ("null" != true_label) {
          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ != 0) {
              // genir(goto,truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            // genir(jne,lhs,0,truelabel)
            gIRList.push_back({IROpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});
          }
          // tl:src fl:src&null
          gen_ir(this->rhs_);
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case3-OR");
        }
        break;
      }
      default: {
        std::string rhs_temp_var = ir::NewTemp();
        int scope_id = ctx.scope_id_;
        auto &scope = ir::gScopes[scope_id];
        scope.symbol_table_.insert({rhs_temp_var, {false, false, -1}});
        ir::Opn rhs_opn = {OpnType::Var, rhs_temp_var, scope_id};

        // genir(assign,0,-,rhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_0_OPN, rhs_opn});

        // tl:null fl:nextlabel
        std::string next_label = ir::NewLabel();
        gen_tlfl_ir("null", next_label, this->rhs_);

        // genir(assign,1,-,rhstmpvar)
        gIRList.push_back({IROpKind::ASSIGN, IMM_1_OPN, rhs_opn});
        // genir(label,nextlabel,-,-)
        gIRList.push_back({IROpKind::LABEL, LABEL_OPN(next_label)});

        if ("null" != false_label) {
          // genir(jreverseop,lhs,rhs,falselabel)
          gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn, rhs_opn, false_label_opn});
          if ("null" != true_label) {
            // genir(goto,truelabel)
            gIRList.push_back({IROpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          // genir(jop,lhstempvar,rhstempvar,truelabel)
          gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn, rhs_opn, true_label_opn});
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case3-Default");
        }
        break;
      }
    }
  } else {
    // std::clog << "f f:" << true_label << " " << false_label << std::endl;
    // lhs_和rhs_都不是CondExp
    gen_ir(this->lhs_);
    CHECK_OPN_INT("condexp lhs exp");
    ir::Opn lhs_opn = ctx.opn_;
    // maybe all imms

    switch (this->op_) {
      case AND: {
        if ("null" != false_label) {
          // NOTE: optional optimize. simplify imm process.
          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ == 0) {
              // genir(goto,flaselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,lhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, lhs_opn, IMM_0_OPN, false_label_opn});
          }

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp");

          auto &rhs_opn = ctx.opn_;
          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ == 0) {
              // genir(goto,flaselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,rhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, rhs_opn, IMM_0_OPN, false_label_opn});
          }

          if ("null" != true_label) {
            // genir(goto, truelabel)
            gIRList.push_back({IROpKind::GOTO, true_label_opn});
          }
        } else if ("null" != true_label) {
          std::string new_false_label = ir::NewLabel();
          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ == 0) {
              // genir(goto newfalselabel).
              gIRList.push_back({IROpKind::GOTO, LABEL_OPN(new_false_label)});
            }
          } else {
            // genir(jeq,lhs,0,newfalselabel)
            gIRList.push_back({IROpKind::JEQ, lhs_opn, IMM_0_OPN, LABEL_OPN(new_false_label)});
          }

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp");

          auto &rhs_opn = ctx.opn_;
          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ != 0) {
              // genir(goto,truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            // genir(jne,rhs,0,truelabel)
            gIRList.push_back({IROpKind::JNE, rhs_opn, IMM_0_OPN, true_label_opn});
          }

          // genir(label,newfalselabel,-,-)
          gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_false_label)});
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case4-ADD");
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

          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ != 0) {
              // genir(goto,newtruelabel)
              gIRList.push_back({IROpKind::GOTO, LABEL_OPN(new_true_label)});
            }
          } else {
            // genir(jne,lhs,0,newtruelabel)
            gIRList.push_back({IROpKind::JNE, lhs_opn, IMM_0_OPN, LABEL_OPN(new_true_label)});
          }

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp");

          auto &rhs_opn = ctx.opn_;
          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ == 0) {
              // genir(goto,falselabel)
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            }
          } else {
            // genir(jeq,rhs,0,falselabel)
            gIRList.push_back({IROpKind::JEQ, rhs_opn, IMM_0_OPN, false_label_opn});
          }

          if ("null" == true_label) {
            // genir(label,newtruelabel,-,-)
            gIRList.push_back({IROpKind::LABEL, LABEL_OPN(new_true_label)});
          } else {
            // genir(goto,newtruelabel,-,-)
            gIRList.push_back({IROpKind::GOTO, LABEL_OPN(new_true_label)});
          }
        } else if ("null" != true_label) {
          if (lhs_opn.type_ == OpnType::Imm) {
            if (lhs_opn.imm_num_ != 0) {
              // genir(goto,truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            // genir(jne,lhs,0,truelabel)
            gIRList.push_back({IROpKind::JNE, lhs_opn, IMM_0_OPN, true_label_opn});
          }

          gen_ir(this->rhs_);
          CHECK_OPN_INT("condexp rhs exp");
          auto &rhs_opn = ctx.opn_;
          if (rhs_opn.type_ == OpnType::Imm) {
            if (rhs_opn.imm_num_ != 0) {
              // genir(goto,truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            // genir(jne,rhs,0,truelabel)
            gIRList.push_back({IROpKind::JNE, rhs_opn, IMM_0_OPN, true_label_opn});
          }
        } else {
          ir::RuntimeError("both true_label and false_label are 'null' in case4-OR");
        }
        break;
      }
      default: {  // relop eqop
        gen_ir(this->rhs_);
        CHECK_OPN_INT("condexp rhs exp");
        auto &rhs_opn = ctx.opn_;

        if (lhs_opn.type_ == OpnType::Imm && rhs_opn.type_ == OpnType::Imm) {
          bool res = false;
          switch (this->op_) {
            case EQ:
              res = lhs_opn.imm_num_ == rhs_opn.imm_num_;
              break;
            case NE:
              res = lhs_opn.imm_num_ != rhs_opn.imm_num_;
              break;
            case LT:
              res = lhs_opn.imm_num_ < rhs_opn.imm_num_;
              break;
            case LE:
              res = lhs_opn.imm_num_ <= rhs_opn.imm_num_;
              break;
            case GT:
              res = lhs_opn.imm_num_ > rhs_opn.imm_num_;
              break;
            case GE:
              res = lhs_opn.imm_num_ >= rhs_opn.imm_num_;
              break;
            default:
              ir::RuntimeError("unidentified relop.");
              break;
          }
          if ("null" != false_label) {
            // genir:if False lhs op rhs goto falselabel
            // (jreverseop, lhs, rhs, falselabel)
            if (!res) {
              gIRList.push_back({IROpKind::GOTO, false_label_opn});
            } else if ("null" != true_label) {
              // NOTE:
              // 如果res为假并且truelabel非null 则不会产生跳转到tl的goto语句
              // genir(goto, truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else if ("null" != true_label) {
            if (res) {
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else {
            ir::RuntimeError("both true_label and false_label are 'null' in case4-Default");
          }
        } else {
          if ("null" != false_label) {
            // genir:if False lhs op rhs goto falselabel
            // (jreverseop, lhs, rhs, falselabel)
            gIRList.push_back({ir::GetOpKind(this->op_, true), lhs_opn, rhs_opn, false_label_opn});

            if ("null" != true_label) {
              // genir(goto, truelabel)
              gIRList.push_back({IROpKind::GOTO, true_label_opn});
            }
          } else if ("null" != true_label) {
            // genir(jop,lhs,rhs,truelabel)
            gIRList.push_back({ir::GetOpKind(this->op_, false), lhs_opn, rhs_opn, true_label_opn});
          } else {
            ir::RuntimeError("both true_label and false_label are 'null' in case4-Default");
          }
        }
        break;
      }
    }
  }
}

}  // namespace ast

#undef ASSERT_ENABLE