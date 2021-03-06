#ifndef __IR__H_
#define __IR__H_

#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace ir {

class SymbolTableItem {
 public:
  bool is_array_;
  bool is_const_;
  std::vector<int> shape_;  // int为空 否则表示数组的各个维度
  std::vector<int> width_;  // int为空 否则表示数组的各个维度的宽度
  std::vector<int> initval_;  // 用于记录全局变量的初始值 int为一个值 // 数组则转换为一维数组存储
  SymbolTableItem(bool is_array, bool is_const) : is_array_(is_array), is_const_(is_const) {
    shape_.reserve(5), width_.reserve(5), initval_.reserve(5);
  }
  SymbolTableItem() { shape_.reserve(5), width_.reserve(5), initval_.reserve(5); }
  void Print(std::ostream &outfile = std::clog);
};

class FuncTableItem {
 public:
  int ret_type_;  // VOID INT
  int scope_id_;
  std::vector<std::vector<int>> shape_list_;
  // TODO: 如何处理库函数的size
  FuncTableItem(int ret_type, int scope_id) : ret_type_(ret_type), scope_id_(scope_id) {}
  FuncTableItem() {}
  void Print(std::ostream &outfile = std::clog);
};

//每个作用域一个符号表
class Scope {
 public:
  std::unordered_map<std::string, SymbolTableItem> symbol_table_;
  int scope_id_;         // 作用域id
  int parent_scope_id_;  // 父作用域id
  // 往符号表中插入中间变量时不改变该值
  bool IsFunc() { return parent_scope_id_ == 0 ? true : false; }
  // Scope() {}
  Scope(int scope_id, int parent_scope_id) : scope_id_(scope_id), parent_scope_id_(parent_scope_id) {}
  void Print(std::ostream &outfile = std::clog);
  bool IsSubScope(int scope_id);
};

int FindSymbol(int scope_id, std::string name, SymbolTableItem *&res_symbol_item);

using Scopes = std::vector<Scope>;
using FuncTable = std::unordered_map<std::string, FuncTableItem>;

class Opn {
 public:
  enum class Type {
    Var,
    Imm,
    Label,
    Func,
    Null,  // 无操作数
    Array,
  };
  Type type_;
  int imm_num_;        // 立即数
  std::string name_;   //
  int scope_id_ = -1;  // 标识所在作用域 -1表示全局作用域的父作用域
  Opn *offset_ = nullptr;
  int ssa_id_ = -1;
  // Opn Type: IMM
  Opn(Type type, int imm_num) : type_(type), imm_num_(imm_num), name_("#" + std::to_string(imm_num)) {}
  // Opn Type: Var
  Opn(Type type, std::string name, int scope_id) : type_(type), name_(name), scope_id_(scope_id) {}
  // Opn Type: Label or Func
  Opn(Type type, std::string name) : type_(type), name_(name) {}
  // Opn Type: Null
  Opn(Type type) : type_(type), name_("-") {}
  // Opn Type: Array
  Opn(Type type, std::string name, int scope_id, Opn *offset)
      : type_(type), name_(name), scope_id_(scope_id), offset_(offset) {}
  Opn() : type_(Type::Null), name_("-") {}
  Opn(const Opn &opn);
  Opn &operator=(const Opn &opn);
  Opn(Opn &&opn);
  Opn &operator=(Opn &&opn);
  std::string GetCompName();
  explicit operator std::string();
};

class IR {
 public:
  enum class OpKind {
    ADD,     // (+,)
    SUB,     // (-,)
    MUL,     // (*,)
    DIV,     // (/,)
    MOD,     // (%,)
    NOT,     // (!,)
    NEG,     // (-,)负
    LABEL,   // (label,)
    PARAM,   // (param,)
    CALL,    // (call, func, num, res|-)
    RET,     // (ret,) or (ret,opn1,)
    GOTO,    // (goto,label)
    ASSIGN,  // (assign, opn1,-,res)
    JEQ,     // ==
    JNE,     // !=
    JLT,     // <
    JLE,     // <=
    JGT,     // >
    JGE,     // >=
    VOID,    // useless
    ASSIGN_OFFSET,  // =[] NOTE: 这个操作符不可省略 不可合并到assign中 因为数组地址和数组取值是不一样的
    PHI,            //
    ALLOCA,         //
    DECLARE,        // 函数参数声明
  };
  OpKind op_;
  Opn opn1_, opn2_, res_;
  std::vector<Opn> phi_args_;
  IR(OpKind op, Opn res, int size) : op_(op), res_(res) { phi_args_.resize(size); }
  IR(OpKind op, Opn opn1, Opn opn2, Opn res) : op_(op), opn1_(opn1), opn2_(opn2), res_(res) {}
  IR(OpKind op, Opn opn1, Opn res) : op_(op), opn1_(opn1), res_(res) {}
  IR(OpKind op, Opn opn1) : op_(op), opn1_(opn1) {}
  IR(OpKind op) : op_(op) {}
  IR() = default;
  IR(const IR &ir) = default;
  IR &operator=(const IR &ir) = default;
  IR(IR &&ir);
  IR &operator=(IR &&ir);
  void PrintIR(std::ostream &outfile = std::clog);
};

class ContextInfo {
 public:
  int scope_id_;
  Opn opn_;
  bool has_return;
  // Used for type check
  std::vector<int> shape_;
  // Used for ArrayInitVal
  std::string array_name_;
  int array_offset_;
  int brace_num_;                   // 当前位置(array_offset_)有几个大括号
  std::vector<int> dim_total_num_;  // a[2][3][4] -> 24,12,4,1
  // Used for Break Continue
  std::stack<std::string> break_label_;
  std::stack<std::string> continue_label_;
  // Used for Condition Expresson
  std::stack<std::string> true_label_;
  std::stack<std::string> false_label_;
  // Used for Return Statement
  std::string current_func_name_;
  // Used for Block
  bool has_aug_scope;  //函数形参也要加在block的作用域里
  // Used for ArrayIdentifier []=
  bool is_assigned_ = false;

  ContextInfo() : scope_id_(0) { shape_.reserve(5); }
};

extern Scopes gScopes;
extern FuncTable gFuncTable;
// extern std::vector<IR> gIRList;
extern const int kIntWidth;

std::string NewTemp();
std::string NewLabel();

IR::OpKind GetOpKind(int op, bool reverse);

void PrintScopes(std::ostream &outfile = std::clog);
void PrintFuncTable(std::ostream &outfile = std::clog);
void SemanticError(int line_no, const std::string &&error_msg);
void RuntimeError(const std::string &&error_msg);

}  // namespace ir
#endif
