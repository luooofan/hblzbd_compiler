#ifndef __ARM_H__
#define __ARM_H__
#include <iostream>
#include <unordered_set>
#include <vector>
class ArmBasicBlock;

namespace arm {
// ref: https://en.wikipedia.org/wiki/Calling_convention#ARM_(A32)
// ref: https://developer.arm.com/documentation/ihi0042/j/
// ref: https://developer.arm.com/documentation/ddi0406/latest

enum ArmReg {
  // args and return value (caller saved)
  r0,
  r1,
  r2,
  r3,
  // local variables (callee saved)
  r4,
  r5,
  r6,
  r7,
  r8,
  r9,
  r10,
  r11,
  // special purposes
  r12,
  r13,
  r14,
  r15,
  // some aliases
  fp = r11,  // frame pointer (omitted), allocatable
  ip = r12,  // ipc scratch register, used in some instructions (caller saved)
  sp = r13,  // stack pointer
  lr = r14,  // link register (caller saved)
  pc = r15,  // program counter
};

class Instruction;

class Reg {
 private:
  std::unordered_set<Instruction*> used_inst_set;

 public:
  //   ArmReg reg_;
  int reg_id_;
  Reg(int reg_id) : reg_id_(reg_id) {}
  Reg(ArmReg armreg) : reg_id_(static_cast<int>(armreg)) {}
  explicit operator std::string() { return "r" + std::to_string(reg_id_); }
  // NOTE: only used for final check.
  void AddUsedInst(Instruction* inst) { used_inst_set.insert(inst); }
  const std::unordered_set<Instruction*>& GetUsedInsts() const { return used_inst_set; }
  const unsigned GetUsedInstNum() const { return used_inst_set.size(); }
};

class Shift {
 public:
  enum class OpCode {
    ASR,  // arithmetic right
    LSL,  // logic left
    LSR,  // logic right
    ROR,  // rotate right
    RRX   // rotate right one bit with extend
  };
  OpCode opcode_ = OpCode::LSL;
  bool is_imm_ = true;
  int shift_imm_ = 0;
  Reg* shift_reg_ = nullptr;

  Shift() {}
  Shift(OpCode opcode, Reg* shift_reg) : opcode_(opcode), is_imm_(false), shift_reg_(shift_reg) {}
  Shift(OpCode opcode, int shift) : opcode_(opcode), shift_imm_(shift) {}
  bool IsNone() { return opcode_ == OpCode::LSL && is_imm_ && 0 == shift_imm_; }
  explicit operator std::string();
  void AddUsedInst(Instruction* inst) {
    if (!is_imm_) shift_reg_->AddUsedInst(inst);
  }
};

// Operand2 is a flexible second operand.
// Operand2 can be a:
// - Constant. <imm8m>
// - Register with optional shift. Reg {, <opsh>} or Reg, LSL/LSR/ASR/ROR Rs
class Operand2 {
 public:
  bool is_imm_ = true;
  int imm_num_ = 0;
  Reg* reg_ = nullptr;
  Shift* shift_ = nullptr;
  Operand2(int imm_num) : imm_num_(imm_num) {}
  Operand2(Reg* reg) : is_imm_(false), reg_(reg) {}
  Operand2(Reg* reg, Shift* shift) : is_imm_(false), reg_(reg), shift_(shift) {}
  explicit operator std::string();
  static bool CheckImm8m(int imm);
  bool HasShift() { return /*!is_imm_ &&*/ nullptr != shift_; }
  bool HasUsedAsOp2(Reg* reg) { return reg_ == reg; }
  bool HasUsedAsOp2WithoutShift(Reg* reg) { return !HasShift() && HasUsedAsOp2(reg); }
  void AddUsedInst(Instruction* inst) {
    if (!is_imm_) reg_->AddUsedInst(inst);
    if (nullptr != shift_) shift_->AddUsedInst(inst);
  }
};

enum class Cond {
  AL,  // 无条件执行,通常省略
  EQ,
  NE,
  GT,
  GE,
  LT,
  LE,
};

Cond GetOppositeCond(Cond cond);
std::string CondToString(Cond cond);

class Instruction {
 public:
  ArmBasicBlock* parent_;
  Cond cond_ = Cond::AL;
  bool IsAL() { return cond_ == Cond::AL; }
  Instruction(Cond cond, ArmBasicBlock* parent, bool push_back = true);
  Instruction(ArmBasicBlock* parent, bool push_back = true);
  Instruction(Instruction* inst);
  virtual ~Instruction() = default;
  virtual void EmitCode(std::ostream& outfile = std::clog) = 0;
  virtual void AddUsedInst(){};

