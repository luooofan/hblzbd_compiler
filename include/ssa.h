#ifndef __SSA_H__
#define __SSA_H__
#include <iostream>
#include <list>
#include <string>
#include <vector>
class SSAModule;
class SSAFunction;
class SSABasicBlock;
class Type;
class User;
class Use;

class Value {
 private:
  Type* type_;
  std::list<Use*> use_list_;  // NOTE Use* or Use??
  std::string name_;          // readable
 public:
  Value(Type* type, const std::string& name = "") : type_(type), name_(name) {}
  Type* GetType() const { return type_; }
  const std::string& GetName() const { return name_; }
  // use_list_目前只能被use对象操作
  void AddUse(Use* use) { use_list_.push_back(use); }
  void KillUse(Use* use) { use_list_.remove(use); }
  // TODO replaceAllUseWith
  virtual ~Value() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

// a relationship
class Use {
 private:
  Value* val_;
  User* user_;

 public:
  Value* Get() const { return val_; }
  User* GetUser() const { return user_; }
  Use(Value* val, User* user) : val_(val), user_(user) {
    if (nullptr != val_) val_->AddUse(this);
  }
  ~Use() {
    if (nullptr != val_) val_->KillUse(this);
  }
  void Set(Value* val) {
    if (nullptr != val_) val_->KillUse(this);
    val_ = val;
    if (nullptr != val_) val_->AddUse(this);
  }
  // TODO 把拷贝构造和复制构造实现为调用Set
};

// We don't need the type system acutally.
class Type {
 public:
  enum TypeID {
    VoidTyID,
    LabelTyID,
    IntegerTyID,  // only can be i32
    FunctionTyID,
    ArrayTyID,
    PointerTyID,
  };
  TypeID tid_;
  Type(TypeID tid) : tid_(tid) {}
  virtual ~Type() = default;
  virtual bool IsPointer() const { return tid_ == TypeID::PointerTyID; }
  // TODO equal
};

class FunctionType : public Type {
 public:
  FunctionType(Type* ret_type, std::vector<Type*> args_type)
      : Type(TypeID::FunctionTyID), ret_type_(ret_type) {  // TODO
    args_type.swap(args_type_);
  };
  Type* ret_type_;
  std::vector<Type*> args_type_;
  virtual ~FunctionType() {}
};

/*
// Only can be one dimensional array in quad-ir. just like a i32 pointer.
// We need array-type and alloca-inst to contain stack space info.
class ArrayType : public Type {
 public:
  Type* contained_;  // only can be i32
  unsigned num_elements_;
  ArrayType(unsigned num_elements)
      : Type(TypeID::ArrayTyID), num_elements_(num_elements), contained_(new Type(Type::IntegerTyID)) {}
  virtual ~ArrayType() {}
};
*/
class PointerType : public Type {
 public:
  Type* referenced_;
  PointerType(Type* referenced) : Type(TypeID::PointerTyID), referenced_(referenced) {}
  virtual ~PointerType() {}
  virtual bool IsPointer() const override { return true; }
};

class ConstantInt : public Value {
 public:
  const int imm_;
  ConstantInt(const int imm) : Value(new Type(Type::IntegerTyID)), imm_(imm) {}
  virtual ~ConstantInt() {}
  virtual void Print(std::ostream& outfile = std::clog) override;
};

class GlobalVariable : public Value {
  GlobalVariable(Type* type, const std::string& name, int size) : Value(type, name), size_(size) {}

