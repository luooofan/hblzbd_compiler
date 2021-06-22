#include "../include/ast.h"

namespace ast
{

  void Root::GenerateIR()
  {
  }
  void Number::GenerateIR()
  {
  }
  void Identifier::GenerateIR()
  {
  }
  void ArrayIdentifier::GenerateIR() {}
  void ConditionExpression::GenerateIR()
  {
  }

  void BinaryExpression::GenerateIR()
  {
  }
  void UnaryExpression::GenerateIR() {}
  void FunctionCall::GenerateIR() {}

  void VariableDefine::GenerateIR()
  {
  }
  void VariableDefineWithInit::GenerateIR() {}
  void ArrayDefine::GenerateIR() {}
  void ArrayDefineWithInit::GenerateIR() {}
  void FunctionDefine::GenerateIR()
  {
  }

  void AssignStatement::GenerateIR()
  {
  }

  void IfElseStatement::GenerateIR()
  {
  }

  void WhileStatement::GenerateIR()
  {
  }

  void BreakStatement::GenerateIR()
  {
  }

  void ContinueStatement::GenerateIR()
  {
  }

  void ReturnStatement::GenerateIR()
  {
  }

  void VoidStatement::GenerateIR() {}

  void EvalStatement::GenerateIR() {}

  void Block::GenerateIR()
  {
  }

  void DeclareStatement::GenerateIR()
  {
  }
  void ArrayInitVal::GenerateIR() {}
  void FunctionFormalParameter::GenerateIR() {}
  void FunctionFormalParameterList::GenerateIR() {}
  void FunctionActualParameterList::GenerateIR() {}

} // namespace ast
