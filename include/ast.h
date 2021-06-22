#ifndef __AST__H__
#define __AST__H__

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ast: abstract syntax tree
// 包含了抽象语法树全部结点类
// 顶层结点类: Root Expression CompUnit Define ArrayInitVal
//            FunctionFormalParameter FunctionFormalParameterList
//            FunctionActualParameterList
// 此外还有一个服务于输出抽象语法树功能的函数: PrintIndentation
namespace ast {

void PrintIndentation(int identation = 0, std::ostream& outfile = std::clog);

class CompUnit;

// ==================== Root Class ====================
// Root object is the root node of abstract syntax tree.
// Regard 'Root' as a list of 'CompUnit's.
class Root {
 public:
  std::vector<CompUnit*> compunit_list_;
  virtual ~Root();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// ==================== Expression Class Hierarchy ====================
// Expression -> Number LeftValue ConditionExpression
//               BinaryExpression UnaryExpression FunctionCall
//
// LeftValue -> Identifier ArrayIdentifier
class Expression {
 public:
  virtual ~Expression() = default;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfilefile = std::clog) = 0;
};

class Number : public Expression {
 public:
  int value_;
  Number(int value);
  virtual ~Number();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// LeftValue -> Identifier ArrayIdentifier
class LeftValue : public Expression {
 public:
  virtual ~LeftValue() = default;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfilefile = std::clog) = 0;
};

class Identifier : public LeftValue {
 public:
  std::string& name_;
  Identifier(std::string& name);
  virtual ~Identifier();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ArrayIdentifier : public LeftValue {
 public:
  Identifier& name_;
  std::vector<Expression*> shape_list_;
  ArrayIdentifier(Identifier& name);
  virtual ~ArrayIdentifier();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ConditionExpression : public Expression {
 public:
  int op_;
  Expression& lhs_;
  Expression& rhs_;
  ConditionExpression(int op, Expression& lhs, Expression& rhs);
  virtual ~ConditionExpression();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class BinaryExpression : public Expression {
 public:
  int op_;
  Expression& lhs_;
  Expression& rhs_;
  BinaryExpression(int op, Expression& lhs, Expression& rhs);
  virtual ~BinaryExpression();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class UnaryExpression : public Expression {
 public:
  int op_;
  Expression& rhs_;
  UnaryExpression(int op, Expression& rhs);
  virtual ~UnaryExpression();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionActualParameterList;

class FunctionCall : public Expression {
 public:
  Identifier& name_;
  FunctionActualParameterList& args_;
  FunctionCall(Identifier& name, FunctionActualParameterList& args);
  virtual ~FunctionCall();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// ==================== Define Class Hierarchy ====================
// Define -> VariableDefine VariableDefineWithInit
//           ArrayDefine ArrayDefineWithInit
// Eg: a=5,b,c[5]={1,2,3,4,5},d[2][3]
class Define {
 public:
  virtual ~Define() = default;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog) = 0;
};

class VariableDefine : public Define {
 public:
  Identifier& name_;
  VariableDefine(Identifier& name);
  virtual ~VariableDefine();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class VariableDefineWithInit : public Define {
 public:
  bool is_const_;
  Identifier& name_;
  Expression& value_;
  VariableDefineWithInit(Identifier& name, Expression& value, bool is_const);
  virtual ~VariableDefineWithInit();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ArrayDefine : public Define {
 public:
  ArrayIdentifier& name_;
  ArrayDefine(ArrayIdentifier& name);
  virtual ~ArrayDefine();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ArrayInitVal;

class ArrayDefineWithInit : public Define {
 public:
  bool is_const_;
  ArrayIdentifier& name_;
  ArrayInitVal& value_;
  ArrayDefineWithInit(ArrayIdentifier& name, ArrayInitVal& value,
                      bool is_const);
  virtual ~ArrayDefineWithInit();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// ==================== CompUnit Class Hierarchy ====================
// CompUnit -> FunctionDefine Statement
//
// Statement -> AssignStatement IfElseStatement WhileStatement
//              BreakStatement ContinueStatement ReturnStatement
//              VoidStatement EvalStatement Block DeclareStatement
//
// NOTE: regard 'Declare' as a kind of 'Statement':'DeclareStatement'
//       regard 'Block' as a list of 'Statement's
class CompUnit {
 public:
  virtual ~CompUnit() = default;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog) = 0;
};

class FunctionFormalParameterList;
class Block;

class FunctionDefine : public CompUnit {
 public:
  int return_type_;
  Identifier& name_;
  FunctionFormalParameterList& args_;
  Block& body_;
  FunctionDefine(int return_type, Identifier& name,
                 FunctionFormalParameterList& args, Block& body);
  virtual ~FunctionDefine();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class Statement : public CompUnit {
 public:
  virtual ~Statement() = default;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog) = 0;
};

class AssignStatement : public Statement {
 public:
  LeftValue& lhs_;
  Expression& rhs_;
  AssignStatement(LeftValue& lhs, Expression& rhs);
  virtual ~AssignStatement();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class IfElseStatement : public Statement {
 public:
  ConditionExpression& cond_;
  Statement& thenstmt_;
  Statement* elsestmt_;
  IfElseStatement(ConditionExpression& cond, Statement& thenstmt,
                  Statement* elsestmt = nullptr);
  virtual ~IfElseStatement();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class WhileStatement : public Statement {
 public:
  ConditionExpression& cond_;
  Statement& bodystmt_;
  WhileStatement(ConditionExpression& cond, Statement& bodystmt);
  virtual ~WhileStatement();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class BreakStatement : public Statement {
 public:
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ContinueStatement : public Statement {
 public:
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ReturnStatement : public Statement {
 public:
  Expression* value_;
  ReturnStatement(Expression* value = nullptr);
  virtual ~ReturnStatement();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class VoidStatement : public Statement {
 public:
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class EvalStatement : public Statement {
 public:
  Expression& value_;
  EvalStatement(Expression& value);
  virtual ~EvalStatement();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class Block : public Statement {
 public:
  std::vector<Statement*> statement_list_;
  virtual ~Block();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class DeclareStatement : public Statement {
 public:
  int type_;
  std::vector<Define*> define_list_;
  DeclareStatement(int type);
  virtual ~DeclareStatement();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// ==================== Other Top Level Classes ====================
// ArrayInitVal
// FunctionFormalParameter
// FunctionFormalParameterList
// FunctionActualParameterList
class ArrayInitVal {
 public:
  bool is_exp_;
  Expression* value_;  // maybe nullptr
  std::vector<ArrayInitVal*> initval_list_;
  ArrayInitVal(bool is_exp, Expression* value = nullptr);
  virtual ~ArrayInitVal();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionFormalParameter {
 public:
  int type_;
  LeftValue& name_;
  FunctionFormalParameter(int type, LeftValue& name);
  virtual ~FunctionFormalParameter();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionFormalParameterList {
 public:
  std::vector<FunctionFormalParameter*> arg_list_;
  virtual ~FunctionFormalParameterList();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionActualParameterList {
 public:
  std::vector<Expression*> arg_list_;
  virtual ~FunctionActualParameterList();
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

}  // namespace ast

#endif