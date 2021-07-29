#ifndef __DAG_H__
#define __DAG_H__
/*用于定义基本块的DAG结构以及由基本块生成DAG的方法*/
#include <string>

class DAG_node{
public:
  enum class Type{
    Var,
    Imm,
    Op 
  };

  Type type_;
  int _imm_num_;
  std::string name_;
  int scope_id_;
  int op_;

  DAG_node *left, *right;

  // 变量类型
  DAG_node(Type type_, )
};

#endif