  // 这几个函数其实只实际作用于Binaryinst Move和LdrStr语句 为了方便才提升到Instruction中
  virtual bool HasUsedAsOp2(Reg* reg) { return false; }
  virtual bool HasUsedAsOp2WithoutShift(Reg* reg) { return false; }
  virtual void ReplaceOp2With(Operand2* op2) {}
};

// BinaryInstruction: <OpCode>{S}{Cond} {Rd,} Rn, <Operand2>
class BinaryInst : public Instruction {
 public:
  enum class OpCode {
    ADD,  // 减法
    SUB,  // 减法
    RSB,  // 反向减法 working for imm - Rn
    MUL,  // Operand2 can only be Rs.

    // Logical
    AND,  // 与
    EOR,  // 异或
    ORR,  // 或
    RON,  // 或非
    BIC,  // 位清零

    SDIV,  // 有符号除法
    // UDIV,  // 无符号除法

    // no Rd. omit S.
    CMP,  // 比较
    CMN,  // 与负数比较
    TST,  // 测试
    TEQ,  // 相等测试

    // convert shift-inst to mov-inst
  };
  OpCode opcode_;
  bool has_s_ = false;  // only mean if opcode has char 'S'. don't mean the instruction whether updates CPSR or not.
                        // for CMP, CMN, TST, TEQ
  Reg* rd_ = nullptr;   // Note: rd may nullptr
  Reg* rn_;
  Operand2* op2_;

  BinaryInst(OpCode opcode, bool has_s, Cond cond, Reg* rd, Reg* rn, Operand2* op2, ArmBasicBlock* parent)
      : Instruction(cond, parent), opcode_(opcode), has_s_(has_s), rd_(rd), rn_(rn), op2_(op2) {
    AddUsedInst();
  }
  BinaryInst(OpCode opcode, Reg* rd, Reg* rn, Operand2* op2, ArmBasicBlock* parent)
      : Instruction(parent), opcode_(opcode), rd_(rd), rn_(rn), op2_(op2) {
    AddUsedInst();
  }

  // for TST TEQ CMP CMN: no rd. omit S.
  BinaryInst(OpCode opcode, Cond cond, Reg* rn, Operand2* op2, ArmBasicBlock* parent)
      : Instruction(cond, parent), opcode_(opcode), rn_(rn), op2_(op2) {
    AddUsedInst();
  }
  BinaryInst(OpCode opcode, Reg* rn, Operand2* op2, ArmBasicBlock* parent)
      : Instruction(parent), opcode_(opcode), rn_(rn), op2_(op2) {
    AddUsedInst();
  }

  virtual ~BinaryInst();

  bool HasS() { return has_s_; }
  virtual void EmitCode(std::ostream& outfile = std::clog) override;
  virtual void AddUsedInst() override { rn_->AddUsedInst(this), op2_->AddUsedInst(this); }
  virtual bool HasUsedAsOp2(Reg* reg) override { return op2_->HasUsedAsOp2(reg); }
  virtual bool HasUsedAsOp2WithoutShift(Reg* reg) override { return op2_->HasUsedAsOp2WithoutShift(reg); }
  virtual void ReplaceOp2With(Operand2* op2) override { op2_ = op2; }
};

// Move: MOV{S}{Cond} Rd, <Operand2>
class Move : public Instruction {
 public:
  bool is_mvn_;
  bool has_s_ = false;
  Reg* rd_;
  Operand2* op2_;

  Move(bool has_s, Cond cond, Reg* rd, Operand2* op2, ArmBasicBlock* parent, bool is_mvn = false, bool push_back = true)
      : Instruction(cond, parent, push_back), has_s_(has_s), rd_(rd), op2_(op2), is_mvn_(is_mvn) {
    AddUsedInst();
  }
  Move(Reg* rd, Operand2* op2, ArmBasicBlock* parent, bool is_mvn = false, bool push_back = true)
      : Instruction(parent, push_back), rd_(rd), op2_(op2), is_mvn_(is_mvn) {
    AddUsedInst();
  }
  Move(Reg* rd, Operand2* op2, Instruction* inst, bool is_mvn = false)
      : Instruction(inst), rd_(rd), op2_(op2), is_mvn_(is_mvn) {
    AddUsedInst();
  }
  virtual ~Move();

