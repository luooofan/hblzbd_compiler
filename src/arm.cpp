#include "../include/arm.h"

#include <cassert>
namespace arm {

// TODO: 是否要构建一个gARMList?
// std::vector<Instruction> gASMList;

std::string CondToString(Cond cond) {
  switch (cond) {
    case Cond::AL:
      return "";
    case Cond::EQ:
      return "eq";
    case Cond::NE:
      return "ne";
    case Cond::GT:
      return "gt";
    case Cond::GE:
      return "ge";
    case Cond::LT:
      return "lt";
    case Cond::LE:
      return "le";
  }
  return "";
}

BinaryInst::~BinaryInst() {
  // if(nullptr!=this->rd_){
  //     delete this->rd_;
  // }
  // if(nullptr!=this->rn_){

  // }
}
void BinaryInst::EmitCode(std::ostream& outfile) {
  std::string opcode;
  switch (this->opcode_) {
    case OpCode::ADD:
      opcode = "add";
      break;
    case OpCode::SUB:
      opcode = "sub";
      break;
    case OpCode::RSB:
      opcode = "rsb";
      break;
    case OpCode::SDIV:
      opcode = "sdiv";
      break;
    case OpCode::MUL:
      opcode = "mul";
      break;
    case OpCode::AND:
      opcode = "and";
      break;
    case OpCode::EOR:
      opcode = "eor";
      break;
    case OpCode::ORR:
      opcode = "orr";
      break;
    case OpCode::RON:
      opcode = "ron";
      break;
    case OpCode::BIC:
      opcode = "bic";
      break;
    case OpCode::CMP:
      opcode = "cmp";
      break;
    case OpCode::CMN:
      opcode = "cmn";
      break;
    case OpCode::TST:
      opcode = "tst";
      break;
    case OpCode::TEQ:
      opcode = "teq";
      break;
    default:
      opcode = "none";
      break;
  }
  if (this->HasS()) opcode += "s";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << (nullptr != this->rd_ ? (std::string(*this->rd_) + ", ") : "")
          << std::string(*this->rn_) << ", " << std::string(*this->op2_) << std::endl;
}

Move::~Move() {}
void Move::EmitCode(std::ostream& outfile) {
  std::string opcode = "mov";
  if (this->HasS()) opcode += "s";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << std::string(*this->rd_);
  outfile << ", " << std::string(*this->op2_) << std::endl;
}

Branch::~Branch() {}
void Branch::EmitCode(std::ostream& outfile) {
  std::string opcode = "b";
  if (this->has_l_) opcode += "l";
  // NOTE:
  if (this->has_x_) opcode += "x";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << this->label_ << std::endl;
}

LdrStr::~LdrStr() {}
void LdrStr::EmitCode(std::ostream& outfile) {
  std::string prefix = this->opkind_ == OpKind::LDR ? "ldr" : "str";
  prefix += CondToString(this->cond_) + " " + std::string(*(this->rd_)) + ", ";
  switch (this->type_) {
    // NOTE: offset为0也要输出
    case Type::Norm: {
      prefix += "[" + std::string(*(this->rn_)) + ", " + std::string(*(this->offset_)) + "]";
      break;
    }
    case Type::Pre: {
      prefix += "[" + std::string(*(this->rn_)) + ", " + std::string(*(this->offset_)) + "]!";
      break;
    }
    case Type::Post: {
      prefix += "[" + std::string(*(this->rn_)) + "], " + std::string(*(this->offset_));
      break;
    }
    case Type::PCrel: {
      prefix += "=" + this->label_;
      break;
    }
    default: {
      assert(0);
      break;
    }
  }
  outfile << "\t" << prefix << std::endl;
}

PushPop::~PushPop() {}
void PushPop::EmitCode(std::ostream& outfile) {
  assert(!this->reg_list_.empty());
  std::string opcode = this->opkind_ == OpKind::PUSH ? "push" : "pop";
  outfile << "\t" << opcode << " { " << std::string(*(this->reg_list_[0]));
  for (auto iter = this->reg_list_.begin() + 1; iter != this->reg_list_.end(); ++iter) {
    outfile << ", " << std::string(**iter);
  }
  outfile << " }" << std::endl;
}

}  // namespace arm