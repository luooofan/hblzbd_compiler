#include "../include/arm_struct.h"

#include <cassert>
#include <iostream>
#include <string>
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

Module* GenerateAsm(ir::Module* module) {
  // 由ir module直接构建arm的bb func 和 module
  // 构建的过程中可以把ir中的label语句删掉
  // 可能会出现一些空的bb
  Module* armmodule = new Module(module->global_scope_);
  std::unordered_map<ir::Function*, Function*> func_map;

  for (auto func : module->func_list_) {
    // every ir func
    // TODO:
    // 需要在第一个基本块 维护栈 保护寄存器
    // 需要在最后一个基本块 维护栈 恢复寄存器
    // 但是此时并不明确到底该如何做
    Function* armfunc = new Function(func->func_name_);
    func_map.insert({func, armfunc});
    armmodule->func_list_.push_back(armfunc);

    // create armbb according to irbb
    std::unordered_map<ir::BasicBlock*, BasicBlock*> bb_map;
    for (auto bb : func->bb_list_) {
      BasicBlock* armbb = new BasicBlock();
      bb_map.insert({bb, armbb});
      armfunc->bb_list_.push_back(armbb);
    }
    // maintain pred and succ
    for (auto bb : func->bb_list_) {
      auto armbb = bb_map[bb];
      for (auto pred : bb->pred_) {
        armbb->pred_.push_back(bb_map[pred]);
      }
      for (auto succ : bb->succ_) {
        armbb->pred_.push_back(bb_map[succ]);
      }
    }

    std::unordered_map<std::string, std::unordered_map<int, Reg*>>
        var_map;  // varname, scope_id, reg

    for (auto bb : func->bb_list_) {
      // every ir basicblock
      auto armbb = bb_map[bb];
      // 不断往armbb里加arm指令

      // 如果第一条ir是label label 转成armbb的label
      // 过滤所有label语句
      // 假设bb的start一定有效
      int start = bb->start_;
      if (ir::gIRList[start].op_ == ir::IR::OpKind::LABEL) {
        auto& opn1 = ir::gIRList[start].opn1_;
        if (opn1.type_ == ir::Opn::Type::Label) {
          armbb->label_ = new std::string(opn1.name_);
        }
        ++start;
      }

      auto dbg_print_var_map = [&var_map]() {
        std::clog << "DBG_PRINT_VAR_MAP BEGIN." << std::endl;
        for (auto& outer : var_map) {
          std::clog << "varname: " << outer.first << std::endl;
          for (auto& inner : outer.second) {
            std::clog << "\tscope_id: " << inner.first << " "
                      << std::string(*(inner.second)) << std::endl;
          }
        }
        std::clog << "DBG_PRINT_VAR_MAP END." << std::endl;
      };

      int virtual_reg_id = 16;
      auto new_virtual_reg = [&virtual_reg_id]() {
        return new Reg(virtual_reg_id++);
      };

      auto resolve_opn2operand2 = [&var_map, &new_virtual_reg](ir::Opn* Opn) {
        // maybe a imm or a var
        if (Opn->type_ == ir::Opn::Type::Imm) {
          return new Operand2(Opn->imm_num_);
        } else if (Opn->type_ == ir::Opn::Type::Array) {
          // Array 找到基址 返回一个op2
          assert(0);
        } else {
          // a var without shift
          Reg* vreg = nullptr;
          auto iter = var_map.find(Opn->name_);
          if (iter != var_map.end()) {
            auto iter2 = (*iter).second.find(Opn->scope_id_);
            if (iter2 != (*iter).second.end()) {
              vreg = (*iter2).second;
              return new Operand2(vreg);
            }
          }
          vreg = new_virtual_reg();
          var_map[Opn->name_][Opn->scope_id_] = vreg;
          return new Operand2(vreg);
        }
      };

      auto resolve_opn2reg = [&armbb, &var_map, &new_virtual_reg,
                              &resolve_opn2operand2](ir::Opn* Opn) {
        if (Opn->type_ == ir::Opn::Type::Imm) {
          Reg* vreg = new_virtual_reg();
          // 如果是一个立即数的话就先生成一条mov指令
          // mov rd(vreg) op2
          auto rd = resolve_opn2operand2(Opn);
          armbb->inst_list_.push_back(
              static_cast<Instruction*>(new Move(false, Cond::AL, vreg, rd)));
          return vreg;
        } else if (Opn->type_ == ir::Opn::Type::Array) {
          // Array
          // 先把基址放到rx里(或者直接找到rx) 然后用LDR指令把值取到ry里 返回ry
          assert(0);
        } else {
          // Var
          auto iter = var_map.find(Opn->name_);
          if (iter != var_map.end()) {
            auto iter2 = (*iter).second.find(Opn->scope_id_);
            if (iter2 != (*iter).second.end()) {
              return (*iter2).second;
            }
          }
          Reg* vreg = new_virtual_reg();
          var_map[Opn->name_][Opn->scope_id_] = vreg;
          return vreg;
        }
      };

      auto get_cond_type = [](ir::IR::OpKind opkind, bool exchange = false) {
        if (exchange) {
          switch (opkind) {
            case ir::IR::OpKind::JEQ:
              return Cond::EQ;
            case ir::IR::OpKind::JNE:
              return Cond::NE;
            case ir::IR::OpKind::JLT:
              return Cond::GT;
            case ir::IR::OpKind::JLE:
              return Cond::GE;
            case ir::IR::OpKind::JGT:
              return Cond::LT;
            case ir::IR::OpKind::JGE:
              return Cond::LE;
            default:
              return Cond::AL;
          }
        } else {
          switch (opkind) {
            case ir::IR::OpKind::JEQ:
              return Cond::EQ;
            case ir::IR::OpKind::JNE:
              return Cond::NE;
            case ir::IR::OpKind::JLT:
              return Cond::LT;
            case ir::IR::OpKind::JLE:
              return Cond::LE;
            case ir::IR::OpKind::JGT:
              return Cond::GT;
            case ir::IR::OpKind::JGE:
              return Cond::GE;
            default:
              return Cond::AL;
          }
        }
        return Cond::AL;
      };

      // valid ir: gIRList[start,end_)
      while (start < bb->end_) {
        // every ir
        // 对于每条有原始变量(非中间变量)的四元式
        // 为了保证正确性 必须生成对使用变量的ldr指令
        // 因为无法确认程序执行流 无法确认当前使用的变量的上一个定值点
        // 也就无法确认到底在哪个寄存器中 所以需要ldr到一个新的虚拟寄存器中
        // 为了保证逻辑与存储的一致性 必须生成对定值变量的str指令
        // 寄存器分配期间也无法删除这些ldr和str指令
        // 这些虚拟寄存器对应的活跃区间也非常短
        auto& ir = ir::gIRList[start++];
        switch (ir.op_) {
          case ir::IR::OpKind::ADD: {
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            if (ir.opn1_.type_ == ir::Opn::Type::Imm) {
              // exchange order: opn1->op2 opn2->rn res->rd
              rn = resolve_opn2reg(&(ir.opn2_));
              op2 = resolve_opn2operand2(&(ir.opn1_));
            } else {
              // opn1->rn opn2->op2 res->rd
              rn = resolve_opn2reg(&(ir.opn1_));
              op2 = resolve_opn2operand2(&(ir.opn2_));
            }
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(
                    BinaryInst::OpCode::ADD, false, Cond::AL, rd, rn, op2)));
            break;
          }
          case ir::IR::OpKind::SUB: {
            BinaryInst::OpCode opcode;
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            if (ir.opn1_.type_ == ir::Opn::Type::Imm) {
              // exchange order: opn1->op2 opn2->rn res->rd
              opcode = BinaryInst::OpCode::RSB;
              rn = resolve_opn2reg(&(ir.opn2_));
              op2 = resolve_opn2operand2(&(ir.opn1_));
            } else {
              // opn1->rn opn2->op2 res->rd
              opcode = BinaryInst::OpCode::SUB;
              rn = resolve_opn2reg(&(ir.opn1_));
              op2 = resolve_opn2operand2(&(ir.opn2_));
            }
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new BinaryInst(opcode, false, Cond::AL, rd, rn, op2)));
            break;
          }
          case ir::IR::OpKind::MUL: {
            // if has imm, add mov inst
            auto rm = resolve_opn2reg(&(ir.opn1_));
            auto rs = resolve_opn2reg(&(ir.opn2_));
            auto op2 = new Operand2(rs);
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(
                    BinaryInst::OpCode::MUL, false, Cond::AL, rd, rm, op2)));
            break;
          }
          case ir::IR::OpKind::DIV: {
            // if has imm, add mov inst
            // may can use __aeabi_idiv. r0
            auto rm = resolve_opn2reg(&(ir.opn1_));
            auto rs = resolve_opn2reg(&(ir.opn2_));
            auto op2 = new Operand2(rs);
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(
                    BinaryInst::OpCode::SDIV, false, Cond::AL, rd, rm, op2)));
            break;
          }
          case ir::IR::OpKind::MOD: {
            // may can use __aeabi_idivmod. r1
            assert(0);
            break;
          }
          case ir::IR::OpKind::NOT: {
            // cmp rn 0
            // moveq rd 1
            // movne rd 0
            auto rn = resolve_opn2reg(&(ir.opn1_));
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(
                    BinaryInst::OpCode::CMP, Cond::AL, rn, new Operand2(0))));
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Move(false, Cond::EQ, rd, new Operand2(1))));
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Move(false, Cond::NE, rd, new Operand2(0))));
            break;
          }
          case ir::IR::OpKind::NEG: {
            // impl by RSB res, opn1, #0
            auto rn = resolve_opn2reg(&(ir.opn1_));
            auto op2 = new Operand2(0);
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(
                    BinaryInst::OpCode::RSB, false, Cond::AL, rd, rn, op2)));
            break;
          }
          case ir::IR::OpKind::LABEL: {
            assert(0);
            // 所有label语句都被跳过
            break;
          }
          case ir::IR::OpKind::PARAM: {
            // assert(1);
            break;
          }
          case ir::IR::OpKind::CALL: {
            assert(ir.opn1_.type_ == ir::Opn::Type::Func);
            // TODO:处理param语句
            // ir[start-1] is call
            // NOTE 需要确保IR中函数调用表现为一系列PARAM+一条CALL
            int param_start = start - 1 - ir.opn2_.imm_num_;
            for (int i = 0; i < ir.opn2_.imm_num_; ++i) {
              auto& param_ir = ir::gIRList[param_start + i];
              if (i < 4) {
                // r0-r3. MOV rx, op2
                auto op2 = resolve_opn2operand2(&(param_ir.opn1_));
                auto rd = new Reg(static_cast<ArmReg>(i));
                armbb->inst_list_.push_back(static_cast<Instruction*>(
                    new Move(false, Cond::AL, rd, op2)));
              } else {
                // push
                assert(0);
              }
            }
            // BL label
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Branch(true, false, Cond::AL, ir.opn1_.name_)));
            if (ir.res_.type_ != ir::Opn::Type::Null) {
              // auto rd = resolve_opn2reg(&(ir.res_));
              // auto op2 = new Operand2(new Reg(ArmReg::r0));
              // armbb->inst_list_.push_back(static_cast<Instruction*>(
              //     new Move(false, Cond::AL, rd, op2)));
              var_map[ir.res_.name_][ir.res_.scope_id_] = new Reg(ArmReg::r0);
              // dbg_print_var_map();
            }
            break;
          }
          case ir::IR::OpKind::RET: {
            // opn1 type: 4 chances.
            //  null: B lr;
            //  imm : mov r0, #imm; B lr;
            //  var with vreg: mov r0, vreg; B lr;
            //  var without vreg: 未初始化即使用 mov r0, new vreg; B lr;
            // TODO: how to deal with var without vreg?
            if (ir.opn1_.type_ != ir::Opn::Type::Null) {
              auto rd = new Reg(ArmReg::r0);
              auto op2 = resolve_opn2operand2(&(ir.opn1_));
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new Move(false, Cond::AL, rd, op2)));
            }
            // TODO: B reg;
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Branch(false, true, Cond::AL, "lr")));
            break;
          }
          case ir::IR::OpKind::GOTO: {
            // B label
            assert(ir.opn1_.type_ == ir::Opn::Type::Label);
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Branch(false, false, Cond::AL, ir.opn1_.name_)));
            break;
          }
          case ir::IR::OpKind::ASSIGN: {
            // maybe imm or var or array
            //
            // NOTE:
            // 很多四元式类型的op1和op2都可以是Array类型
            // 但是只有assign语句的rd可能是Array类型
            assert(ir.opn2_.type_ == ir::Opn::Type::Null);
            Reg* rd = nullptr;
            Operand2* op2 = resolve_opn2operand2(&(ir.opn1_));
            if (ir.res_.type_ == ir::Opn::Type::Array) {
              // mov,rd(new vreg),op2
              rd = new_virtual_reg();
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new Move(false, Cond::AL, rd, op2)));
              // TODO: array.
              assert(0);
              // str,rd,[base #offset] (TODO: how to access base address? first
              // find. not found: ldr then bind.)
            } else {
              // mov,rd,op2
              rd = resolve_opn2reg(&(ir.res_));
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new Move(false, Cond::AL, rd, op2)));
            }
            break;
          }
          case ir::IR::OpKind::JEQ:
          case ir::IR::OpKind::JNE:
          case ir::IR::OpKind::JLT:
          case ir::IR::OpKind::JLE:
          case ir::IR::OpKind::JGT:
          case ir::IR::OpKind::JGE: {
            assert(ir.res_.type_ == ir::Opn::Type::Label);
            // CMP rn op2; BEQ label;
            Reg* rn = nullptr;
            Operand2* op2 = nullptr;
            if (ir.opn1_.type_ == ir::Opn::Type::Imm) {
              // exchange order: opn1->op2 opn2->rn res->rd
              rn = resolve_opn2reg(&(ir.opn2_));
              op2 = resolve_opn2operand2(&(ir.opn1_));
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new BinaryInst(BinaryInst::OpCode::CMP, Cond::AL, rn, op2)));
              armbb->inst_list_.push_back(static_cast<Instruction*>(new Branch(
                  false, false, get_cond_type(ir.op_, true), ir.res_.name_)));
            } else {
              // opn1->rn opn2->op2 res->rd
              rn = resolve_opn2reg(&(ir.opn1_));
              op2 = resolve_opn2operand2(&(ir.opn2_));
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new BinaryInst(BinaryInst::OpCode::CMP, Cond::AL, rn, op2)));
              armbb->inst_list_.push_back(static_cast<Instruction*>(new Branch(
                  false, false, get_cond_type(ir.op_), ir.res_.name_)));
            }
            // auto rn = resolve_opn2reg(&(ir.opn1_));
            // auto op2 = resolve_opn2operand2(&(ir.opn2_));
            break;
          }
          case ir::IR::OpKind::VOID: {
            assert(0);
            break;
          }
          default: {
            assert(0);
            break;
          }
        }
      }
    }
  }

  // maintain call_func_list
  for (auto func : module->func_list_) {
    auto armfunc = func_map[func];
    for (auto callee : func->call_func_list_) {
      auto iter = func_map.find(callee);
      if (iter == func_map.end()) {
        // buildin system library function
        armfunc->call_func_list_.push_back(new Function(callee->func_name_));
      } else {
        armfunc->call_func_list_.push_back((*iter).second);
      }
    }
  }

  // TODO:
  // 函数的开头prologue和结尾epilogue 以及call语句附近 还缺少一些指令
  // 需要知道函数使用的栈大小 过程中操作的寄存器 才能决定最终指令
  // 可以等寄存器分配之后再插入
  // 函数开头要-sp 参数也要作为栈空间的一部分 把参数从寄存器中取出放入栈中
  // TODO:
  // 数组类型暂时没处理
  // 商和取余和取非操作目前没实现
  // ret语句 B lr?
  return armmodule;
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
  outfile << "\t" << opcode << " "
          << (nullptr != this->rd_ ? (std::string(*this->rd_) + ", ") : "")
          << std::string(*this->rn_) << ", " << std::string(*this->op2_)
          << std::endl;
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
  prefix += CondToString(this->cond_) + std::string(*(this->rt_)) + " [" +
            std::string(*(this->rn_));
  switch (this->type_) {
    // NOTE: offset为0也要输出
    case Type::Norm: {
      prefix += ", " + std::string(*(this->offset_)) + "]";
      break;
    }
    case Type::Pre: {
      prefix += ", " + std::string(*(this->offset_)) + "]!";
      break;
    }
    case Type::Post: {
      prefix += "], " + std::string(*(this->offset_));
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
  std::string opcode = this->opkind_ == OpKind::PUSH ? "push" : "pop";
  outfile << "\t" << opcode << " {";
  for (auto reg : this->reg_list_) {
    outfile << " " << std::string(*reg);
  }
  outfile << " }" << std::endl;
}

void BasicBlock::EmitCode(std::ostream& outfile) {
  if (this->HasLabel()) {
    outfile << *this->label_ << ":" << std::endl;
  }
  outfile << "\t@ BasicBlock Begin." << std::endl;
  for (auto inst : this->inst_list_) {
    inst->EmitCode(outfile);
    // std::clog << "print 1 inst successfully." << std::endl;
  }
  outfile << "\t@ BasicBlock End." << std::endl;
}

void Function::EmitCode(std::ostream& outfile) {
  outfile << ".global " << func_name_ << std::endl;
  outfile << "\t.type " << func_name_ << ", %function" << std::endl;
  outfile << func_name_ << ":" << std::endl;
  outfile << "@ call_func: " << std::endl;
  for (auto func : call_func_list_) {
    outfile << "\t@ " << func->func_name_ << std::endl;
  }
  outfile << "@ Function Begin." << std::endl;
  for (auto bb : bb_list_) {
    bb->EmitCode(outfile);
  }
  outfile << "@ Function End." << std::endl;
}

void Module::EmitCode(std::ostream& outfile) {
  // outfile << ".arch armv7" << std::endl;
  auto& global_symtab = global_scope_.symbol_table_;
  if (!global_symtab.empty()) {
    outfile << ".section .data" << std::endl;
    outfile << ".align 4" << std::endl << std::endl;
    // TODO: order?
    for (auto& symbol : global_symtab) {
      outfile << ".global " << symbol.first << std::endl;
      outfile << "\t.type " << symbol.first << ", %object" << std::endl;
      outfile << symbol.first << ":" << std::endl;
      for (auto value : symbol.second.initval_) {
        outfile << "\t.word " << std::to_string(value) << std::endl;
      }
      outfile << std::endl;
    }
  }
  outfile << ".section .text" << std::endl << std::endl;
  for (auto func : func_list_) {
    func->EmitCode(outfile);
  }
}
}  // namespace arm
