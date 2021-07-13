#include "../include/ast.h"

#include "parser.hpp"

#define PRINT_INDENT PrintIndentation(indentation, outfile);
#define PRINT_NODE PrintNode(indentation + 1, outfile);
#define LINE_NO " (" << this->line_no_ << ") "
#define TYPE_STRING(type) \
  ((type) == INT ? "int" : ((type) == VOID ? "void" : "undefined"))
#define OP_STRING(op) (kOpStr[(op)-258])

static const std::string kOpStr[] = {"+",  "-", "!",  "*",  "/",  "%",  "<",
                                     "<=", ">", ">=", "==", "!=", "&&", "||"};

namespace ast {

void PrintIndentation(int indentation, std::ostream &outfile) {
  for (int i = 0; i < indentation; ++i) {
    outfile << "|   ";
  }
  outfile << "|-> ";
}

// ==================== Root Class ====================
Root::Root(int line_no) : line_no_(line_no) {}
Root::~Root() {
  for (auto &ptr : compunit_list_) {
    delete ptr;
  }
}
void Root::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Root" << LINE_NO << std::endl;
  for (const auto &ele : compunit_list_) {
    ele->PRINT_NODE;
  }
}

// ==================== Expression Class Hierarchy ====================
Expression::Expression(int line_no) : line_no_(line_no) {}
// Expression::~Expression() {}

Number::Number(int line_no, int value) : Expression(line_no), value_(value) {}
Number::~Number() {}
void Number::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Number: " << value_ << LINE_NO << std::endl;
}

LeftValue::LeftValue(int line_no) : Expression(line_no) {}
// LeftValue::~LeftValue() {}

Identifier::Identifier(int line_no, std::string &name)
    : LeftValue(line_no), name_(name) {}
Identifier::~Identifier() { delete &name_; }
void Identifier::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Identifier: " << name_ << LINE_NO << std::endl;
}

ArrayIdentifier::ArrayIdentifier(int line_no, Identifier &name)
    : LeftValue(line_no), name_(name) {}
ArrayIdentifier::~ArrayIdentifier() {
  delete &name_;
  // for (auto &ptr : shape_list_) {
  //   delete ptr;
  // }
}
void ArrayIdentifier::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "ArrayIdentifier" << LINE_NO << std::endl;
  name_.PRINT_NODE;
  ++indentation;
  PRINT_INDENT;
  outfile << "Shape" << LINE_NO << std::endl;
  for (const auto &ele : shape_list_) {
    ele->PRINT_NODE;
  }
}

ConditionExpression::ConditionExpression(int line_no, int op, Expression &lhs,
                                         Expression &rhs)
    : Expression(line_no), op_(op), lhs_(lhs), rhs_(rhs) {}
ConditionExpression::~ConditionExpression() {
  delete &(this->lhs_);
  delete &(this->rhs_);
}
void ConditionExpression::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "ConditionExpression OP: " << OP_STRING(this->op_) << LINE_NO
          << std::endl;
  this->lhs_.PRINT_NODE;
  this->rhs_.PRINT_NODE;
}

BinaryExpression::BinaryExpression(int line_no, int op,
                                   std::shared_ptr<Expression> lhs,
                                   Expression &rhs)
    : Expression(line_no), op_(op), lhs_(std::move(lhs)), rhs_(rhs) {}
BinaryExpression::~BinaryExpression() {
  // delete &(this->lhs_);
  delete &(this->rhs_);
}
void BinaryExpression::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "BinaryExpression OP: " << OP_STRING(this->op_) << LINE_NO
          << std::endl;
  // this->lhs_.PRINT_NODE;
  this->lhs_->PRINT_NODE;
  this->rhs_.PRINT_NODE;
}

UnaryExpression::UnaryExpression(int line_no, int op, Expression &rhs)
    : Expression(line_no), op_(op), rhs_(rhs) {}
UnaryExpression::~UnaryExpression() { delete &(this->rhs_); }
void UnaryExpression::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "UnaryExpression OP: " << OP_STRING(this->op_) << LINE_NO
          << std::endl;
  this->rhs_.PRINT_NODE;
}

FunctionCall::FunctionCall(int line_no, Identifier &name,
                           FunctionActualParameterList &args)
    : Expression(line_no), name_(name), args_(args) {}
FunctionCall::~FunctionCall() {
  delete &(this->name_);
  delete &(this->args_);
}
void FunctionCall::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "FunctionCall" << LINE_NO << std::endl;
  this->name_.PRINT_NODE;
  this->args_.PRINT_NODE;
}

// ==================== Define Class Hierarchy ====================
Define::Define(int line_no) : line_no_(line_no) {}
// Define::~Define() {}

