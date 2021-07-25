#include "../include/ir.h"

#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>

#include "../include/ast.h"
#include "parser.hpp"  // EQOP RELOP
#define TYPE_STRING(type) ((type) == INT ? "int" : ((type) == VOID ? "void" : "undefined"))
// #define PRINT_IR(op_name)                                                                          \
//   printf("(%10s,%10s,%10s,%10s)", (op_name), this->opn1_.name_.c_str(), this->opn2_.name_.c_str(), \
//          this->res_.name_.c_str());                                                                \
//   if (opn1_.type_ == Opn::Type::Array) printf(" %s", opn1_.offset_->name_.c_str());                \
//   if (opn2_.type_ == Opn::Type::Array) printf(" %s", opn2_.offset_->name_.c_str());                \
//   if (res_.type_ == Opn::Type::Array) printf(" %s", res_.offset_->name_.c_str());                  \
//   printf("\n");
#define PRINT_IR(op_name)                                                                                    \
  outfile << "(" << std::setw(10) << op_name << std::setw(10) << opn1_.name_ << std::setw(10) << opn2_.name_ \
          << std::setw(10) << res_.name_ << ")";                                                             \
  if (opn1_.type_ == Opn::Type::Array) outfile << " " << opn1_.offset_->name_;                               \
  if (opn2_.type_ == Opn::Type::Array) outfile << " " << opn2_.offset_->name_;                               \
  if (res_.type_ == Opn::Type::Array) outfile << " " << res_.offset_->name_;                                 \
  outfile << std::endl;

#define ASSERT_ENABLE
// assert(res);
#ifdef ASSERT_ENABLE
#define MyAssert(res)                                                    \
  if (!(res)) {                                                          \
    std::cerr << "Assert: " << __FILE__ << " " << __LINE__ << std::endl; \
    exit(255);                                                           \
  }
#else
#define MyAssert(res) ;
#endif

namespace ir {

Scopes gScopes;
FuncTable gFuncTable;
std::vector<IR> gIRList;

const int kIntWidth = 4;

void PrintScopes(std::ostream &outfile) {
  for (auto &symbol_table : gScopes) {
    symbol_table.Print(outfile);
  }
}

void PrintFuncTable(std::ostream &outfile) {
  outfile << "FuncTable:" << std::endl;
  outfile << std::setw(10) << "name" << std::setw(10) << "ret_type" << std::setw(10) << "size" << std::setw(10)
          << "scope_id" << std::endl;
  // printf("%10s%10s%10s%10s\n", "name", "ret_type", "size", "scope_id");
  for (auto &symbol : gFuncTable) {
    // printf("%10s", symbol.first.c_str());
    outfile << std::setw(10) << symbol.first;
    symbol.second.Print(outfile);
  }
}

void SymbolTableItem::Print(std::ostream &outfile) {
  outfile << std::setw(10) << (this->is_array_ ? "√" : "×") << std::setw(10) << (this->is_const_ ? "√" : "×")
          << std::setw(10) << this->offset_ << std::setw(10) << (this->is_arg_ ? "√" : "×") << std::setw(10)
          << this->stack_offset_ << std::endl;
  // printf("%10s%10s%10d\n", (this->is_array_ ? "√" : "×"), (this->is_const_ ? "√" : "×"), this->offset_);
  if (this->is_array_) {
    // printf("%20s: ", "shape");
    outfile << std::setw(20) << "shape"
            << ": ";
    for (const auto &shape : this->shape_) {
      // printf("[%d]", shape);
      outfile << "[" << shape << "]";
    }
    // printf("\n");
    outfile << std::endl;
  }
  if (!this->initval_.empty()) {
    // printf("%20s: ", "initval");
    // outfile << std::setw(20) << "initval"
    //         << ": ";
    // for (const auto &initval : this->initval_) {
    //   // printf("%3d ", initval);
    //   outfile << std::setw(3) << initval;
    // }
    // printf("\n");
    outfile << std::endl;
  }
}

void FuncTableItem::Print(std::ostream &outfile) {
  outfile << std::setw(10) << TYPE_STRING(this->ret_type_) << std::setw(10) << this->size_ << std::setw(10)
          << this->scope_id_ << std::endl;
  // printf("%10s%10d%10d\n", TYPE_STRING(this->ret_type_), this->size_, this->scope_id_);
}

void Scope::Print(std::ostream &outfile) {
  outfile << "Scope:" << std::endl
          << "  scope_id: " << this->scope_id_ << std::endl
          << "  parent_scope_id: " << this->parent_scope_id_ << std::endl
          << "  dyn_offset: " << this->dynamic_offset_ << std::endl;
  // printf("%10s%10s%10s%10s\n", "name", "is_array", "is_const", "offset");
  outfile << std::setw(10) << "name" << std::setw(10) << "is_array" << std::setw(10) << "is_const" << std::setw(10)
          << "offset" << std::setw(10) << "is_arg" << std::setw(10) << "stack_offset" << std::endl;
  for (auto &symbol : this->symbol_table_) {
    // printf("%10s", symbol.first.c_str());
    outfile << std::setw(10) << symbol.first;
    symbol.second.Print(outfile);
  }
}

bool Scope::IsSubScope(int scope_id) {
  MyAssert(scope_id >= 0);
  // <0无意义
  int sid = this->scope_id_;
  while (-1 != sid) {
    if (sid == scope_id) {
      return true;
    } else {
      sid = gScopes[sid].parent_scope_id_;
    }
  }
  return false;
}

int FindSymbol(int scope_id, std::string name, SymbolTableItem *&res_symbol_item) {
  while (-1 != scope_id) {
    auto &current_scope = gScopes[scope_id];
    auto &current_symbol_table = current_scope.symbol_table_;
    auto symbol_iter = current_symbol_table.find(name);
    if (symbol_iter == current_symbol_table.end()) {
      scope_id = current_scope.parent_scope_id_;
    } else {
      res_symbol_item = &((*symbol_iter).second);
      return scope_id;
    }
  }
  res_symbol_item = nullptr;
  return -1;
};

std::string NewTemp() {
  static int i = 0;
  return "temp-" + std::to_string(i++);
}

std::string NewLabel() {
  static int i = 0;
  return ".label" + std::to_string(i++);
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
      default:
        return IR::OpKind::VOID;
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
      default:
        return IR::OpKind::VOID;
    }
  }
  return IR::OpKind::VOID;
}

void IR::PrintIR(std::ostream &outfile) {
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
    case IR::OpKind::NEG:
      PRINT_IR("neg");
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
    // case IR::OpKind::OFFSET_ASSIGN:
    //   PRINT_IR("[]=");
    //   break;
    case IR::OpKind::ASSIGN_OFFSET:
      PRINT_IR("=[]");
      break;
    default:
      // printf("unimplemented\n");
      outfile << "unimplemented" << std::endl;
      break;
  }
}

void SemanticError(int line_no, const std::string &&error_msg) {
  std::cerr << "语义错误 at line " << line_no << " : " << error_msg << std::endl;
  exit(255);
}

void RuntimeError(const std::string &&error_msg) {
  std::cerr << "运行时错误: " << error_msg << std::endl;
  exit(255);
}

}  // namespace ir

#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif
