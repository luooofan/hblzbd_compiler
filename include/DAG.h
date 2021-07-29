#ifndef __DAG_H__
#define __DAG_H__
/*用于定义基本块的DAG结构以及由基本块生成DAG的方法*/
#include <string>

#include "ir.h"
#include "ir_struct.h"
#include "pass_manager.h"

class DAG_node{
public:
  enum class Type{
    Var,
    Imm,
    Op 
  };

  Type type_;
  int imm_num_;
  std::string name_;
  int scope_id_;
  ir::IR::OpKind op_;

  DAG_node *left_, *right_;

  // 变量类型
  DAG_node(Type type, std::string name, int scope_id) : type_(type), name_(name), scope_id_(scope_id) {}
  // 立即数类型
  DAG_node(Type type, int imm_num) : type_(type), imm_num_(imm_num) {}
  // 中间节点类型
  DAG_node(Type type, ir::IR::OpKind op, DAG_node *left, DAG_node* right) : type_(type), op_(op), left_(left), right_(right) {}
};


class DAG_analysis : Analysis{
public:
  DAG_analysis(Module **m) : Analysis(m) {}

  void Run();
  void Run4bb();
};

#endif