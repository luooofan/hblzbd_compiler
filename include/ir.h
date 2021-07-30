#ifndef __IR__H_
#define __IR__H_

#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace ir {
// TODO: 部分结构应该改为指针或引用
class SymbolTableItem {
 public:
  bool is_array_;
  bool is_const_;
  int offset_;              // 函数栈中偏移(中间变量偏移为-1) offset必须维护好
  int stack_offset_ = 0;    // 实际在栈中的偏移 用于generate arm
  std::vector<int> shape_;  // int为空 否则表示数组的各个维度
  std::vector<int> width_;  // int为空 否则表示数组的各个维度的宽度
  std::vector<int> initval_;  // 用于记录全局变量的初始值 int为一个值 // 数组则转换为一维数组存储
  SymbolTableItem(bool is_array, bool is_const, int offset)
      : is_array_(is_array), is_const_(is_const), offset_(offset) {}
  SymbolTableItem() {}
  bool IsArg() { return is_arg_; }
  bool IsTemp() { return -1 == offset_; }
  void SetIsArg() { is_arg_ = true; }
  bool IsHighArg() { return is_arg_ && offset_ > 4 * 3; }  // HighArg指参数位置>4
  void Print(std::ostream &outfile = std::clog);

 private:
  bool is_arg_ = false;
};

class FuncTableItem {
 public:
  int size_;      // 函数栈大小(不考虑中间变量)
  int ret_type_;  // VOID INT
  int scope_id_;
  std::vector<std::vector<int>> shape_list_;
  // TODO: 如何处理库函数的size
  FuncTableItem(int ret_type, int scope_id) : ret_type_(ret_type), size_(0), scope_id_(scope_id) {}
  FuncTableItem() {}
  void Print(std::ostream &outfile = std::clog);
};

//每个作用域一个符号表
class Scope {
 public:
  std::unordered_map<std::string, SymbolTableItem> symbol_table_;
  int scope_id_;         // 作用域id
  int parent_scope_id_;  // 父作用域id
  int dynamic_offset_;   // 当前作用域符号表中最后一个有效项在函数栈中的偏移
  // 往符号表中插入中间变量时不改变该值
  // bool is_func_;
  bool IsFunc() { return parent_scope_id_ == 0 ? true : false; }
  // Scope() {}
  Scope(int scope_id, int parent_scope_id, int dynamic_offset)
      : scope_id_(scope_id), parent_scope_id_(parent_scope_id), dynamic_offset_(dynamic_offset) {}
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
  Opn(Type type, int imm_num, int scope_id)
      : type_(type), imm_num_(imm_num), name_("#" + std::to_string(imm_num)), scope_id_(scope_id), offset_(nullptr) {}
  // Opn Type: Var or Label or Func
  Opn(Type type, std::string name, int scope_id) : type_(type), name_(name), scope_id_(scope_id), offset_(nullptr) {}
  // Opn Type: Null
  Opn(Type type) : type_(type), name_("-"), offset_(nullptr) { scope_id_ = -1; }
  // Opn Type: Array
  Opn(Type type, std::string name, int scope_id, Opn *offset)
      : type_(type), name_(name), scope_id_(scope_id), offset_(offset) {}
  Opn() : type_(Type::Null), name_("-"), offset_(nullptr) { scope_id_ = -1; }
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
    PHI,
    // OFFSET_ASSIGN,  // []=
  };
  OpKind op_;
  Opn opn1_, opn2_, res_;
  std::vector<Opn> phi_args_;
  IR(OpKind op, Opn res, int size) : op_(op), res_(res) { phi_args_.resize(size); }
  IR(OpKind op, Opn opn1, Opn opn2, Opn res) : op_(op), opn1_(opn1), opn2_(opn2), res_(res) {}
  IR(OpKind op, Opn opn1, Opn res) : op_(op), opn1_(opn1), res_(res) {}
  IR(OpKind op, Opn opn1) : op_(op), opn1_(opn1) {}
  IR(OpKind op) : op_(op) {}
  IR() {}
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

  ContextInfo() : scope_id_(0) {}
};

extern Scopes gScopes;
extern FuncTable gFuncTable;
extern std::vector<IR> gIRList;
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
