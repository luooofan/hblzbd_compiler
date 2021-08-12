#include "../include/arm.h"

#include <unordered_set>
#define ASSERT_ENABLE
#include "../include/myassert.h"

namespace arm {

std::unordered_set<std::string> gAllLabel;

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

void Reg::Check() { MyAssert(reg_id_ >= 0 && reg_id_ < 16); }

Shift::operator std::string() {
  std::string opcode = "";
  switch (opcode_) {
    case OpCode::ASR:
      opcode = "ASR";
      break;
    case OpCode::LSL:
      opcode = "LSL";
      break;
    case OpCode::LSR:
      opcode = "LSR";
      break;
    case OpCode::ROR:
      opcode = "ROR";
      break;
    case OpCode::RRX:
      return "RRX";
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
void Shift::Check() {
  // 如果是RRX 不应该跟立即数或寄存器 其他移位类型立即数有相应限制
  if (is_imm_) {
    switch (opcode_) {
      case OpCode::LSL:
        MyAssert(shift_imm_ >= 0 && shift_imm_ <= 31);
        break;
      case OpCode::LSR:
      case OpCode::ASR:
        MyAssert(shift_imm_ >= 1 && shift_imm_ <= 32);
        break;
      case OpCode::ROR:
        MyAssert(shift_imm_ >= 1 && shift_imm_ <= 31);
        break;
      case OpCode::RRX:
        MyAssert(0 == shift_imm_);
        break;
      default:
        MyAssert(0);
        break;
    }
  } else {
    MyAssert(nullptr != shift_reg_);
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
void Operand2::Check() {
  if (is_imm_) {
    MyAssert(CheckImm8m(imm_num_));
  } else {
    MyAssert(nullptr != reg_);
    reg_->Check();
    if (nullptr != shift_) shift_->Check();
  }
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
void BinaryInst::Check() {
  if (nullptr != rd_) {
    // TODO: Check
    // MyAssert(opcode_ == OpCode::ADD || opcode_ == OpCode::SUB || opcode_ == OpCode::RSB ||
    //          ((opcode_ == OpCode::MUL || opcode_ == OpCode::SDIV) && !op2_->is_imm_));
    rd_->Check();
  } else {
    MyAssert(opcode_ == OpCode::CMP);
  }
  MyAssert(nullptr != rn_ && nullptr != op2_);
  rn_->Check();
  op2_->Check();
}

Move::~Move() {}
void Move::EmitCode(std::ostream& outfile) {
  std::string opcode = this->is_mvn_ ? "mvn" : "mov";
  if (this->HasS()) opcode += "s";
  opcode += CondToString(this->cond_);
  outfile << "\t" << opcode << " " << std::string(*this->rd_);
  outfile << ", " << std::string(*this->op2_) << std::endl;
}
void Move::Check() {
  MyAssert(nullptr != rd_ && nullptr != op2_);
  rd_->Check();
  op2_->Check();
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
void Branch::Check() {
  MyAssert(label_ != "" && label_ != "putf");  // 要么以点开头 要么是函数名
  // std::cout << label_ << std::endl;
  // if (label_ != "lr") MyAssert(arm::gAllLabel.find(label_) != arm::gAllLabel.end());
  // this->EmitCode();
  if (has_x_) {
    MyAssert(label_ == "lr");
  }  // 目前没有blx bx只能是bx lr
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
void LdrStr::Check() {
  MyAssert(nullptr != rd_ && nullptr != rn_);
  rd_->Check();
  rn_->Check();
  if (is_offset_imm_) {
    MyAssert(CheckImm12(offset_imm_) && nullptr == offset_);
  } else {
    MyAssert(nullptr != offset_);
    offset_->Check();
  }
}

LdrPseudo::~LdrPseudo() {}
void LdrPseudo::EmitCode(std::ostream& outfile) {
  // convert ldr-pseudo inst to movw&movt
  // https://community.arm.com/developer/ip-products/processors/b/processors-ip-blog/posts/how-to-load-constants-in-assembly-for-arm-architecture
  if (!this->is_imm_) {
    outfile << "\tmov32 " << std::string(*(this->rd_)) + ", " << this->literal_ << std::endl;
  } else if (0 == ((this->imm_ >> 16) & 0xFFFF)) {
    outfile << "\tmovw " << std::string(*(this->rd_)) + ", #" << (this->imm_ & 0xFFFF) << std::endl;
  } else {
    outfile << "\tmov32 " << std::string(*(this->rd_)) + ", " << this->imm_ << std::endl;
  }
  // outfile << "\tmov32 " << std::string(*(this->rd_)) << ", "
  //         << (this->is_imm_ ? std::to_string(this->imm_) : this->literal_) << std::endl;
}
void LdrPseudo::Check() {
  MyAssert(nullptr != rd_);
  rd_->Check();
  // NOTE: literal_应该能在全局表中找到 TODO!!!
  // MyAssert(IsImm() || (ir::gScopes[0].symbol_table_.find(literal_) != ir::gScopes[0].symbol_table_.end()));
}

PushPop::~PushPop() {}
void PushPop::EmitCode(std::ostream& outfile) {
  MyAssert(!this->reg_list_.empty());
  std::string opcode = this->opkind_ == OpKind::PUSH ? "push" : "pop";
  outfile << "\t" << opcode << " { " << std::string(*(this->reg_list_[0]));
  for (auto iter = this->reg_list_.begin() + 1; iter != this->reg_list_.end(); ++iter) {
    outfile << ", " << std::string(**iter);
  }
  outfile << " }" << std::endl;
}
void PushPop::Check() {
  MyAssert(!reg_list_.empty());
  for (auto reg : reg_list_) {
    reg->Check();
  }
}

}  // namespace arm

#undef ASSERT_ENABLE
