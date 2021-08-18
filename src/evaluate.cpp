#include "../include/ast.h"
#include "../include/ir.h"

#define ASSERT_ENABLE
#include "../include/myassert.h"

namespace ast {

// 若计算成功 返回一个立即数形式的ir::Opn
void Number::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  ctx.opn_ = ir::Opn(ir::Opn::Type::Imm, value_);
}

void Identifier::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  ir::SymbolTableItem* s = nullptr;
  int scope_id = ir::FindSymbol(ctx.scope_id_, name_, s);
  if (!s) {
    ir::SemanticError(line_no_, name_ + ": undefined variable");
  }

  if (!s->is_const_) {
    ir::SemanticError(line_no_, name_ + ": not const type");
  }

  ctx.opn_ = ir::Opn(ir::Opn::Type::Imm, s->initval_[0]);
}
void ArrayIdentifier::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  ir::SymbolTableItem* s = nullptr;
  int scope_id = ir::FindSymbol(ctx.scope_id_, name_.name_, s);
  if (!s) {
    ir::SemanticError(line_no_, name_.name_ + ": undefined variable");
  }
  if (shape_list_.size() != s->shape_.size()) {
    ir::SemanticError(line_no_, name_.name_ + ": the dimension of array is not correct");
  }

  int index, t;

  for (int i = 0; i < shape_list_.size(); ++i) {
    shape_list_[i]->Evaluate(ctx, gIRList);
    t = ctx.opn_.imm_num_;
    if (i == shape_list_.size() - 1) {
      index += t * 4;
    } else {
      index += t * s->width_[i + 1];
    }
  }

  ctx.opn_ = ir::Opn(ir::Opn::Type::Imm, s->initval_[index / 4]);
}
void ConditionExpression::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  ir::RuntimeError("条件表达式不实现Evaluate函数");
}
void BinaryExpression::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  int l, r, res;

  // lhs_.Evaluate(ctx, gIRList);
  lhs_->Evaluate(ctx, gIRList);
  l = ctx.opn_.imm_num_;
  rhs_.Evaluate(ctx, gIRList);
  r = ctx.opn_.imm_num_;

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
      // printf("mystery operator code: %d", op_);
      MyAssert(0);
      break;
  }

  ctx.opn_ = ir::Opn(ir::Opn::Type::Imm, res);
}
void UnaryExpression::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  int r, res;

  rhs_.Evaluate(ctx, gIRList);
  r = ctx.opn_.imm_num_;

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
      // printf("mystery operator code: %d", op_);
      MyAssert(0);
      break;
  }

  ctx.opn_ = ir::Opn(ir::Opn::Type::Imm, res);
}
void FunctionCall::Evaluate(ir::ContextInfo& ctx,
                            std::vector<ir::IR*>& gIRList) {  // 不应该试图对Function调用evaluate函数
  MyAssert(0);
}

// 填写initval信息
void ArrayInitVal::Evaluate(ir::ContextInfo& ctx, std::vector<ir::IR*>& gIRList) {
  auto& symbol_item = (*ir::gScopes[ctx.scope_id_].symbol_table_.find(ctx.array_name_)).second;
  if (this->is_exp_) {
    this->value_->Evaluate(ctx, gIRList);
    if (ctx.opn_.type_ != ir::Opn::Type::Imm) {  // 语义错误 非常量表达式
      ir::SemanticError(this->line_no_, ctx.opn_.name_ + "非常量表达式");
    }

    symbol_item.initval_.push_back(ctx.opn_.imm_num_);
    ++ctx.array_offset_;
  } else {
    // 此时该结点表示一对大括号 {}
    int& offset = ctx.array_offset_;
    int& brace_num = ctx.brace_num_;

    // 根据offset和brace_num算出finaloffset
    int temp_level = 0;
    const auto& dim_total_num = ctx.dim_total_num_;
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
        this->initval_list_[0]->Evaluate(ctx, gIRList);

        for (int i = 1; i < this->initval_list_.size(); ++i) {
          brace_num = 1;
          this->initval_list_[i]->Evaluate(ctx, gIRList);
        }
      }
      // 补0补到finaloffset
      // while (offset < final_offset) {
      //   symbol_item.initval_.push_back(0);
      //   ++offset;
      // }
      // 语义检查
      if (offset > final_offset) {  // 语义错误 初始值设定项值太多
        ir::SemanticError(this->line_no_, "初始值设定项值太多");
      }
      symbol_item.initval_.insert(symbol_item.initval_.end(), final_offset - offset, 0);
      offset = final_offset;
    }
  }
}
}  // namespace ast

#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif