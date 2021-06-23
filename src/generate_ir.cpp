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
}
void Number::GenerateIR() {
  //生成一个opn 维护shape 不生成目标代码
  gContextInfo.opn_ = Opn(Opn::Type::Imm, this->value_);
  gContextInfo.shape_.clear();
}
void Identifier::GenerateIR() {
  const auto ident_iter =
      FindSymbol(gContextInfo.current_scope_id_, this->name_);
  if (nullptr == ident_iter) {
    SemanticError(this->line_no_, this->name_ + ": variable undefined");
  }

  gContextInfo.shape_.clear();
  gContextInfo.opn_ =
      Opn(Opn::Type::Var, this->name_, gContextInfo.current_scope_id_);
}
void ArrayIdentifier::GenerateIR() {}

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

void BinaryExpression::GenerateIR() {
  this->lhs_.GenerateIR();
  if (!gContextInfo.shape_.empty()) {
    SemanticError(this->line_no_, "type not int");
  }
  Opn lhs_opn = gContextInfo.opn_;

  this->rhs_.GenerateIR();
  if (!gContextInfo.shape_.empty()) {
    SemanticError(this->line_no_, "type not int");
  }
  Opn rhs_opn = gContextInfo.opn_;

  std::string temp_res = NewTemp();
  const int &scope_id = gContextInfo.current_scope_id_;
  auto &scope = gSymbolTables[scope_id];
  scope.symbol_table_.insert({temp_res, {false, false, scope.size_}});
  scope.size_ += kIntWidth;

  gContextInfo.shape_.clear();
  gContextInfo.opn_ = Opn(Opn::Type::Var, temp_res, scope_id);

  switch (this->op_) {
    case ADD:
      gIRList.push_back({IR::OpKind::ADD, lhs_opn, rhs_opn, gContextInfo.opn_});
      break;
    case SUB:
      gIRList.push_back({IR::OpKind::SUB, lhs_opn, rhs_opn, gContextInfo.opn_});
      break;
  }
}
void UnaryExpression::GenerateIR() {}
void FunctionCall::GenerateIR() {}

void VariableDefine::GenerateIR() {
  // TODO 未完成
  auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &var_iter = symbol_table.find(this->name_.name_);
  if (var_iter == symbol_table.end()) {
    symbol_table.insert({this->name_.name_, {false, false, scope.size_}});
    scope.size_ += kIntWidth;
    // gContextInfo.shape_.clear();
    // gContextInfo.opn_ =
    //     Opn(Opn::Type::Var, this->name_.name_,
    //     gContextInfo.current_scope_id_);
  } else {
    SemanticError(this->line_no_, this->name_.name_ + ": variable redefined");
  }
}
void VariableDefineWithInit::GenerateIR() {}
void ArrayDefine::GenerateIR() {}
void ArrayDefineWithInit::GenerateIR() {
  // TODO 未完成
  auto &name = this->name_.name_.name_;
  auto &scope = gSymbolTables[gContextInfo.current_scope_id_];
  auto &symbol_table = scope.symbol_table_;
  const auto &array_iter = symbol_table.find(name);
  if (array_iter == symbol_table.end()) {
    // 调用arrayindent的inserttable函数插入到符号表中 需要传递const信息
    // 在这里实现
    SymbolTableItem symbol_item(true, this->is_const_, scope.size_);
    // 填shape
    for (const auto &shape : this->name_.shape_list_) {
      shape->Evaluate();
      if (gContextInfo.opn_.type_ != Opn::Type::Imm) {
        // 语义错误 数组维度应为常量表达式
        return;
      } else {
        symbol_item.shape_.push_back(gContextInfo.opn_.imm_num_);
      }
    }
    // 填width和dim total num
    std::stack<int> temp_width;
    int width = 4;
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
    if (this->is_const_ || gContextInfo.current_scope_id_ == 0) {
      this->value_.Evaluate();
    } else {
      this->value_.GenerateIR();
    }
  }
}
void FunctionDefine::GenerateIR() {
  // TODO 未完成
  const auto &func_iter = gFuncTable.find(this->name_.name_);
  if (func_iter == gFuncTable.end()) {
    gFuncTable.insert({this->name_.name_, this->return_type_});
    gContextInfo.current_func_name_ = this->name_.name_;
    // genir(label,funcname,-,-)
    gIRList.push_back({IR::OpKind::LABEL, LABEL_OPN(this->name_.name_)});
    this->body_.GenerateIR();
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
    if (!gContextInfo.shape_.empty()) {
      // not int
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
  }
}

void VoidStatement::GenerateIR() {}

void EvalStatement::GenerateIR() { this->value_.GenerateIR(); }

void Block::GenerateIR() {
  // new scope
  int parent_scope_id = gContextInfo.current_scope_id_;
  gContextInfo.current_scope_id_ = gSymbolTables.size();
  gSymbolTables.push_back({gContextInfo.current_scope_id_, parent_scope_id});
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
    gIRList.push_back({IR::OpKind::OFFSET_ASSIGN,
                       gContextInfo.opn_,
                       {Opn::Type::Var, gContextInfo.array_name_},
                       gContextInfo.array_offset_ * 4});

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
        gIRList.push_back({IR::OpKind::OFFSET_ASSIGN,
                           IMM_0_OPN,
                           {Opn::Type::Var, gContextInfo.array_name_},
                           (offset++ * 4)});
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

}  // namespace ast