 public:
  std::vector<int> val_;
  int size_;
  // type must be i32 pointer
  GlobalVariable(const std::string& name, int size)
      : Value(new PointerType(new Type(Type::IntegerTyID)), name), size_(size) {}
  virtual ~GlobalVariable() {}
  virtual void Print(std::ostream& outfile = std::clog) override;
};

class UndefVariable : public Value {
 public:
  UndefVariable(Type* type, const std::string& name) : Value(type, name) {}
  virtual ~UndefVariable() {}
  virtual void Print(std::ostream& outfile = std::clog) override;
};

class Argument : public Value {
 public:
  unsigned arg_no_;
  Argument(Type* type, const std::string& name) : Value(type, name) {}
  virtual void Print(std::ostream& outfile = std::clog) override;
};

class FunctionValue : public Value {
 public:
  // std::vector<>
  // TODO 参数
  FunctionValue(FunctionType* type, const std::string& name, SSAFunction* func);
  virtual ~FunctionValue() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class BasicBlockValue : public Value {
 public:
  BasicBlockValue(const std::string& name, SSABasicBlock* bb);
  virtual ~BasicBlockValue() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class User : public Value {
 public:
  std::vector<Use> operands_;  // All Use instances are constructed in User
  User(Type* type, const std::string& name) : Value(type, name) {}
  Value* GetOperand(unsigned i) {
    // assert
    return operands_[i].Get();
  }
  unsigned GetNumOperands() const { return operands_.size(); }
  virtual ~User() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class SSAInstruction : public User {
 public:
  SSABasicBlock* parent_;
  SSAInstruction(Type* type, const std::string& name, SSABasicBlock* parent);
  void SetParent(SSABasicBlock* parent) { parent_ = parent; }
  virtual ~SSAInstruction() {}
  virtual void Print(std::ostream& outfile = std::clog) = 0;
};

class BinaryOperator : public SSAInstruction {
 public:
  enum OpKind {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
  };
  OpKind op_;
  BinaryOperator(Type* type, OpKind op, const std::string name, Value* lhs, Value* rhs, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent), op_(op) {
    operands_.push_back(Use(lhs, this));
    operands_.push_back(Use(rhs, this));
  };
  virtual ~BinaryOperator() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class UnaryOperator : public SSAInstruction {
 public:
  enum OpKind {
    NEG,
    NOT,
  };
  OpKind op_;
  UnaryOperator(Type* type, OpKind op, const std::string name, Value* lhs, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent), op_(op) {
    operands_.push_back(Use(lhs, this));
  }
  virtual ~UnaryOperator() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class BranchInst : public SSAInstruction {
 public:
  enum Cond {
    AL,
    EQ,
    NE,
    GT,
    GE,
    LT,
    LE,
  };
  Cond cond_;
  BranchInst(BasicBlockValue* target, SSABasicBlock* parent)
      : SSAInstruction(new Type(Type::VoidTyID), "", parent), cond_(Cond::AL) {
    operands_.push_back(Use(target, this));
  }
  BranchInst(Cond cond, Value* lhs, Value* rhs, BasicBlockValue* target, SSABasicBlock* parent)
      : SSAInstruction(new Type(Type::VoidTyID), "", parent), cond_(cond) {
    operands_.push_back(Use(lhs, this));
    operands_.push_back(Use(rhs, this));
    operands_.push_back(Use(target, this));
  }
  bool HasCond() const { return cond_ != Cond::AL; }
  virtual ~BranchInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class CallInst : public SSAInstruction {
 public:
  CallInst(FunctionValue* func, SSABasicBlock* parent) : SSAInstruction(new Type(Type::VoidTyID), "", parent) {
    operands_.push_back(Use(func, this));
  }
  CallInst(Type* type, const std::string& name, FunctionValue* func, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent) {
    operands_.push_back(Use(func, this));
  }
  void AddArg(Value* arg) { operands_.push_back(Use(arg, this)); }
  virtual ~CallInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class ReturnInst : public SSAInstruction {
 public:
  ReturnInst(Value* v, SSABasicBlock* parent) : SSAInstruction(new Type(Type::VoidTyID), "", parent) {
    operands_.push_back(Use(v, this));
  }
  ReturnInst(SSABasicBlock* parent) : SSAInstruction(new Type(Type::VoidTyID), "", parent) {}
  bool IsVoidReturn() const { return GetNumOperands() == 0; }
  virtual ~ReturnInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

// Alloca-inst has only a constint operand which indicates the stack space would be allocated in runtime
// and will not be used by other users.
class AllocaInst : public SSAInstruction {
  AllocaInst(Type* type, SSABasicBlock* parent) : SSAInstruction(type, "", parent) {}

 public:
  AllocaInst(Value* val, SSABasicBlock* parent) : SSAInstruction(new Type(Type::VoidTyID), "", parent) {
    operands_.push_back(Use(val, this));
  }
  virtual ~AllocaInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class LoadInst : public SSAInstruction {
 public:
  LoadInst(Type* type, const std::string& name, Value* ptr, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent) {
    operands_.push_back(Use(ptr, this));
  }
  LoadInst(Type* type, const std::string& name, Value* ptr, Value* offset, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent) {
    operands_.push_back(Use(ptr, this));
    operands_.push_back(Use(offset, this));
  }
  virtual ~LoadInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class StoreInst : public SSAInstruction {
  StoreInst(Type* type, const std::string& name, Value* ptr, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent) {
    operands_.push_back(Use(ptr, this));
  }
  StoreInst(Type* type, const std::string& name, Value* ptr, Value* offset, SSABasicBlock* parent)
      : SSAInstruction(type, name, parent) {
    operands_.push_back(Use(ptr, this));
    operands_.push_back(Use(offset, this));
  }

 public:
  StoreInst(Value* val, Value* ptr, SSABasicBlock* parent) : SSAInstruction(new Type(Type::VoidTyID), "", parent) {
    operands_.push_back(Use(val, this));
    operands_.push_back(Use(ptr, this));
  }
  StoreInst(Value* val, Value* ptr, Value* offset, SSABasicBlock* parent)
      : SSAInstruction(new Type(Type::VoidTyID), "", parent) {
    operands_.push_back(Use(val, this));
    operands_.push_back(Use(ptr, this));
    operands_.push_back(Use(offset, this));
  }
  virtual ~StoreInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class MovInst : public SSAInstruction {
 public:
  MovInst(Type* type, const std::string& name, Value* v, SSABasicBlock* parent) : SSAInstruction(type, name, parent) {
    operands_.push_back(Use(v, this));
  }
  virtual ~MovInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

class PhiInst : public SSAInstruction {
 public:
  PhiInst(Type* type, const std::string& name, SSABasicBlock* parent) : SSAInstruction(type, name, parent) {}
  void AddParam(Value* v, BasicBlockValue* bb) {
    operands_.push_back(Use(v, this));
    operands_.push_back(Use(bb, this));
  }
  virtual ~PhiInst() {}
  virtual void Print(std::ostream& outfile = std::clog);
};

#endif