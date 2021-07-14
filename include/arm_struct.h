// Reference: https://developer.arm.com/documentation/100076/0200
#ifndef __ARM_STRUCT_H__
#define __ARM_STRUCT_H__
#include <unordered_set>

#include "../include/ir_struct.h"

namespace arm {

// ref: https://en.wikipedia.org/wiki/Calling_convention#ARM_(A32)
// ref: https://developer.arm.com/documentation/ihi0042/j/
enum class ArmReg {
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

class Reg {
 public:
  //   ArmReg reg_;
  int reg_id_;
  Reg(int reg_id) : reg_id_(reg_id) {}
  Reg(ArmReg armreg) : reg_id_(static_cast<int>(armreg)) {}
  explicit operator std::string() { return "r" + std::to_string(reg_id_); }
};

class Shift {
 public:
  enum class OpCode {
    // no shift, same as LSL #0
    NONE,
    // arithmetic right
    ASR,
    // logic left
    LSL,
    // logic right
    LSR,
    // rotate right
    ROR,
    // rotate right one bit with extend
    RRX
  };
  OpCode opcode_;
  bool is_imm_;
  int shift_imm_;
  Reg* shift_reg_;

  Shift()
      : opcode_(OpCode::NONE),
        is_imm_(true),
        shift_imm_(0),
        shift_reg_(nullptr) {}
  Shift(OpCode opcode, Reg* shift_reg)
      : opcode_(opcode), is_imm_(false), shift_reg_(shift_reg) {}
  Shift(OpCode opcode, int shift)
      : opcode_(opcode),
        is_imm_(true),
        shift_reg_(nullptr),
        shift_imm_(shift) {}
  bool IsNone() { return opcode_ == OpCode::NONE; }
  explicit operator std::string() {
    std::string opcode = "";
    switch (opcode_) {
      case OpCode::NONE:
        return "";
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
        opcode = "RRX";
        break;
      default:
        break;
    }
    if (is_imm_) {
      return " , " + opcode + " #" + std::to_string(shift_imm_);
    } else {
      return " , " + opcode + " " + std::string(*shift_reg_);
    }
  }
};

// Operand2 is a flexible second operand.
// Operand2 can be a:
// - Constant.
// - Register with optional shift.
class Operand2 {
 public:
  bool is_imm_;
  int imm_num_;
  Reg* reg_;
  Shift* shift_;
  Operand2(int imm_num)
      : is_imm_(true), imm_num_(imm_num), reg_(nullptr), shift_(nullptr) {}
  Operand2(Reg* reg) : is_imm_(false), reg_(reg), shift_(nullptr) {}
  Operand2(Reg* reg, Shift* shift) : is_imm_(false), reg_(reg), shift_(shift) {}
  explicit operator std::string() {
    return is_imm_ ? ("#" + std::to_string(imm_num_))
                   : ("" + std::string(*reg_) +
                      (nullptr == shift_ ? "" : std::string(*shift_)));
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

std::string CondToString(Cond cond);

class Instruction {
 public:
  Cond cond_;
  bool IsAL() { return cond_ == Cond::AL; }
  Instruction(Cond cond) : cond_(cond) {}
  Instruction() : cond_(Cond::AL) {}
  virtual ~Instruction() = default;
  virtual void EmitCode(std::ostream& outfile = std::clog) = 0;
};

// BinaryInstruction: <OpCode>{S}{Cond} {Rd,} Rn, <Operand2>
class BinaryInst : public Instruction {
 public:
  enum class OpCode {
    ADD,   // 减法
    SUB,   // 减法
    RSB,   // 反向减法 working for imm - Rn
    MUL,   // Operand2 can only be Rs.
    SDIV,  // 有符号除
    // MOD,

    // Logical
    AND,  // 与
    EOR,  // 异或
    ORR,  // 或
    RON,  // 或非
    BIC,  // 位清零

    // no Rd. omit S.
    CMP,  // 比较
    CMN,  // 与负数比较
    TST,  // 测试
    TEQ,  // 相等测试

    // convert shift-inst to mov-inst
  };
  OpCode opcode_;
  bool has_s_;  // only mean if opcode has char 'S'. don't mean the
                // instruction whether updates CPSR or not.
  Reg* rd_;     // Note: rd may nullptr
  Reg* rn_;
  Operand2* op2_;

  bool HasS() { return has_s_; }
  BinaryInst(OpCode opcode, bool has_s, Cond cond, Reg* rd, Reg* rn,
             Operand2* op2)
      : Instruction(cond),
        opcode_(opcode),
        has_s_(has_s),
        rd_(rd),
        rn_(rn),
        op2_(op2) {}
  BinaryInst(OpCode opcode, Cond cond, Reg* rn, Operand2* op2)
      : Instruction(cond),
        opcode_(opcode),
        has_s_(false),
        rd_(nullptr),
        rn_(rn),
        op2_(op2) {
    // for TST TEQ CMP CMN: no rd. omit S.
  }
  virtual ~BinaryInst();
  virtual void EmitCode(std::ostream& outfile = std::clog);
};

// Move: MOV{S}{Cond} Rd, <Operand2>
class Move : public Instruction {
 public:
  bool has_s_;  // only mean if opcode has char 'S'. don't mean the
                // instruction whether updates CPSR or not.
  Reg* rd_;
  Operand2* op2_;
  bool HasS() { return has_s_; }
  Move(bool has_s, Cond cond, Reg* rd, Operand2* op2)
      : Instruction(cond), has_s_(has_s), rd_(rd), op2_(op2) {}
  virtual ~Move();
  virtual void EmitCode(std::ostream& outfile = std::clog);
};

// Branch: B{L}{Cond} <label>
class Branch : public Instruction {
 public:
  std::string label_;
  bool has_l_;
  bool has_x_;
  Branch(bool has_l, bool has_x, Cond cond, std::string label)
      : Instruction(cond), has_l_(has_l), has_x_(has_x), label_(label) {}
  virtual ~Branch();
  virtual void EmitCode(std::ostream& outfile = std::clog);
};

// Load:
// Store:
class LdrStr : public Instruction {
 public:
  enum class OpKind { LDR, STR };
  enum class Type { Pre, Norm, Post, PCrel };
  OpKind opkind_;
  Type type_;
  Reg* rd_;
  Reg* rn_;           // Base
  Operand2* offset_;  // can be a imm, a reg, or a scaled reg(imm_shift)
  std::string label_;
  LdrStr(OpKind opkind, Type type, Cond cond, Reg* rd, Reg* rn,
         Operand2* offset)
      : Instruction(cond),
        opkind_(opkind),
        type_(type),
        rd_(rd),
        rn_(rn),
        offset_(offset) {}
  LdrStr(Reg* rd, std::string label)
      : opkind_(OpKind::LDR), type_(Type::PCrel), rd_(rd), label_(label) {}
  virtual ~LdrStr();
  virtual void EmitCode(std::ostream& outfile = std::clog);
};

// Push:
// Pop:
class PushPop : public Instruction {
 public:
  enum class OpKind { PUSH, POP };
  OpKind opkind_;
  std::vector<Reg*> reg_list_;
  PushPop(OpKind opkind, Cond cond) : Instruction(cond), opkind_(opkind) {}
  virtual ~PushPop();
  virtual void EmitCode(std::ostream& outfile = std::clog);
};

class BasicBlock {
 public:
  // all instructions
  // int start_;  // close
  // int end_;    // open
  std::string* label_;
  std::vector<Instruction*> inst_list_;
  // pred succ
  std::vector<BasicBlock*> pred_;
  std::vector<BasicBlock*> succ_;
  // used for reg alloc
  std::unordered_set<int> def_;  // 基本块的def 可看作是def中去掉liveuse
  std::unordered_set<int> liveuse_;  // 基本块的use 可看作是活跃的use
  std::unordered_set<int> livein_;
  std::unordered_set<int> liveout_;

  // in out data stream
  BasicBlock() : label_(nullptr) {}
  BasicBlock(std::string* label) : label_(label) {}
  // BasicBlock(int start) : start_(start), end_(-1) {}
  //   void Print();
  bool HasLabel() { return nullptr != label_; }
  void EmitCode(std::ostream& outfile = std::clog);
};

class Function {
 public:
  // all BasicBlocks
  std::string func_name_;
  std::vector<BasicBlock*> bb_list_;
  int stack_size_;
  int arg_num_;
  int virtual_max = 0;

  // optional
  // Function*
  std::vector<Function*> call_func_list_;
  Function(std::string func_name, int arg_num, int stack_size)
      : func_name_(func_name), arg_num_(arg_num), stack_size_(stack_size) {}
  //   void Print();
  bool IsLeaf() { return call_func_list_.empty(); }
  void EmitCode(std::ostream& outfile = std::clog);
};

class Module {
 public:
  // global declaration
  ir::Scope& global_scope_;
  // all functions
  std::vector<Function*> func_list_;
  Module(ir::Scope& global_scope) : global_scope_(global_scope) {}
  //   void Print();
  void EmitCode(std::ostream& outfile = std::clog);
};

Module* GenerateAsm(ir::Module* module);

// extern std::vector<Instruction> gASMList;
}  // namespace arm
#endif