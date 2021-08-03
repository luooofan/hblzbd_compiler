#include "../include/ssa.h"

#include "../include/ssa_struct.h"

FunctionValue::FunctionValue(FunctionType *type, const std::string &name, SSAFunction *func) : Value(type, name) {
  func->BindValue(this);
}

BasicBlockValue::BasicBlockValue(const std::string &name, SSABasicBlock *bb) : Value(new Type(Type::LabelTyID), name) {
  bb->BindValue(this);
}

SSAInstruction::SSAInstruction(Type *type, const std::string &name, SSABasicBlock *parent)
    : User(type, name), parent_(nullptr) {
  parent->AddInstruction(this);
};

void Value::Print(std::ostream &outfile) { outfile << "%" << name_; }

void ConstantInt::Print(std::ostream &outfile) { outfile << "#" << imm_; }
void GlobalVariable::Print(std::ostream &outfile) {
  outfile << "@_glob_var_" << GetName() << " size: " << size_ << std::endl;
}
void UndefVariable::Print(std::ostream &outfile) { Value::Print(outfile); }

void Argument::Print(std::ostream &outfile) {}

void FunctionValue::Print(std::ostream &outfile) { Value::Print(outfile); }

void BasicBlockValue::Print(std::ostream &outfile) { Value::Print(outfile); }

void User::Print(std::ostream &outfile) {
  for (auto &opn : operands_) {
    auto val = opn.Get();
    if (dynamic_cast<GlobalVariable *>(val)) {
      outfile << "@" << val->GetName();
    } else if (auto src_val = dynamic_cast<ConstantInt *>(val)) {
      outfile << "#" << src_val->imm_;
    } else if (dynamic_cast<BasicBlockValue *>(val) || dynamic_cast<FunctionValue *>(val)) {
      outfile << "" << val->GetName();
    } else {
      outfile << "%" << val->GetName();
    }
    outfile << " ";
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
  outfile << " = " << op << " ";
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
  outfile << op << " ";
  User::Print(outfile);
  outfile << std::endl;
}
void BranchInst::Print(std::ostream &outfile) {
  outfile << "b";
  std::string cond = "";
  switch (cond_) {
    case AL:
      cond = "";
      break;
    case EQ:
      cond = "eq";
      break;
    case NE:
      cond = "ne";
      break;
    case GT:
      cond = "gt";
      break;
    case GE:
      cond = "ge";
      break;
    case LT:
      cond = "lt";
      break;
    case LE:
      cond = "le";
      break;
    default:
      break;
  }
  outfile << cond << " ";
  User::Print(outfile);
  outfile << std::endl;
}
void CallInst::Print(std::ostream &outfile) {
  outfile << "call";
  outfile << std::endl;
}
void ReturnInst::Print(std::ostream &outfile) {
  outfile << "return ";
  User::Print(outfile);
  outfile << std::endl;
}
void AllocaInst::Print(std::ostream &outfile) {
  outfile << "alloca ";
  // outfile << (dynamic_cast<ArrayType *>(GetType()))->num_elements_;
  User::Print(outfile);
  outfile << std::endl;
}
void LoadInst::Print(std::ostream &outfile) {
  Value::Print(outfile);
  outfile << " = load ";
  User::Print(outfile);
  outfile << std::endl;
}
void StoreInst::Print(std::ostream &outfile) {
  outfile << "store ";
  User::Print(outfile);
  outfile << std::endl;
}
void MovInst::Print(std::ostream &outfile) {
  Value::Print(outfile);
  outfile << " = ";
  User::Print(outfile);
  outfile << std::endl;
}
void PhiInst::Print(std::ostream &outfile) {
  Value::Print(outfile);
  outfile << " = phi ";
  User::Print(outfile);
  outfile << std::endl;
}