#ifndef __DAG_H__
#define __DAG_H__
/*用于定义基本块的DAG结构以及由基本块生成DAG的方法*/
#include <string>

#include "ir.h"
#include "ir_struct.h"
#include "./Pass/pass_manager.h"

class DAG_node;
class IRBasicBlock;

DAG_node* createOpNode(ir::Opn* opn);
DAG_node* createOpnNode(ir::Opn* opn);

class DAG_node{
public:
  enum class Type{
    Var,
    Imm,
    Op,
    Label, 
    Func,
    Null,
    Array, 
    Assign
  };

  Type type_;
  int imm_num_;
  std::string name_;
  int scope_id_;
  ir::IR::OpKind op_;
  DAG_node *offset_;    // 对于数组元素存储其偏移

  // 如果只有一个子节点则赋给left指针
  DAG_node *left_, *mediate_,*right_;
  // 定值变量表中变量的形式： 变量名_#scope_id
  std::unordered_set<std::string> var_list_;
  // 用于处理CALL语句和PARAM语句, 还有declare语句也会用到, 还有j~语句也会用到
  std::vector<ir::IR*> ir_list_;

  // 变量类型
  DAG_node(Type type, std::string name, int scope_id) : type_(type), name_(name), scope_id_(scope_id), mediate_(nullptr), offset_(nullptr) {op_ = ir::IR::OpKind::VOID; imm_num_ = 0; left_ = right_ = nullptr; var_list_.clear();}
  // 立即数类型
  DAG_node(Type type, int imm_num) : type_(type), imm_num_(imm_num), mediate_(nullptr), offset_(nullptr) {op_ = ir::IR::OpKind::VOID; name_ = "", scope_id_ = -1; left_ = right_ = nullptr; var_list_.clear();}
  // 中间节点类型
  DAG_node(Type type, ir::IR::OpKind op) : type_(type), op_(op), left_(nullptr), right_(nullptr), mediate_(nullptr), offset_(nullptr) {imm_num_ = 0; name_ = ""; scope_id_ = -1; var_list_.clear();}
  // 标签类型
  DAG_node(Type type, std::string name) : type_(type), name_(name), mediate_(nullptr), offset_(nullptr) {imm_num_ = 0; scope_id_ = -1; op_ = ir::IR::OpKind::VOID; left_ = right_ = nullptr; var_list_.clear();}
  // 函数类型
  // DAG_node(Type type, std::string name) : type_(type), name_(name) {imm_num_ = 0; scope_id_ = -1; op_ = ir::IR::OpKind::VOID; left_ = right_ = nullptr; var_list_.clear();}
  // 空类型
  DAG_node(Type type) : type_(type), mediate_(nullptr), offset_(nullptr) {imm_num_ = 0; scope_id_ = -1; op_ = ir::IR::OpKind::VOID; left_ = right_ = nullptr; var_list_.clear(); name_ = "";}
  // 函数调用类型
  DAG_node(Type type, std::string func_name, ir::IR::OpKind op, int arg_num) : type_(type), name_(func_name), op_(op), imm_num_(arg_num), mediate_(nullptr), offset_(nullptr) {scope_id_ = -1; left_ = right_ = nullptr; }
};


class DAG_analysis : Analysis{
public:
  DAG_analysis(Module **m) : Analysis(m) {}

  void Run();
  void Run4bb(IRBasicBlock *bb);
  DAG_node* FindSameOpNode(IRBasicBlock *bb, ir::IR *ir);
  DAG_node* FindSameOpnNode(IRBasicBlock *bb, ir::Opn *ir);
  void DeleteVar(IRBasicBlock *bb, std::string var);
  void replaceOldArray(IRBasicBlock *bb);
};

#endif