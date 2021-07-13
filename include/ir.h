#ifndef __IR__H_
#define __IR__H_

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
  int offset_;              // 函数栈中偏移(中间变量偏移为-1)
  std::vector<int> shape_;  // int为空 否则表示数组的各个维度
  std::vector<int> width_;  // int为空 否则表示数组的各个维度的宽度
  std::vector<int> initval_;  // 用于记录全局变量的初始值 int为一个值
                              // 数组则转换为一维数组存储
  SymbolTableItem(bool is_array, bool is_const, int offset)
      : is_array_(is_array), is_const_(is_const), offset_(offset) {}
  void Print();
};

class FuncTableItem {
 public:
  int size_;      // 函数栈大小(不考虑中间变量)
  int ret_type_;  // VOID INT
  std::vector<std::vector<int>> shape_list_;
  // TODO: 如何处理库函数的size
  FuncTableItem(int ret_type) : ret_type_(ret_type), size_(0) {}
  // FuncTableItem() {}
  void Print();
};

//每个作用域一个符号表
class Scope {
 public:
  std::unordered_map<std::string, SymbolTableItem> symbol_table_;
  int scope_id_;         // 作用域id
  int parent_scope_id_;  // 父作用域id
  int dynamic_offset_;  // 当前作用域符号表中最后一个有效项在函数栈中的偏移
  // 往符号表中插入中间变量时不改变该值
  // bool is_func_;
  bool IsFunc() { return parent_scope_id_ == 0 ? true : false; }
  // Scope() {}
  Scope(int scope_id, int parent_scope_id, int dynamic_offset)
      : scope_id_(scope_id),
        parent_scope_id_(parent_scope_id),
        dynamic_offset_(dynamic_offset) {}
  void Print();
};

SymbolTableItem *FindSymbol(int scope_id, std::string name);

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
  int imm_num_;       // 立即数
  std::string name_;  //
  int scope_id_;  // 标识所在作用域 -1表示全局作用域的父作用域
  Opn *offset_;
  // Opn Type: IMM
  Opn(Type type, int imm_num, int scope_id)
      : type_(type),
        imm_num_(imm_num),
        name_("#" + std::to_string(imm_num)),
        scope_id_(scope_id),
        offset_(nullptr) {}
  // Opn Type: Var or Label or Func
  Opn(Type type, std::string name, int scope_id)
      : type_(type), name_(name), scope_id_(scope_id), offset_(nullptr) {}
  // Opn Type: Null
  Opn(Type type) : type_(type), name_("-"), offset_(nullptr) { scope_id_ = -1; }
  // Opn Type: Array
  Opn(Type type, std::string name, int scope_id, Opn *offset)
      : type_(type), name_(name), scope_id_(scope_id), offset_(offset) {}
  Opn() {}
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
    // OFFSET_ASSIGN,  // []=
    // ASSIGN_OFFSET,  // =[]
  };
  OpKind op_;
  Opn opn1_, opn2_, res_;
  int offset_;  // only used for []instruction
  IR(OpKind op, Opn opn1, Opn opn2, Opn res)
      : op_(op), opn1_(opn1), opn2_(opn2), res_(res), offset_(0) {}
  IR(OpKind op, Opn opn1, Opn res, int offset)
      : op_(op),
        opn1_(opn1),
        opn2_({Opn::Type::Null}),
        res_(res),
        offset_(offset) {}
  IR(OpKind op, Opn opn1, Opn res)
      : op_(op), opn1_(opn1), opn2_({Opn::Type::Null}), res_(res), offset_(0) {}
  IR(OpKind op, Opn opn1)
      : op_(op),
        opn1_(opn1),
        opn2_({Opn::Type::Null}),
        res_({Opn::Type::Null}),
        offset_(0) {}
  IR(OpKind op)
      : op_(op),
        opn1_({Opn::Type::Null}),
        opn2_({Opn::Type::Null}),
        res_({Opn::Type::Null}),
        offset_(0) {}
  IR() {}
  void PrintIR();
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
  int brace_num_;  // 当前位置(array_offset_)有几个大括号
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

  ContextInfo() : opn_({Opn::Type::Null}), scope_id_(0) {}
};

extern Scopes gScopes;
extern FuncTable gFuncTable;
extern std::vector<IR> gIRList;
extern const int kIntWidth;

std::string NewTemp();
std::string NewLabel();

IR::OpKind GetOpKind(int op, bool reverse);

void PrintScopes();
void PrintFuncTable();
void SemanticError(int line_no, const std::string &&error_msg);
void RuntimeError(const std::string &&error_msg);

}  // namespace ir
#endif
