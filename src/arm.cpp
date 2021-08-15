#include "../include/arm.h"

#include <algorithm>
#include <unordered_set>

#include "../include/arm_struct.h"
#define ASSERT_ENABLE
#include "../include/myassert.h"

namespace arm {

Cond GetOppositeCond(Cond cond) {
  switch (cond) {
    case Cond::AL:
      return Cond::AL;
    case Cond::EQ:
      return Cond::NE;
    case Cond::NE:
      return Cond::EQ;
    case Cond::GT:
      return Cond::LE;
    case Cond::GE:
      return Cond::LT;
    case Cond::LT:
      return Cond::GE;
    case Cond::LE:
      return Cond::GT;
    default:
      MyAssert(0);
      break;
  }
}

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
    default:
      MyAssert(0);
      break;
  }
  return "";
}

Shift::operator std::string() {
  std::string opcode = "";
  switch (opcode_) {
    case OpCode::ASR:
      opcode = "asr";
      break;
    case OpCode::LSL:
      opcode = "lsl";
      break;
    case OpCode::LSR:
      opcode = "lsr";
      break;
    case OpCode::ROR:
      opcode = "ror";
      break;
    case OpCode::RRX:
      return "rrx";
    default:
      MyAssert(0);
      break;
  }
  if (is_imm_) {
    return opcode + " #" + std::to_string(shift_imm_);
  } else {
    return opcode + " " + std::string(*shift_reg_);
  }
}

Operand2::operator std::string() {
  return is_imm_ ? ("#" + std::to_string(imm_num_))
                 : (std::string(*reg_) + (nullptr == shift_ || shift_->IsNone() ? "" : ", " + std::string(*shift_)));
}
bool Operand2::CheckImm8m(int imm) {
  // NOTE: assign a int to unsigned int
  unsigned int encoding = imm;
  for (int ror = 0; ror < 32; ror += 2) {
    // 如果encoding的前24bit都是0 说明imm能被表示成一个8bit循环右移偶数位得到
    if (!(encoding & ~0xFFu)) {
      return true;
    }
    encoding = (encoding << 30u) | (encoding >> 2u);
  }
  return false;
}

Instruction::Instruction(Cond cond, ArmBasicBlock* parent, bool push_back) : cond_(cond), parent_(parent) {
  if (push_back) parent_->inst_list_.push_back(this);
}
Instruction::Instruction(ArmBasicBlock* parent, bool push_back) : parent_(parent) {
  if (push_back) parent_->inst_list_.push_back(this);
}
Instruction::Instruction(Instruction* inst) : parent_(inst->parent_) {
  auto& inst_list = parent_->inst_list_;
  auto it = std::find(inst_list.begin(), inst_list.end(), inst);
  MyAssert(it != inst_list.end());
  inst_list.insert(it, this);
}

BinaryInst::~BinaryInst() {}
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
    case OpCode::SDIV:
      opcode = "sdiv";
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
      MyAssert(0);
      break;
  }
  if (this->HasS()) opcode += "s";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << (nullptr != this->rd_ ? (std::string(*this->rd_) + ", ") : "")
          << std::string(*this->rn_) << ", " << std::string(*this->op2_) << std::endl;
}

Move::~Move() {}
void Move::EmitCode(std::ostream& outfile) {
  std::string opcode = this->is_mvn_ ? "mvn" : "mov";
  if (this->HasS()) opcode += "s";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << std::string(*this->rd_);
  outfile << ", " << std::string(*this->op2_) << std::endl;
}

Branch::~Branch() {}
bool Branch::IsCall() {
  // 所有的标签都要由.开头
  if (this->has_l_ && this->label_.size() > 0 && this->label_ != "lr" && this->label_[0] != '.') {
    return true;
  } else {
    return false;
  }
}
bool Branch::IsRet() { return this->has_x_ && this->label_ == "lr"; }
void Branch::EmitCode(std::ostream& outfile) {
  std::string opcode = "b";
  if (this->has_l_) opcode += "l";
  // NOTE: only for bx lr
  if (this->has_x_) opcode += "x";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << this->label_ << std::endl;
}

LdrStr::~LdrStr() {}
void LdrStr::EmitCode(std::ostream& outfile) {
  std::string prefix = this->opkind_ == OpKind::LDR ? "ldr" : "str";
  prefix += CondToString(this->cond_) + " " + std::string(*(this->rd_)) + ", ";
  std::string offset = this->is_offset_imm_ ? "#" + std::to_string(offset_imm_) : std::string(*(this->offset_));
  switch (this->type_) {
    case Type::Norm: {
      prefix += "[" + std::string(*(this->rn_)) + ", " + offset + "]";
      break;
    }
    case Type::Pre: {
      prefix += "[" + std::string(*(this->rn_)) + ", " + offset + "]!";
      break;
    }
    case Type::Post: {
      prefix += "[" + std::string(*(this->rn_)) + "], " + offset;
      break;
    }
    default: {
      MyAssert(0);
      break;
    }
  }
  outfile << "\t" << prefix << std::endl;
}

LdrPseudo::~LdrPseudo() {}
void LdrPseudo::EmitCode(std::ostream& outfile) {
  // convert ldr-pseudo inst to movw&movt
  // https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/how-to-load-constants-in-assembly-for-arm-architecture
  if (!this->is_imm_) {
    outfile << "\tmov32 " << CondToString(this->cond_) << ", " << std::string(*(this->rd_)) + ", " << this->literal_
            << std::endl;
  } else if (0 == ((this->imm_ >> 16) & 0xFFFF)) {
    outfile << "\tmovw" << CondToString(this->cond_) << " " << std::string(*(this->rd_)) + ", #"
            << (this->imm_ & 0xFFFF) << std::endl;
  } else {
    outfile << "\tmov32 " << CondToString(this->cond_) << ", " << std::string(*(this->rd_)) + ", " << this->imm_
            << std::endl;
  }
  // outfile << "\tmov32 " << std::string(*(this->rd_)) << ", "
  //         << (this->is_imm_ ? std::to_string(this->imm_) : this->literal_) << std::endl;
}
PushPop::~PushPop() {}
void PushPop::EmitCode(std::ostream& outfile) {
  MyAssert(!this->reg_list_.empty());
  std::string opcode = this->opkind_ == OpKind::PUSH ? "push" : "pop";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " { " << std::string(*(this->reg_list_[0]));
  for (auto iter = this->reg_list_.begin() + 1; iter != this->reg_list_.end(); ++iter) {
    outfile << ", " << std::string(**iter);
  }
  outfile << " }" << std::endl;
}

}  // namespace arm

#undef ASSERT_ENABLE
