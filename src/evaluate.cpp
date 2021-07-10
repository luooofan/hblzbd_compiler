#include "../include/ast.h"
#include "../include/ir.h"

namespace ast {
// 若计算成功 返回一个立即数形式的ir::Opn
void Number::Evaluate() {
  ir::gContextInfo.opn_ = ir::Opn(ir::Opn::Type::Imm, value_);
}

void Identifier::Evaluate() {
  ir::SymbolTableItem *s;
  s = ir::FindSymbol(ir::gContextInfo.current_scope_id_, name_);
  if (!s) {
    ir::SemanticError(line_no_, name_ + ": undefined variable");
  }

  if (!s->is_const_) {
    ir::SemanticError(line_no_, name_ + ": not const type");
  }

  ir::gContextInfo.opn_ = ir::Opn(ir::Opn::Type::Imm, s->initval_[0]);
}
void ArrayIdentifier::Evaluate() {
  ir::SymbolTableItem *s;
  s = ir::FindSymbol(ir::gContextInfo.current_scope_id_, name_.name_);
  if (!s) {
    ir::SemanticError(line_no_, name_.name_ + ": undefined variable");
  }
  if (shape_list_.size() != s->shape_.size()) {
    ir::SemanticError(line_no_,
                      name_.name_ + ": the dimension of array is not correct");
  }

  int index, t;

  for (int i = 0; i < shape_list_.size(); ++i) {
    shape_list_[i]->Evaluate();
    t = ir::gContextInfo.opn_.imm_num_;
    if (i == shape_list_.size() - 1) {
      index += t * 4;
    } else {
      index += t * s->width_[i + 1];
    }
  }

  ir::gContextInfo.opn_ = ir::Opn(ir::Opn::Type::Imm, s->initval_[index / 4]);
}
void ConditionExpression::Evaluate() {
  ir::RuntimeError("条件表达式不实现Evaluate函数");
}
void BinaryExpression::Evaluate() {
  int l, r, res;

  lhs_.Evaluate();
  l = ir::gContextInfo.opn_.imm_num_;
  rhs_.Evaluate();
  r = ir::gContextInfo.opn_.imm_num_;

  switch (op_) {
    case 258:
      res = l + r;
      break;
    case 259:
      res = l - r;
      break;
    case 261:
      res = l * r;
      break;
    case 262:
      res = l / r;
      break;
    case 263:
      res = l % r;
      break;
    case 270:
      res = l && r;
      break;
    case 271:
      res = l || r;
      break;
    default:
      printf("mystery operator code: %d", op_);
      break;
  }

  ir::gContextInfo.opn_ = ir::Opn(ir::Opn::Type::Imm, res);
}
void UnaryExpression::Evaluate() {
  int r, res;

  rhs_.Evaluate();
  r = ir::gContextInfo.opn_.imm_num_;

  switch (op_) {
    case 258:
      res = r;
      break;
    case 259:
      res = -r;
      break;
    case 260:
      res = !r;
      break;
    default:
      printf("mystery operator code: %d", op_);
      break;
  }

  ir::gContextInfo.opn_ = ir::Opn(ir::Opn::Type::Imm, res);
}
void FunctionCall::Evaluate() {  //返回一个空ir::Opn
}

// 填写initval信息
void ArrayInitVal::Evaluate() {
  auto &symbol_item = (*ir::gSymbolTables[ir::gContextInfo.current_scope_id_]
                            .symbol_table_.find(ir::gContextInfo.array_name_))
                          .second;
  if (this->is_exp_) {
    this->value_->Evaluate();
    if (ir::gContextInfo.opn_.type_ !=
        ir::Opn::Type::Imm) {  // 语义错误 非常量表达式
      ir::SemanticError(this->line_no_,
                        ir::gContextInfo.opn_.name_ + "非常量表达式");
    }

    symbol_item.initval_.push_back(ir::gContextInfo.opn_.imm_num_);
    ++ir::gContextInfo.array_offset_;
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
        this->initval_list_[0]->Evaluate();

        for (int i = 1; i < this->initval_list_.size(); ++i) {
          brace_num = 1;
          this->initval_list_[i]->Evaluate();
        }
      }
      // 补0补到finaloffset
      while (offset < final_offset) {
        symbol_item.initval_.push_back(0);
        ++offset;
      }
      // 语义检查
      if (offset > final_offset) {  // 语义错误 初始值设定项值太多
        ir::SemanticError(this->line_no_, "初始值设定项值太多");
      }
    }
  }
}
}  // namespace ast
