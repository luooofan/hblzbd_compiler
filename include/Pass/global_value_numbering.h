#ifndef __GLOBAL_VALUE_NUMBERING_H__
#define __GLOBAL_VALUE_NUMBERING_H__

#include <map>
#include <unordered_set>
#include <vector>

#include "./pass_manager.h"
class Value;
class Module;
class SSABasicBlock;
class SSAFunction;
class SSAInstruction;

// Require: def-use info. dom tree info. no_side_effect_functions info.
class GlobalValueNumbering : public Transform {
 public:
  GlobalValueNumbering(Module** m) : Transform(m) {}
  void Run() override;
  void Run4Func(SSAFunction* func);
  void CommenExpressionEliminate(SSABasicBlock* bb);

  // actually useless
  static void DFS(std::vector<SSABasicBlock*>& po, std::unordered_set<SSABasicBlock*>& vis, SSABasicBlock* bb);
  static std::vector<SSABasicBlock*> ComputeRPO(SSAFunction* func);

 private:
  // 一个Value唯一标识一个值(except ConstantInt) 所以Value*可作为number 不必真正地编号
  // 创建一个表达式类型 有op和oprands 如果op值相等 operands(value*)也都一致 就认为表达式一致 要考虑符合交换律的情况
  // 一个有定值能被使用[used]的语句被认为是一个表达式
  // 暂时不处理mov和phi表达式 load表达式待定(可以从store传播到load 也可以从load传播到load)
  // 所以其实目前只处理binary unary call表达式
  // 需要记录表达式到value*的映射 一旦发现冗余表达式 删除 替换值即可
  // 虽然值编号也可以删除值冗余 但由于已经在SimpleOptimizePass中实现 所以此实现只关注冗余表达式
  // 如果此Pass之前已经做过SimpleOptimizePass 因为此Pass不会生成新的mov语句 也不会传播常数 所以不会带来新的优化机会

  struct Expression {
    enum OpKind {
      // BinaryOperator
      ADD,
      SUB,
      MUL,
      DIV,
      MOD,
      // UnaryOperator
      NEG,
      NOT,
      // Others
      CALL,
      LOAD
    };
    OpKind op_;
    std::vector<Value*> operands_;
    SSAInstruction* inst_ = nullptr;
    // Expression() = default;
    // Expression(OpKind op) : op_(op) {}
    Expression(SSAInstruction* inst) : inst_(inst) {}
    ~Expression() = default;
    bool operator<(const Expression& exp) const;
    explicit operator std::string() const;
  };

  std::unordered_set<SSAFunction*> no_side_effect_funcs;
  std::vector<std::pair<Expression, Value*>> exp_val_vec;
};

#endif