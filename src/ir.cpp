#include "../include/ir.h"

#include <cstdio>
#include <iostream>

#include "../include/ast.h"
#include "parser.hpp"  // EQOP RELOP

#define TYPE_STRING(type) \
  ((type) == INT ? "int" : ((type) == VOID ? "void" : "undefined"))
#define PRINT_IR(op_name)                                                 \
  printf("(%10s,%10s,%10s,%10s)", (op_name), this->opn1_.name_.c_str(), \
         this->opn2_.name_.c_str(), this->res_.name_.c_str());            \
  if(opn1_.type_ == Opn::Type::Array) printf(" %s", opn1_.offset_->name_.c_str()); \
  if(opn2_.type_ == Opn::Type::Array) printf(" %s", opn2_.offset_->name_.c_str()); \
  if(res_.type_ == Opn::Type::Array) printf(" %s", res_.offset_->name_.c_str()); \
  printf("\n");

SymbolTables gSymbolTables;
FuncTable gFuncTable;
std::vector<IR> gIRList;
ContextInfoInGenIR gContextInfo;

const int kIntWidth = 4;

void PrintSymbolTables() {
  for (auto &symbol_table : gSymbolTables) {
    symbol_table.Print();
  }
}
void PrintFuncTable() {
  std::cout << "FuncTable:" << std::endl;
  printf("%10s%10s\n", "name", "ret_type");
  for (auto &symbol : gFuncTable) {
    printf("%10s", symbol.first.c_str());
    symbol.second.Print();
  }
}
void SymbolTableItem::Print() {
  printf("%10s%10s%10d\n", (this->is_array_ ? "√" : "×"),
         (this->is_const_ ? "√" : "×"), this->offset_);
  if (this->is_array_) {
    printf("%20s: ", "shape");
    for (const auto &shape : this->shape_) {
      printf("[%d]", shape);
    }
    printf("\n");
  }
  if (!this->initval_.empty()) {
    printf("%20s: ", "initval");
    for (const auto &initval : this->initval_) {
      printf("%3d ", initval);
    }
    printf("\n");
  }
}
void FuncTableItem::Print() { printf("%10s\n", TYPE_STRING(this->ret_type_)); }
void SymbolTable::Print() {
  std::cout << "SymbolTable:\n"
            << "  scope_id: " << this->scope_id_ << std::endl
            << "  parent_scope_id: " << this->parent_scope_id_ << std::endl
            << "  size: " << this->size_ << std::endl;
  printf("%10s%10s%10s%10s\n", "name", "is_array", "is_const", "offset");
  for (auto &symbol : this->symbol_table_) {
    printf("%10s", symbol.first.c_str());
    symbol.second.Print();
  }
}

SymbolTableItem *FindSymbol(int scope_id, std::string name) {
  while (-1 != scope_id) {
    auto &current_scope = gSymbolTables[scope_id];
    auto &current_symbol_table = current_scope.symbol_table_;
    auto symbol_iter = current_symbol_table.find(name);
    if (symbol_iter == current_symbol_table.end()) {
      scope_id = current_scope.parent_scope_id_;
    } else {
      return &((*symbol_iter).second);
    }
  }
  return nullptr;
};

std::string NewTemp() {
  static int i = 0;
  return "temp-" + std::to_string(i++);
}

std::string NewLabel() {
  static int i = 0;
  return "label" + std::to_string(i++);
}

IR::OpKind GetOpKind(int op, bool reverse) {
  if (reverse) {
    switch (op) {
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
  } else {
    switch (op) {
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

void IR::PrintIR() {
  switch (op_) {
    case IR::OpKind::ADD:
      PRINT_IR("add");
      break;
    case IR::OpKind::SUB:
      PRINT_IR("sub");
      break;
    case IR::OpKind::MUL:
      PRINT_IR("mul");
      break;
    case IR::OpKind::DIV:
      PRINT_IR("div");
      break;
    case IR::OpKind::MOD:
      PRINT_IR("mod");
      break;
    case IR::OpKind::AND:
      PRINT_IR("and");
      break;
    case IR::OpKind::OR:
      PRINT_IR("or");
      break;
    case IR::OpKind::POS:
      PRINT_IR("pos");
      break;
    case IR::OpKind::NEG:
      PRINT_IR("neg");
      break;
    case IR::OpKind::GT:
      PRINT_IR(">");
      break;
    case IR::OpKind::GE:
      PRINT_IR(">=");
      break;
    case IR::OpKind::LT:
      PRINT_IR("<");
      break;
    case IR::OpKind::LE:
      PRINT_IR("<=");
      break;
    case IR::OpKind::EQ:
      PRINT_IR("==");
      break;
    case IR::OpKind::NE:
      PRINT_IR("!=");
      break;
    case IR::OpKind::NOT:
      PRINT_IR("not");
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
    case IR::OpKind::PARAM:
      PRINT_IR("param");
      break;
    case IR::OpKind::CALL:
      PRINT_IR("call");
      break;
    case IR::OpKind::OFFSET_ASSIGN:
      // printf("(%10s,%10s,%10s,%6s+%3d)\n", "[]=", this->opn1_.name_.c_str(),
      //        this->opn2_.name_.c_str(), this->res_.name_.c_str(),
      //        this->offset_);
      PRINT_IR("[]=");
      break;
    case IR::OpKind::ASSIGN_OFFSET:
      // printf("(%10s,%6s+%3d,%10s,%10s)\n", "=[]", this->opn1_.name_.c_str(),
      //        this->offset_, this->opn2_.name_.c_str(),
      //        this->res_.name_.c_str());
      PRINT_IR("=[]");
      break;
    default:
      printf("unimplemented\n");
      break;
  }
}

void SemanticError(int line_no, const std::string &&error_msg) {
  std::cerr << "语义错误 at line " << line_no << " : " << error_msg
            << std::endl;
  exit(1);
}
void RuntimeError(const std::string &&error_msg) {
  std::cerr << "运行时错误: " << error_msg << std::endl;
  exit(1);
}