  bool HasS() { return has_s_; }
  virtual void EmitCode(std::ostream& outfile = std::clog) override;
  virtual void AddUsedInst() override { op2_->AddUsedInst(this); }
  virtual bool HasUsedAsOp2(Reg* reg) override { return op2_->HasUsedAsOp2(reg); }
  virtual bool HasUsedAsOp2WithoutShift(Reg* reg) override { return op2_->HasUsedAsOp2WithoutShift(reg); }
  virtual void ReplaceOp2With(Operand2* op2) override { op2_ = op2; }
};

// Branch: B{L}{Cond} <label> label can be "lr" or a func name or a normal label beginning with a dot.
class Branch : public Instruction {
 public:
  std::string label_;
  bool has_l_;
  bool has_x_;
  bool IsCall();
  bool IsRet();
  Branch(bool has_l, bool has_x, Cond cond, std::string label, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(cond, parent, push_back), has_l_(has_l), has_x_(has_x), label_(label) {}
  Branch(bool has_l, bool has_x, std::string label, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(parent, push_back), has_l_(has_l), has_x_(has_x), label_(label) {}
  virtual ~Branch();
  virtual void EmitCode(std::ostream& outfile = std::clog) override;
};

// LoadStore: <op>/*{size}*/ rd, rn {, #<imm12>} OR rd , rn, +/- rm {, <opsh>}, i.e. a Operand2-style offset.
class LdrStr : public Instruction {
 public:
  enum class OpKind { LDR, STR };
  enum class Type { Pre, Norm, Post };
  OpKind opkind_;
  Type type_ = Type::Norm;
  Reg* rd_;
  Reg* rn_;  // Base
  bool is_offset_imm_ = false;
  int offset_imm_ = -1;
  Operand2* offset_ = nullptr;  // a reg, or a scaled reg(imm_shift)

  LdrStr(OpKind opkind, Type type, Cond cond, Reg* rd, Reg* rn, Operand2* offset, ArmBasicBlock* parent,
         bool push_back = true)
      : Instruction(cond, parent, push_back), opkind_(opkind), type_(type), rd_(rd), rn_(rn), offset_(offset) {
    AddUsedInst();
  }
  LdrStr(OpKind opkind, Reg* rd, Reg* rn, Operand2* offset, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(parent, push_back), opkind_(opkind), rd_(rd), rn_(rn), offset_(offset) {
    AddUsedInst();
  }
  LdrStr(OpKind opkind, Type type, Cond cond, Reg* rd, Reg* rn, int offset, ArmBasicBlock* parent,
         bool push_back = true)
      : Instruction(cond, parent, push_back),
        opkind_(opkind),
        type_(type),
        rd_(rd),
        rn_(rn),
        is_offset_imm_(true),
        offset_imm_(offset) {
    AddUsedInst();
  }
  LdrStr(OpKind opkind, Reg* rd, Reg* rn, int offset, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(parent, push_back), opkind_(opkind), rd_(rd), rn_(rn), is_offset_imm_(true), offset_imm_(offset) {
    AddUsedInst();
  }
  virtual ~LdrStr();

  virtual void EmitCode(std::ostream& outfile = std::clog) override;
  static bool CheckImm12(int imm) { return (imm < 4096) && (imm > -4096); }

  virtual void AddUsedInst() override {
    if (opkind_ == OpKind::STR) rd_->AddUsedInst(this);
    rn_->AddUsedInst(this);
    if (nullptr != offset_) offset_->AddUsedInst(this);
  }
  virtual bool HasUsedAsOp2(Reg* reg) override {
    if (nullptr != offset_)
      return offset_->HasUsedAsOp2(reg);
    else
      return false;
  }
  virtual bool HasUsedAsOp2WithoutShift(Reg* reg) override {
    if (nullptr != offset_)
      return offset_->HasUsedAsOp2WithoutShift(reg);
    else
      return false;
  }
  virtual void ReplaceOp2With(Operand2* op2) override {
    if (nullptr != offset_) offset_ = op2;
  }
};

// ldr-pseudo inst: ref: https://developer.arm.com/documentation/dui0041/c/Babbfdih
// when emit code, it will be converted to movw and movt insts.
class LdrPseudo : public Instruction {
 public:
  Reg* rd_;
  std::string literal_;
  int imm_;
  LdrPseudo(Cond cond, Reg* rd, const std::string& literal, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(cond, parent, push_back), rd_(rd), is_imm_(false), literal_(literal) {}
  LdrPseudo(Reg* rd, const std::string& literal, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(parent, push_back), rd_(rd), is_imm_(false), literal_(literal) {}
  LdrPseudo(Cond cond, Reg* rd, int imm, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(cond, parent, push_back), rd_(rd), is_imm_(true), imm_(imm) {}
  LdrPseudo(Reg* rd, int imm, ArmBasicBlock* parent, bool push_back = true)
      : Instruction(parent, push_back), rd_(rd), is_imm_(true), imm_(imm) {}

  virtual ~LdrPseudo();

  bool IsImm() { return is_imm_; }
  virtual void EmitCode(std::ostream& outfile = std::clog) override;

 private:
  bool is_imm_;
};

// PushPop: <op> <reglist>
class PushPop : public Instruction {
 public:
  enum class OpKind { PUSH, POP };
  OpKind opkind_;
  std::vector<Reg*> reg_list_;
  PushPop(OpKind opkind, Cond cond, ArmBasicBlock* parent) : Instruction(cond, parent), opkind_(opkind) {}
  PushPop(OpKind opkind, ArmBasicBlock* parent) : Instruction(parent), opkind_(opkind) {}
  virtual ~PushPop();
  virtual void EmitCode(std::ostream& outfile = std::clog) override;
};

}  // namespace arm

#endif