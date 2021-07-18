#ifndef __AST__H__
#define __AST__H__

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../include/ir.h"

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
  int line_no_;
  std::vector<CompUnit*> compunit_list_;
  Root(int line_no);
  virtual ~Root();
  virtual void GenerateIR(ir::ContextInfo& ctx);
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
  int line_no_;
  Expression(int line_no);
  virtual ~Expression() = default;
  virtual void GenerateIR(ir::ContextInfo& ctx) = 0;
  virtual void Evaluate(ir::ContextInfo& ctx) = 0;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfilefile = std::clog) = 0;
};

class Number : public Expression {
 public:
  int value_;
  Number(int line_no, int value);
  virtual ~Number();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// LeftValue -> Identifier ArrayIdentifier
class LeftValue : public Expression {
 public:
  LeftValue(int line_no);
  virtual ~LeftValue() = default;
  virtual void GenerateIR(ir::ContextInfo& ctx) = 0;
  virtual void Evaluate(ir::ContextInfo& ctx) = 0;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfilefile = std::clog) = 0;
};

class Identifier : public LeftValue {
 public:
  std::string& name_;
  Identifier(int line_no, std::string& name);
  virtual ~Identifier();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ArrayIdentifier : public LeftValue {
 public:
  Identifier& name_;
  std::vector<std::shared_ptr<Expression>> shape_list_;
  ArrayIdentifier(int line_no, Identifier& name);
  virtual ~ArrayIdentifier();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// NOTE: 存在unaryexp或bixep必须作为condexp的场景
class ConditionExpression : public Expression {
 public:
  int op_;
  Expression& lhs_;
  Expression& rhs_;
  ConditionExpression(int line_no, int op, Expression& lhs, Expression& rhs);
  virtual ~ConditionExpression();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// NOTE: 没有unary必须作为biexp的场景
class BinaryExpression : public Expression {
 public:
  int op_;
  std::shared_ptr<Expression> lhs_;
  Expression& rhs_;
  BinaryExpression(int line_no, int op, std::shared_ptr<Expression> lhs,
                   Expression& rhs);
  virtual ~BinaryExpression();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class UnaryExpression : public Expression {
 public:
  int op_;
  Expression& rhs_;
  UnaryExpression(int line_no, int op, Expression& rhs);
  virtual ~UnaryExpression();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionActualParameterList;

class FunctionCall : public Expression {
 public:
  Identifier& name_;
  FunctionActualParameterList& args_;
  FunctionCall(int line_no, Identifier& name,
               FunctionActualParameterList& args);
  virtual ~FunctionCall();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

// ==================== Define Class Hierarchy ====================
// Define -> VariableDefine VariableDefineWithInit
//           ArrayDefine ArrayDefineWithInit
// Eg: a=5,b,c[5]={1,2,3,4,5},d[2][3]
class Define {
 public:
  int line_no_;
  Define(int line_no);
  virtual ~Define() = default;
  virtual void GenerateIR(ir::ContextInfo& ctx) = 0;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog) = 0;
};

class VariableDefine : public Define {
 public:
  Identifier& name_;
  VariableDefine(int line_no, Identifier& name);
  virtual ~VariableDefine();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class VariableDefineWithInit : public Define {
 public:
  bool is_const_;
  Identifier& name_;
  Expression& value_;
  VariableDefineWithInit(int line_no, Identifier& name, Expression& value,
                         bool is_const);
  virtual ~VariableDefineWithInit();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ArrayDefine : public Define {
 public:
  ArrayIdentifier& name_;
  ArrayDefine(int line_no, ArrayIdentifier& name);
  virtual ~ArrayDefine();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ArrayInitVal;

class ArrayDefineWithInit : public Define {
 public:
  bool is_const_;
  ArrayIdentifier& name_;
  ArrayInitVal& value_;
  ArrayDefineWithInit(int line_no, ArrayIdentifier& name, ArrayInitVal& value,
                      bool is_const);
  virtual ~ArrayDefineWithInit();
  virtual void GenerateIR(ir::ContextInfo& ctx);
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
  int line_no_;
  CompUnit(int line_no);
  virtual ~CompUnit() = default;
  virtual void GenerateIR(ir::ContextInfo& ctx) = 0;
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
  FunctionDefine(int line_no, int return_type, Identifier& name,
                 FunctionFormalParameterList& args, Block& body);
  virtual ~FunctionDefine();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class Statement : public CompUnit {
 public:
  Statement(int line_no);
  virtual ~Statement() = default;
  virtual void GenerateIR(ir::ContextInfo& ctx) = 0;
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog) = 0;
};

class AssignStatement : public Statement {
 public:
  LeftValue& lhs_;
  Expression& rhs_;
  AssignStatement(int line_no, LeftValue& lhs, Expression& rhs);
  virtual ~AssignStatement();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class IfElseStatement : public Statement {
 public:
  ConditionExpression& cond_;
  Statement& thenstmt_;
  Statement* elsestmt_;
  IfElseStatement(int line_no, ConditionExpression& cond, Statement& thenstmt,
                  Statement* elsestmt = nullptr);
  virtual ~IfElseStatement();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class WhileStatement : public Statement {
 public:
  ConditionExpression& cond_;
  Statement& bodystmt_;
  WhileStatement(int line_no, ConditionExpression& cond, Statement& bodystmt);
  virtual ~WhileStatement();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class BreakStatement : public Statement {
 public:
  BreakStatement(int line_no);
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ContinueStatement : public Statement {
 public:
  ContinueStatement(int line_no);
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class ReturnStatement : public Statement {
 public:
  Expression* value_;
  ReturnStatement(int line_no, Expression* value = nullptr);
  virtual ~ReturnStatement();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class VoidStatement : public Statement {
 public:
  VoidStatement(int line_no);
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class EvalStatement : public Statement {
 public:
  Expression& value_;
  EvalStatement(int line_no, Expression& value);
  virtual ~EvalStatement();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class Block : public Statement {
 public:
  std::vector<Statement*> statement_list_;
  Block(int line_no);
  virtual ~Block();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class DeclareStatement : public Statement {
 public:
  int type_;
  std::vector<Define*> define_list_;
  DeclareStatement(int line_no, int type);
  virtual ~DeclareStatement();
  virtual void GenerateIR(ir::ContextInfo& ctx);
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
  int line_no_;
  bool is_exp_;
  Expression* value_;  // maybe nullptr
  std::vector<ArrayInitVal*> initval_list_;
  ArrayInitVal(int line_no, bool is_exp, Expression* value = nullptr);
  virtual ~ArrayInitVal();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void Evaluate(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionFormalParameter {
 public:
  int line_no_;
  int type_;
  LeftValue& name_;
  FunctionFormalParameter(int line_no, int type, LeftValue& name);
  virtual ~FunctionFormalParameter();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionFormalParameterList {
 public:
  int line_no_;
  std::vector<FunctionFormalParameter*> arg_list_;
  FunctionFormalParameterList(int line_no);
  virtual ~FunctionFormalParameterList();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

class FunctionActualParameterList {
 public:
  int line_no_;
  std::vector<Expression*> arg_list_;
  FunctionActualParameterList(int line_no);
  virtual ~FunctionActualParameterList();
  virtual void GenerateIR(ir::ContextInfo& ctx);
  virtual void PrintNode(int indentation = 0,
                         std::ostream& outfile = std::clog);
};

}  // namespace ast

#endif