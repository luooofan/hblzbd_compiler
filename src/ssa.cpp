#include "../include/ssa.h"

#include <iomanip>

#include "../include/ssa_struct.h"
#define INDENT 12
#define ASSERT_ENABLE
#include "../include/myassert.h"

void Value::KillUse(Value *val) {
  for (auto it = use_list_.begin(); it != use_list_.end();) {
    if (val == (*it)->Get()) {
      it = use_list_.erase(it);
    } else {
      ++it;
    }
  }
}
void Value::KillUse(FunctionValue *val) { KillUse(val->GetFunction()); }
void Value::KillUse(SSAFunction *func) {
  // 删除该值在该函数中的全部使用
  for (auto it = use_list_.begin(); it != use_list_.end();) {
    auto inst = dynamic_cast<SSAInstruction *>((*it)->Get());
    if (nullptr != inst && inst->GetParent()->GetFunction() == func) {
      it = use_list_.erase(it);
    } else {
      ++it;
    }
  }
}
void Value::ReplaceAllUseWith(Value *val) {
  auto uses = GetUses();
  for (auto use : uses) {
    use->Set(val);
  }
  MyAssert(!IsUsed());
}
Value::~Value() {
  auto uses = GetUses();  // necessary. can't use reference
  for (auto use : uses) {
    use->Set(nullptr);  // necessary. when destruct a use instance, must ensure that its value_ is a valid pointer.
  }
  // 并没有删除使用它的那个user的operand 只是让operand的use中的value变为nullptr了
  MyAssert(!IsUsed());
}

// FunctionValue::FunctionValue(FunctionType *type, const std::string &name, SSAFunction *func) : Value(type, name) {
//   func->BindValue(this);
// }
// FunctionValue::FunctionValue(const std::string &name) : Value(new Type(Type::FunctionTyID), name) {}

FunctionValue::FunctionValue(const std::string &name, SSAFunction *func)
    : Value(new Type(Type::FunctionTyID), name), func_(func) {
  func->BindValue(this);
}

BasicBlockValue::BasicBlockValue(const std::string &name, SSABasicBlock *bb)
    : Value(new Type(Type::LabelTyID), name), bb_(bb) {
  bb->BindValue(this);
}

User::~User() {
  // MyAssert(!IsUsed()); // delete module 或者 function时 才允许仍有使用
}
Value *User::GetOperand(unsigned i) {
  MyAssert(i < GetNumOperands());
  return operands_[i].Get();
}

SSAInstruction::SSAInstruction(Type *type, const std::string &name, SSABasicBlock *parent)
    : User(type, name), parent_(nullptr) {
  parent->AddInstruction(this);
};
SSAInstruction::SSAInstruction(Type *type, const std::string &name, SSAInstruction *inst)
    : User(type, name), parent_(nullptr) {
  inst->parent_->AddInstruction(this, inst);
};
void SSAInstruction::Remove() { GetParent()->RemoveInstruction(this); }

int BinaryOperator::ComputeConstInt(int lhs, int rhs) {
  switch (op_) {
    case ADD:
      return lhs + rhs;
    case SUB:
      return lhs - rhs;
    case MUL:
      return lhs * rhs;
    case DIV:
      return lhs / rhs;
    case MOD:
      return lhs % rhs;
    default:
      MyAssert(0);
      break;
  }
  return 0;
}

int UnaryOperator::ComputeConstInt(int lhs) {
  switch (op_) {
    case NEG:
      return -lhs;
    case NOT:
      return !lhs;
    default:
      MyAssert(0);
      break;
  }
  return 0;
}

void Value::Print(std::ostream &outfile) { outfile << std::setw(INDENT) << "%" + name_; }

void ConstantInt::Print(std::ostream &outfile) { outfile << std::setw(INDENT) << "#" + std::to_string(imm_); }

void GlobalVariable::Print(std::ostream &outfile) {
  outfile << "@_glob_var_" << GetName() << " size: " << size_ << std::endl;
}

void UndefVariable::Print(std::ostream &outfile) { Value::Print(outfile); }

void Argument::Print(std::ostream &outfile) { Value::Print(outfile); }

