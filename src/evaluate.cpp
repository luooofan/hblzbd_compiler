#include "../include/ast.h"
#include "../include/ir.h"

namespace ast {
// 若计算成功 返回一个立即数形式的Opn
void Number::Evaluate() {
  gContextInfo.opn_ = Opn(Opn::Type::Imm, this->value_);
  gContextInfo.shape_.clear();
}
void Identifier::Evaluate() {}
void ArrayIdentifier::Evaluate() {}
void ConditionExpression::Evaluate() {
  RuntimeError("条件表达式不实现Evaluate函数");
}
void BinaryExpression::Evaluate() {}
void UnaryExpression::Evaluate() {}
void FunctionCall::Evaluate() {  //返回一个空Opn
}

// 填写initval信息
void ArrayInitVal::Evaluate() {
  auto &symbol_item =
      (*gSymbolTables[gContextInfo.current_scope_id_].symbol_table_.find(
           gContextInfo.array_name_))
          .second;
  if (this->is_exp_) {
    this->value_->Evaluate();
    if (gContextInfo.opn_.type_ != Opn::Type::Imm) {  // 语义错误 非常量表达式
      SemanticError(this->line_no_, gContextInfo.opn_.name_ + "非常量表达式");
    }

    symbol_item.initval_.push_back(gContextInfo.opn_.imm_num_);
    ++gContextInfo.array_offset_;
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
        SemanticError(this->line_no_, "初始值设定项值太多");
      }
    }
  }
}
}  // namespace ast