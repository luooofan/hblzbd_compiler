#include "../include/ast.h"

#include "../include/parser.hpp"

#define PRINT_INDENT PrintIndentation(indentation, outfile);
#define PRINT_NODE PrintNode(indentation + 1, outfile);
#define TYPE_STRING(type) \
  ((type) == INT ? "int" : ((type) == VOID ? "void" : "undefined"))
#define OP_STRING(op) (kOpStr[(op)-258])

static const std::string kOpStr[] = {"+", "-", "!", "*", "/", "%", "<",
                                     "<=", ">", ">=", "==", "!=", "&&", "||"};

namespace ast
{

  void PrintIndentation(int indentation, std::ostream &outfile)
  {
    for (int i = 0; i < indentation; ++i)
    {
      outfile << "|   ";
    }
    outfile << "|-> ";
  }

  // ==================== Root Class ====================
  Root::~Root()
  {
    for (auto &ptr : compunit_list_)
    {
      delete ptr;
    }
  }
  void Root::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Root" << std::endl;
    for (const auto &ele : compunit_list_)
    {
      ele->PRINT_NODE;
    }
  }

  // ==================== Expression Class Hierarchy ====================
  // Expression::~Expression() {}

  Number::Number(int value) : value_(value) {}
  Number::~Number() {}
  void Number::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Number: " << value_ << std::endl;
  }

  // LeftValue::~LeftValue() {}

  Identifier::Identifier(std::string &name) : name_(name) {}
  Identifier::~Identifier() { delete &name_; }
  void Identifier::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Identifier: " << name_ << std::endl;
  }

  ArrayIdentifier::ArrayIdentifier(Identifier &name) : name_(name) {}
  ArrayIdentifier::~ArrayIdentifier()
  {
    delete &name_;
    for (auto &ptr : shape_list_)
    {
      delete ptr;
    }
  }
  void ArrayIdentifier::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "ArrayIdentifier" << std::endl;
    name_.PRINT_NODE;
    ++indentation;
    PRINT_INDENT;
    outfile << "Shape" << std::endl;
    for (const auto &ele : shape_list_)
    {
      ele->PRINT_NODE;
    }
  }

  ConditionExpression::ConditionExpression(int op, Expression &lhs,
                                           Expression &rhs)
      : op_(op), lhs_(lhs), rhs_(rhs) {}
  ConditionExpression::~ConditionExpression()
  {
    delete &(this->lhs_);
    delete &(this->rhs_);
  }
  void ConditionExpression::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "ConditionExpression OP: " << OP_STRING(this->op_) << std::endl;
    this->lhs_.PRINT_NODE;
    this->rhs_.PRINT_NODE;
  }

  BinaryExpression::BinaryExpression(int op, Expression &lhs, Expression &rhs)
      : op_(op), lhs_(lhs), rhs_(rhs) {}
  BinaryExpression::~BinaryExpression()
  {
    delete &(this->lhs_);
    delete &(this->rhs_);
  }
  void BinaryExpression::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "BinaryExpression OP: " << OP_STRING(this->op_) << std::endl;
    this->lhs_.PRINT_NODE;
    this->rhs_.PRINT_NODE;
  }

  UnaryExpression::UnaryExpression(int op, Expression &rhs)
      : op_(op), rhs_(rhs) {}
  UnaryExpression::~UnaryExpression() { delete &(this->rhs_); }
  void UnaryExpression::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "UnaryExpression OP: " << OP_STRING(this->op_) << std::endl;
    this->rhs_.PRINT_NODE;
  }

  FunctionCall::FunctionCall(Identifier &name, FunctionActualParameterList &args)
      : name_(name), args_(args) {}
  FunctionCall::~FunctionCall()
  {
    delete &(this->name_);
    delete &(this->args_);
  }
  void FunctionCall::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "FunctionCall" << std::endl;
    this->name_.PRINT_NODE;
    this->args_.PRINT_NODE;
  }

  // ==================== Define Class Hierarchy ====================
  // Define::~Define() {}

  VariableDefine::VariableDefine(Identifier &name) : name_(name) {}
  VariableDefine::~VariableDefine() { delete &name_; }
  void VariableDefine::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Define" << std::endl;
    name_.PRINT_NODE;
  }

  VariableDefineWithInit::VariableDefineWithInit(Identifier &name,
                                                 Expression &value, bool is_const)
      : name_(name), value_(value), is_const_(is_const) {}
  VariableDefineWithInit::~VariableDefineWithInit()
  {
    delete &name_;
    delete &value_;
  }
  void VariableDefineWithInit::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "DefineWithInit" << (is_const_ ? " const" : "") << std::endl;
    name_.PRINT_NODE;
    value_.PRINT_NODE;
  }

  ArrayDefine::ArrayDefine(ArrayIdentifier &name) : name_(name) {}
  ArrayDefine::~ArrayDefine() { delete &name_; }
  void ArrayDefine::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Define" << std::endl;
    name_.PRINT_NODE;
  }

  ArrayDefineWithInit::ArrayDefineWithInit(ArrayIdentifier &name,
                                           ArrayInitVal &value, bool is_const)
      : name_(name), value_(value), is_const_(is_const) {}
  ArrayDefineWithInit::~ArrayDefineWithInit()
  {
    delete &name_;
    delete &value_;
  }
  void ArrayDefineWithInit::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "DefineWithInit" << (is_const_ ? " const" : "") << std::endl;
    name_.PRINT_NODE;
    value_.PRINT_NODE;
  }

  // ==================== CompUnit Class Hierarchy ====================
  // CompUnit::~CompUnit() {}

  FunctionDefine::FunctionDefine(int return_type, Identifier &name,
                                 FunctionFormalParameterList &args, Block &body)
      : return_type_(return_type), name_(name), args_(args), body_(body) {}
  FunctionDefine::~FunctionDefine()
  {
    delete &(this->name_);
    delete &(this->args_);
    delete &(this->body_);
  }
  void FunctionDefine::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "FunctionDefine" << std::endl;

    ++indentation;
    PRINT_INDENT;
    outfile << "ReturnType: " << TYPE_STRING(this->return_type_) << std::endl;
    --indentation;

    this->name_.PRINT_NODE;
    this->args_.PRINT_NODE;
    this->body_.PRINT_NODE;
  }

  // Statement::~Statement() {}

  AssignStatement::AssignStatement(LeftValue &lhs, Expression &rhs)
      : lhs_(lhs), rhs_(rhs) {}
  AssignStatement::~AssignStatement()
  {
    delete &(this->lhs_);
    delete &(this->rhs_);
  }
  void AssignStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "AssignStatement" << std::endl;
    this->lhs_.PRINT_NODE;
    this->rhs_.PRINT_NODE;
  }

  IfElseStatement::IfElseStatement(ConditionExpression &cond, Statement &thenstmt,
                                   Statement *elsestmt)
      : cond_(cond), thenstmt_(thenstmt), elsestmt_(elsestmt) {}
  IfElseStatement::~IfElseStatement()
  {
    delete &(this->cond_);
    delete &(this->thenstmt_);
    if (nullptr != this->elsestmt_)
    {
      delete this->elsestmt_;
    }
  }
  void IfElseStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "IfElseStatement" << std::endl;

    ++indentation;
    PRINT_INDENT;
    outfile << "Condition" << std::endl;
    this->cond_.PRINT_NODE;
    PRINT_INDENT;
    outfile << "Then" << std::endl;
    this->thenstmt_.PRINT_NODE;

    if (nullptr != this->elsestmt_)
    {
      PRINT_INDENT;
      outfile << "Else" << std::endl;
      this->elsestmt_->PRINT_NODE;
    }
  }

  WhileStatement::WhileStatement(ConditionExpression &cond, Statement &bodystmt)
      : cond_(cond), bodystmt_(bodystmt) {}
  WhileStatement::~WhileStatement()
  {
    delete &(this->cond_);
    delete &(this->bodystmt_);
  }
  void WhileStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "WhileStatement" << std::endl;

    ++indentation;
    PRINT_INDENT;
    outfile << "Condition" << std::endl;
    this->cond_.PRINT_NODE;

    PRINT_INDENT;
    outfile << "Body" << std::endl;
    this->bodystmt_.PRINT_NODE;
  }

  void BreakStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Break" << std::endl;
  }

  void ContinueStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Continue" << std::endl;
  }

  ReturnStatement::ReturnStatement(Expression *value) : value_(value) {}
  ReturnStatement::~ReturnStatement()
  {
    if (nullptr != this->value_)
    {
      delete this->value_;
    }
  }
  void ReturnStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "ReturnStatement" << std::endl;
    if (nullptr != this->value_)
    {
      this->value_->PRINT_NODE;
    }
  }

  void VoidStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Void" << std::endl;
  }

  EvalStatement::EvalStatement(Expression &value) : value_(value) {}
  EvalStatement::~EvalStatement() { delete &(this->value_); }
  void EvalStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "EvalStatement" << std::endl;
    this->value_.PRINT_NODE;
  }

  Block::~Block()
  {
    for (auto &ptr : this->statement_list_)
    {
      delete ptr;
    }
  }
  void Block::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "Block" << std::endl;
    for (const auto &ele : this->statement_list_)
    {
      ele->PRINT_NODE;
    }
  }

  DeclareStatement::DeclareStatement(int type) : type_(type) {}
  DeclareStatement::~DeclareStatement()
  {
    for (auto &ptr : define_list_)
    {
      delete ptr;
    }
  }
  void DeclareStatement::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "DeclareStatement Type: " << TYPE_STRING(this->type_) << std::endl;
    for (const auto &ele : define_list_)
    {
      ele->PRINT_NODE;
    }
  }

  // ==================== Other Top Level Classes ====================
  ArrayInitVal::ArrayInitVal(bool is_exp, Expression *value)
      : is_exp_(is_exp), value_(value) {}
  ArrayInitVal::~ArrayInitVal()
  {
    if (nullptr != this->value_)
    {
      delete this->value_;
    }
    for (auto &ptr : this->initval_list_)
    {
      delete ptr;
    }
  }
  void ArrayInitVal::PrintNode(int indentation, std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "ArrayInitVal" << std::endl;
    if (this->is_exp_ && nullptr != this->value_)
    {
      this->value_->PRINT_NODE;
    }
    for (const auto &ele : this->initval_list_)
    {
      ele->PRINT_NODE;
    }
  }

  FunctionFormalParameter::FunctionFormalParameter(int type, LeftValue &name)
      : type_(type), name_(name) {}
  FunctionFormalParameter::~FunctionFormalParameter() { delete &(this->name_); }
  void FunctionFormalParameter::PrintNode(int indentation,
                                          std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "FunctionFormalParameter Type: " << TYPE_STRING(this->type_)
            << std::endl;
    this->name_.PRINT_NODE;
  }

  FunctionFormalParameterList::~FunctionFormalParameterList()
  {
    for (auto &ptr : this->arg_list_)
    {
      delete ptr;
    }
  }
  void FunctionFormalParameterList::PrintNode(int indentation,
                                              std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "FunctionFormalParameterList" << std::endl;
    if (this->arg_list_.empty())
    {
      ++indentation;
      PRINT_INDENT;
      outfile << "Empty" << std::endl;
    }
    else
    {
      for (const auto &ele : this->arg_list_)
      {
        ele->PRINT_NODE;
      }
    }
  }

  FunctionActualParameterList::~FunctionActualParameterList()
  {
    for (auto &ptr : this->arg_list_)
    {
      delete ptr;
    }
  }
  void FunctionActualParameterList::PrintNode(int indentation,
                                              std::ostream &outfile)
  {
    PRINT_INDENT;
    outfile << "FunctionActualParameterList" << std::endl;
    if (this->arg_list_.empty())
    {
      ++indentation;
      PRINT_INDENT;
      outfile << "Empty" << std::endl;
    }
    else
    {
      for (const auto &ele : this->arg_list_)
      {
        ele->PRINT_NODE;
      }
    }
  }

} // namespace ast