void FunctionValue::Print(std::ostream &outfile) { outfile << std::setw(INDENT) << GetName(); }

void BasicBlockValue::Print(std::ostream &outfile) { Value::Print(outfile); }

void User::Print(std::ostream &outfile) {
  for (auto &opn : operands_) {
    auto val = opn.Get();
    outfile << std::setw(INDENT);
    if (nullptr == val) {
      outfile << "nullptr";
    } else if (dynamic_cast<GlobalVariable *>(val)) {
      outfile << "%g:" + val->GetName();
    } else if (auto src_val = dynamic_cast<ConstantInt *>(val)) {
      outfile << "#" + std::to_string(src_val->GetImm());
    } else if (dynamic_cast<BasicBlockValue *>(val) || dynamic_cast<FunctionValue *>(val)) {
      outfile << "" + val->GetName();
    } else if (auto src_val = dynamic_cast<Argument *>(val)) {
      outfile << "%a" + std::to_string(src_val->GetArgNo()) + ":" + val->GetName();
    } else {
      outfile << "%" + val->GetName();
    }
  }
}

void BinaryOperator::Print(std::ostream &outfile) {
  std::string op = "";
  switch (op_) {
    case ADD:
      op = "add";
      break;
    case SUB:
      op = "sub";
      break;
    case MUL:
      op = "mul";
      break;
    case DIV:
      op = "div";
      break;
    case MOD:
      op = "mod";
      break;
    default:
      break;
  }
  Value::Print(outfile);
  outfile << std::setw(INDENT) << "=" + op;
  User::Print(outfile);
  outfile << std::endl;
}
void UnaryOperator::Print(std::ostream &outfile) {
  std::string op = "";
  switch (op_) {
    case NEG:
      op = "neg";
      break;
    case NOT:
      op = "not";
      break;
    default:
      break;
  }
  Value::Print(outfile);
  outfile << std::setw(INDENT) << "=" + op;
  User::Print(outfile);
  outfile << std::endl;
}
void BranchInst::Print(std::ostream &outfile) {
  std::string cond = "b";
  switch (cond_) {
    case AL:
      cond += "";
      break;
    case EQ:
      cond += "eq";
      break;
    case NE:
      cond += "ne";
      break;
    case GT:
      cond += "gt";
      break;
    case GE:
      cond += "ge";
      break;
    case LT:
      cond += "lt";
      break;
    case LE:
      cond += "le";
      break;
    default:
      break;
  }
  outfile << std::setw(INDENT) << cond;
  User::Print(outfile);
  outfile << std::endl;
}
void CallInst::Print(std::ostream &outfile) {
  std::string prefix = "";
  if (!GetType()->IsVoid()) {
    Value::Print(outfile);
    prefix = "=";
  }
  prefix += "call";
  outfile << std::setw(INDENT) << prefix;
  User::Print(outfile);
  outfile << std::endl;
}
void ReturnInst::Print(std::ostream &outfile) {
  outfile << std::setw(INDENT) << "return";
  User::Print(outfile);
  outfile << std::endl;
}
void AllocaInst::Print(std::ostream &outfile) {
  outfile << std::setw(INDENT) << "alloca";
  // outfile << (dynamic_cast<ArrayType *>(GetType()))->num_elements_;
  User::Print(outfile);
  outfile << std::endl;
}
void LoadInst::Print(std::ostream &outfile) {
  Value::Print(outfile);
  outfile << std::setw(INDENT) << "=load";
  User::Print(outfile);
  outfile << std::endl;
}
void StoreInst::Print(std::ostream &outfile) {
  outfile << std::setw(INDENT) << "store";
  User::Print(outfile);
  outfile << std::endl;
}
void MovInst::Print(std::ostream &outfile) {
  Value::Print(outfile);
  outfile << std::setw(INDENT) << "=mov";
  User::Print(outfile);
  outfile << std::endl;
}
void PhiInst::Print(std::ostream &outfile) {
  Value::Print(outfile);
  outfile << std::setw(INDENT) << "=phi";
  User::Print(outfile);
  outfile << std::endl;
}

#undef ASSERT_ENABLE