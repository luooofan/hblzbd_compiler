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

  // 如果只有一个子节点则副歌left指针
  DAG_node *left_, *right_;
  // 定值变量表中变量的形式： 变量名_#scope_id
  std::unordered_set<std::string> var_list_;
  // 只用于处理CALL语句和PARAM语句
  std::vector<ir::IR*> param_and_call_;

  // 变量类型
  DAG_node(Type type, std::string name, int scope_id) : type_(type), name_(name), scope_id_(scope_id) {op_ = ir::IR::OpKind::VOID; imm_num_ = 0; left_ = right_ = nullptr; var_list_.clear();}
  // 立即数类型
  DAG_node(Type type, int imm_num) : type_(type), imm_num_(imm_num) {op_ = ir::IR::OpKind::VOID; name_ = "", scope_id_ = -1; left_ = right_ = nullptr; var_list_.clear();}
  // 中间节点类型
  DAG_node(Type type, ir::IR::OpKind op, DAG_node *left, DAG_node* right) : type_(type), op_(op), left_(left), right_(right) {imm_num_ = 0; name_ = ""; scope_id_ = -1; var_list_.clear();}
  // 标签类型
  DAG_node(Type type, std::string name) : type_(type), name_(name) {imm_num_ = 0; scope_id_ = -1; op_ = ir::IR::OpKind::VOID; left_ = right_ = nullptr; var_list_.clear();}
  // 函数类型
  // DAG_node(Type type, std::string name) : type_(type), name_(name) {imm_num_ = 0; scope_id_ = -1; op_ = ir::IR::OpKind::VOID; left_ = right_ = nullptr; var_list_.clear();}
  // 空类型
  DAG_node(Type type) : type_(type) {imm_num_ = 0; scope_id_ = -1; op_ = ir::IR::OpKind::VOID; left_ = right_ = nullptr; var_list_.clear(); name_ = "";}
  // 函数调用类型
  DAG_node(Type type, std::string func_name, ir::IR::OpKind op, int arg_num) : type_(type), name_(func_name), op_(op), imm_num_(arg_num) {scope_id_ = -1; left_ = right_ = nullptr; }
};


class DAG_analysis : Analysis{
public:
  DAG_analysis(Module **m) : Analysis(m) {}

  void Run();
  void Run4bb(IRBasicBlock *bb);
};

#endif