VariableDefine::VariableDefine(int line_no, Identifier &name)
    : Define(line_no), name_(name) {}
VariableDefine::~VariableDefine() { delete &name_; }
void VariableDefine::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Define" << LINE_NO << std::endl;
  name_.PRINT_NODE;
}

VariableDefineWithInit::VariableDefineWithInit(int line_no, Identifier &name,
                                               Expression &value, bool is_const)
    : Define(line_no), name_(name), value_(value), is_const_(is_const) {}
VariableDefineWithInit::~VariableDefineWithInit() {
  delete &name_;
  delete &value_;
}
void VariableDefineWithInit::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "DefineWithInit" << (is_const_ ? " const" : "") << LINE_NO
          << std::endl;
  name_.PRINT_NODE;
  value_.PRINT_NODE;
}

ArrayDefine::ArrayDefine(int line_no, ArrayIdentifier &name)
    : Define(line_no), name_(name) {}
ArrayDefine::~ArrayDefine() { delete &name_; }
void ArrayDefine::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Define" << LINE_NO << std::endl;
  name_.PRINT_NODE;
}

ArrayDefineWithInit::ArrayDefineWithInit(int line_no, ArrayIdentifier &name,
                                         ArrayInitVal &value, bool is_const)
    : Define(line_no), name_(name), value_(value), is_const_(is_const) {}
ArrayDefineWithInit::~ArrayDefineWithInit() {
  delete &name_;
  delete &value_;
}
void ArrayDefineWithInit::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "DefineWithInit" << (is_const_ ? " const" : "") << LINE_NO
          << std::endl;
  name_.PRINT_NODE;
  value_.PRINT_NODE;
}

// ==================== CompUnit Class Hierarchy ====================
CompUnit::CompUnit(int line_no) : line_no_(line_no) {}
// CompUnit::~CompUnit() {}

FunctionDefine::FunctionDefine(int line_no, int return_type, Identifier &name,
                               FunctionFormalParameterList &args, Block &body)
    : CompUnit(line_no),
      return_type_(return_type),
      name_(name),
      args_(args),
      body_(body) {}
FunctionDefine::~FunctionDefine() {
  delete &(this->name_);
  delete &(this->args_);
  delete &(this->body_);
}
void FunctionDefine::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "FunctionDefine" << LINE_NO << std::endl;

  ++indentation;
  PRINT_INDENT;
  outfile << "ReturnType: " << TYPE_STRING(this->return_type_) << LINE_NO
          << std::endl;
  --indentation;

  this->name_.PRINT_NODE;
  this->args_.PRINT_NODE;
  this->body_.PRINT_NODE;
}

Statement::Statement(int line_no) : CompUnit(line_no) {}
// Statement::~Statement() {}

AssignStatement::AssignStatement(int line_no, LeftValue &lhs, Expression &rhs)
    : Statement(line_no), lhs_(lhs), rhs_(rhs) {}
AssignStatement::~AssignStatement() {
  delete &(this->lhs_);
  delete &(this->rhs_);
}
void AssignStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "AssignStatement" << LINE_NO << std::endl;
  this->lhs_.PRINT_NODE;
  this->rhs_.PRINT_NODE;
}

IfElseStatement::IfElseStatement(int line_no, ConditionExpression &cond,
                                 Statement &thenstmt, Statement *elsestmt)
    : Statement(line_no),
      cond_(cond),
      thenstmt_(thenstmt),
      elsestmt_(elsestmt) {}
IfElseStatement::~IfElseStatement() {
  delete &(this->cond_);
  delete &(this->thenstmt_);
  if (nullptr != this->elsestmt_) {
    delete this->elsestmt_;
  }
}
void IfElseStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "IfElseStatement" << LINE_NO << std::endl;

  ++indentation;
  PRINT_INDENT;
  outfile << "Condition" << LINE_NO << std::endl;
  this->cond_.PRINT_NODE;
  PRINT_INDENT;
  outfile << "Then" << LINE_NO << std::endl;
  this->thenstmt_.PRINT_NODE;

  if (nullptr != this->elsestmt_) {
    PRINT_INDENT;
    outfile << "Else" << LINE_NO << std::endl;
    this->elsestmt_->PRINT_NODE;
  }
}

WhileStatement::WhileStatement(int line_no, ConditionExpression &cond,
                               Statement &bodystmt)
    : Statement(line_no), cond_(cond), bodystmt_(bodystmt) {}
WhileStatement::~WhileStatement() {
  delete &(this->cond_);
  delete &(this->bodystmt_);
}
void WhileStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "WhileStatement" << LINE_NO << std::endl;

  ++indentation;
  PRINT_INDENT;
  outfile << "Condition" << LINE_NO << std::endl;
  this->cond_.PRINT_NODE;

  PRINT_INDENT;
  outfile << "Body" << LINE_NO << std::endl;
  this->bodystmt_.PRINT_NODE;
}

