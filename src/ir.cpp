#include "../include/ir.h"

#include <iomanip>
#include <iostream>

#include "../include/ast.h"
#include "parser.hpp"  // EQOP RELOP
#define TYPE_STRING(type) ((type) == INT ? "int" : ((type) == VOID ? "void" : "undefined"))
#define PRINT_IR(op_name)                                                                            \
  outfile << "(" << std::setw(15) << op_name << std::setw(15) << std::string(opn1_) << std::setw(15) \
          << std::string(opn2_) << std::setw(15) << std::string(res_) << ")" << std::endl;

#define ASSERT_ENABLE
#include "../include/myassert.h"

namespace ir {

Scopes gScopes;
FuncTable gFuncTable;
// std::vector<IR> gIRList;

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
  for (auto &symbol : gFuncTable) {
    outfile << std::setw(10) << symbol.first;
    symbol.second.Print(outfile);
  }
}

void SymbolTableItem::Print(std::ostream &outfile) {
  outfile << std::setw(10) << (this->is_array_ ? "√" : "×") << std::setw(10) << (this->is_const_ ? "√" : "×");
  if (this->is_array_) {
    outfile << std::setw(10) << "shape: ";
    for (const auto &shape : this->shape_) {
      outfile << "[" << shape << "]";
    }
  }
  if (!this->initval_.empty()) {
  }
  outfile << std::endl;
}

void FuncTableItem::Print(std::ostream &outfile) {
  outfile << std::setw(10) << TYPE_STRING(this->ret_type_) << std::setw(10) << this->scope_id_ << std::endl;
}

void Scope::Print(std::ostream &outfile) {
  outfile << "Scope:" << std::endl
          << "  scope_id: " << this->scope_id_ << "  parent_scope_id: " << this->parent_scope_id_ << std::endl;
  if (!this->symbol_table_.empty()) {
    outfile << std::setw(10) << "name" << std::setw(10) << "is_array" << std::setw(10) << "is_const" << std::endl;
  }
  for (auto &symbol : this->symbol_table_) {
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

static const std::string kTempPrefix = "temp-";
static const std::string kLabelPrefix = ".label";
std::string NewTemp() {
  static int i = 0;
  return kTempPrefix + std::to_string(i++);
}

std::string NewLabel() {
  static int i = 0;
  return kLabelPrefix + std::to_string(i++);
}

Opn::Opn(const Opn &opn) : type_(opn.type_), imm_num_(opn.imm_num_), scope_id_(opn.scope_id_), ssa_id_(opn.ssa_id_) {
  name_ = opn.name_;
  if (nullptr != opn.offset_) offset_ = new Opn(*opn.offset_);
}
Opn &Opn::operator=(const Opn &opn) {
  type_ = opn.type_, imm_num_ = opn.imm_num_, scope_id_ = opn.scope_id_, ssa_id_ = opn.ssa_id_;
  name_ = opn.name_;
  if (nullptr != offset_) {
    delete offset_;
    offset_ = nullptr;
  }
  if (nullptr != opn.offset_) offset_ = new Opn(*opn.offset_);
  return *this;
}
Opn::Opn(Opn &&opn)
    : type_(opn.type_), imm_num_(opn.imm_num_), scope_id_(opn.scope_id_), offset_(opn.offset_), ssa_id_(opn.ssa_id_) {
  name_ = std::move(opn.name_);
}
Opn &Opn::operator=(Opn &&opn) {
  type_ = opn.type_, imm_num_ = opn.imm_num_, scope_id_ = opn.scope_id_, ssa_id_ = opn.ssa_id_;
  if (nullptr != offset_) {
    delete offset_;
    offset_ = nullptr;
  }
  offset_ = opn.offset_;
  name_ = std::move(opn.name_);
  return *this;
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
std::string Opn::GetCompName() {  // 对于全局变量直接返回原name_ 对于局部变量返回复合name
  if (type_ == Type::Imm || type_ == Type::Null) MyAssert(0);
  if (type_ == Type::Label || type_ == Type::Func || scope_id_ == 0) return name_;
  auto res = scope_id_ == -1 ? name_ : name_ + "." + std::to_string(scope_id_);
  return ssa_id_ == -1 ? res : res + "." + std::to_string(ssa_id_);
}
Opn::operator std::string() {
  switch (type_) {
    case Type::Var: {
      if (scope_id_ == 0) return name_;
      auto ret = scope_id_ == -1 ? name_ : name_ + "." + std::to_string(scope_id_);
      return ssa_id_ == -1 ? ret : ret + "." + std::to_string(ssa_id_);
    }
    case Type::Array: {
      auto ret = scope_id_ == -1 ? name_ : name_ + "." + std::to_string(scope_id_);
      ret += (ssa_id_ == -1 ? "" : "." + std::to_string(ssa_id_));
      if (scope_id_ == 0) ret = name_;
      return nullptr == offset_ ? ret : ret + "[" + std::string(*offset_) + "]";
    }
    default: {
      return name_;
    }
  }
}

IR::IR(IR &&ir) : op_(ir.op_) {
  opn1_ = std::move(ir.opn1_), opn2_ = std::move(ir.opn2_), res_ = std::move(ir.res_);
  phi_args_ = std::move(ir.phi_args_);
}
IR &IR::operator=(IR &&ir) {
  opn1_ = std::move(ir.opn1_), opn2_ = std::move(ir.opn2_), res_ = std::move(ir.res_);
  phi_args_ = std::move(ir.phi_args_);
  return *this;
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
    case IR::OpKind::ASSIGN_OFFSET:
      PRINT_IR("=[]");
      break;
    case IR::OpKind::PHI:
      outfile << "(" << std::setw(15) << "phi";
      for (auto &arg : phi_args_) outfile << std::setw(15) << std::string(arg);
      outfile << std::setw(15) << std::string(res_) << ")" << std::endl;
      break;
    case IR::OpKind::ALLOCA:
      PRINT_IR("alloca");
      break;
    case IR::OpKind::DECLARE:
      PRINT_IR("declare");
      break;
    default:
      MyAssert(0);
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

#undef ASSERT_ENABLE
