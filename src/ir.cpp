#include "../include/ir.h"

#include <iostream>

#include "../include/ast.h"
#include "parser.hpp" // EQOP RELOP

#define PRINT_IR(op_name)                                                 \
  printf("(%10s,%10s,%10s,%10s)\n", (op_name), this->opn1_.name_.c_str(), \
         this->opn2_.name_.c_str(), this->res_.name_.c_str());

SymbolTables gSymbolTables;
FuncTable gFuncTable;
std::vector<IR> gIRList;
ContextInfoInGenIR gContextInfo;

const int kIntWidth = 4;

SymbolTableItem *FindSymbol(int scope_id, std::string name)
{
  while (-1 != scope_id)
  {
    auto &current_scope = gSymbolTables[scope_id];
    auto &current_symbol_table = current_scope.symbol_table_;
    auto symbol_iter = current_symbol_table.find(name);
    if (symbol_iter == current_symbol_table.end())
    {
      scope_id = current_scope.parent_scope_id_;
    }
    else
    {
      return &((*symbol_iter).second);
    }
  }
  return nullptr;
};

std::string NewTemp()
{
  static int i = 0;
  return "temp-" + std::to_string(i++);
}

std::string NewLabel()
{
  static int i = 0;
  return "label" + std::to_string(i++);
}

IR::OpKind GetOpKind(int op, bool reverse)
{
  if (reverse)
  {
    switch (op)
    {
    case EQ:
      return IR::OpKind::JNE;
    case NE:
      return IR::OpKind::JEQ;
    case LT:
      return IR::OpKind::JGE;
    case LE:
      return IR::OpKind::JGT;
    case GT:
      return IR::OpKind::JLE;
    case GE:
      return IR::OpKind::JLT;
    }
  }
  else
  {
    switch (op)
    {
    case EQ:
      return IR::OpKind::JEQ;
    case NE:
      return IR::OpKind::JNE;
    case LT:
      return IR::OpKind::JLT;
    case LE:
      return IR::OpKind::JLE;
    case GT:
      return IR::OpKind::JGT;
    case GE:
      return IR::OpKind::JGE;
    }
  }
  return IR::OpKind::VOID;
}

void IR::PrintIR()
{
  switch (op_)
  {
  case IR::OpKind::ADD:
    PRINT_IR("add");
    break;
  case IR::OpKind::SUB:
    PRINT_IR("sub");
    break;
  case IR::OpKind::RET:
    PRINT_IR("return");
    break;
  case IR::OpKind::ASSIGN:
    PRINT_IR("assign");
    break;
  case IR::OpKind::LABEL:
    PRINT_IR("label");
    break;
  case IR::OpKind::GOTO:
    PRINT_IR("goto");
    break;
  case IR::OpKind::JEQ:
    PRINT_IR("jeq");
    break;
  case IR::OpKind::JNE:
    PRINT_IR("jne");
    break;
  case IR::OpKind::JLT:
    PRINT_IR("jlt");
    break;
  case IR::OpKind::JLE:
    PRINT_IR("jle");
    break;
  case IR::OpKind::JGT:
    PRINT_IR("jgt");
    break;
  case IR::OpKind::JGE:
    PRINT_IR("jge");
    break;
  }
}

void SemanticError(int line_no, const std::string &&error_msg)
{
  std::cerr << "语义错误 at line " << line_no << " : " << error_msg
            << std::endl;
}
void RuntimeError(const std::string &&error_msg)
{
  std::cerr << "运行时错误: " << error_msg << std::endl;
  exit(1);
}