BreakStatement::BreakStatement(int line_no) : Statement(line_no) {}
void BreakStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Break" << LINE_NO << std::endl;
}

ContinueStatement::ContinueStatement(int line_no) : Statement(line_no) {}
void ContinueStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Continue" << LINE_NO << std::endl;
}

ReturnStatement::ReturnStatement(int line_no, Expression *value)
    : Statement(line_no), value_(value) {}
ReturnStatement::~ReturnStatement() {
  if (nullptr != this->value_) {
    delete this->value_;
  }
}
void ReturnStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "ReturnStatement" << LINE_NO << std::endl;
  if (nullptr != this->value_) {
    this->value_->PRINT_NODE;
  }
}

VoidStatement::VoidStatement(int line_no) : Statement(line_no) {}
void VoidStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Void" << LINE_NO << std::endl;
}

EvalStatement::EvalStatement(int line_no, Expression &value)
    : Statement(line_no), value_(value) {}
EvalStatement::~EvalStatement() { delete &(this->value_); }
void EvalStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "EvalStatement" << LINE_NO << std::endl;
  this->value_.PRINT_NODE;
}

Block::Block(int line_no) : Statement(line_no) {}
Block::~Block() {
  for (auto &ptr : this->statement_list_) {
    delete ptr;
  }
}
void Block::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "Block" << LINE_NO << std::endl;
  for (const auto &ele : this->statement_list_) {
    ele->PRINT_NODE;
  }
}

DeclareStatement::DeclareStatement(int line_no, int type)
    : Statement(line_no), type_(type) {}
DeclareStatement::~DeclareStatement() {
  for (auto &ptr : define_list_) {
    delete ptr;
  }
}
void DeclareStatement::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "DeclareStatement Type: " << TYPE_STRING(this->type_) << LINE_NO
          << std::endl;
  for (const auto &ele : define_list_) {
    ele->PRINT_NODE;
  }
}

// ==================== Other Top Level Classes ====================
ArrayInitVal::ArrayInitVal(int line_no, bool is_exp, Expression *value)
    : line_no_(line_no), is_exp_(is_exp), value_(value) {}
ArrayInitVal::~ArrayInitVal() {
  if (nullptr != this->value_) {
    delete this->value_;
  }
  for (auto &ptr : this->initval_list_) {
    delete ptr;
  }
}
void ArrayInitVal::PrintNode(int indentation, std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "ArrayInitVal" << LINE_NO << std::endl;
  if (this->is_exp_ && nullptr != this->value_) {
    this->value_->PRINT_NODE;
  }
  for (const auto &ele : this->initval_list_) {
    ele->PRINT_NODE;
  }
}

FunctionFormalParameter::FunctionFormalParameter(int line_no, int type,
                                                 LeftValue &name)
    : line_no_(line_no), type_(type), name_(name) {}
FunctionFormalParameter::~FunctionFormalParameter() { delete &(this->name_); }
void FunctionFormalParameter::PrintNode(int indentation,
                                        std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "FunctionFormalParameter Type: " << TYPE_STRING(this->type_)
          << LINE_NO << std::endl;
  this->name_.PRINT_NODE;
}
FunctionFormalParameterList::FunctionFormalParameterList(int line_no)
    : line_no_(line_no) {}
FunctionFormalParameterList::~FunctionFormalParameterList() {
  for (auto &ptr : this->arg_list_) {
    delete ptr;
  }
}
void FunctionFormalParameterList::PrintNode(int indentation,
                                            std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "FunctionFormalParameterList" << LINE_NO << std::endl;
  if (this->arg_list_.empty()) {
    ++indentation;
    PRINT_INDENT;
    outfile << "Empty" << std::endl;
  } else {
    for (const auto &ele : this->arg_list_) {
      ele->PRINT_NODE;
    }
  }
}

FunctionActualParameterList::FunctionActualParameterList(int line_no)
    : line_no_(line_no) {}
FunctionActualParameterList::~FunctionActualParameterList() {
  for (auto &ptr : this->arg_list_) {
    delete ptr;
  }
}
void FunctionActualParameterList::PrintNode(int indentation,
                                            std::ostream &outfile) {
  PRINT_INDENT;
  outfile << "FunctionActualParameterList" << LINE_NO << std::endl;
  if (this->arg_list_.empty()) {
    ++indentation;
    PRINT_INDENT;
    outfile << "Empty" << std::endl;
  } else {
    for (const auto &ele : this->arg_list_) {
      ele->PRINT_NODE;
    }
  }
}

}  // namespace ast