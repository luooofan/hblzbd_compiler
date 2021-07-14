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

  auto div_func = new Function("__aeabi_idiv", 2, 0);
  auto mod_func = new Function("__aeabi_idivmod", 2, 0);

  for (auto func : module->func_list_) {
    // every ir func
    Function* armfunc =
        new Function(func->func_name_, func->arg_num_, func->stack_size_);
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

    int virtual_reg_id = 16;
    auto new_virtual_reg = [&virtual_reg_id]() {
      return new Reg(virtual_reg_id++);
    };

    // 添加prologue 先push 再sub 再str
    auto first_bb = bb_map[func->bb_list_.front()];
    // push {lr}
    auto push_inst = new PushPop(PushPop::OpKind::PUSH, Cond::AL);
    push_inst->reg_list_.push_back(new Reg(ArmReg::lr));
    first_bb->inst_list_.push_back(static_cast<Instruction*>(push_inst));
    // sub sp, sp, #stack_size
    first_bb->inst_list_.push_back(static_cast<Instruction*>(new BinaryInst(
        BinaryInst::OpCode::SUB, false, Cond::AL, new Reg(ArmReg::sp),
        new Reg(ArmReg::sp), new Operand2(func->stack_size_))));
    // 保存sp到vreg中  add rx, sp, 0
    // 之后要用sp的地方都找sp_vreg 除了call附近 和 epilogue中
    auto sp_vreg = new_virtual_reg();
    first_bb->inst_list_.push_back(static_cast<Instruction*>(
        new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL, sp_vreg,
                       new Reg(ArmReg::sp), new Operand2(0))));
    // str args 把参数全部存到当前栈中 顺着存 or 倒着存
    int arg_num = armfunc->arg_num_;
    for (int i = 0; i < arg_num; ++i) {
      if (i < 4) {
        // str ri sp offset=i*4
        first_bb->inst_list_.push_back(static_cast<Instruction*>(
            new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL,
                       new Reg(ArmReg(i)), sp_vreg, new Operand2(i * 4))));
      } else {
        // 可以存到当前栈中 也可以改一下符号表中的偏移
        // ldr ri sp stack_size+4+(arg_num-4)*4
        // str ri sp offset = i*4
        int offset = armfunc->stack_size_ + 4 + (i - 4) * 4;
        auto vreg = new_virtual_reg();
        first_bb->inst_list_.push_back(static_cast<Instruction*>(
            new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL, vreg,
                       sp_vreg, new Operand2(offset))));
        first_bb->inst_list_.push_back(static_cast<Instruction*>(
            new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, vreg,
                       sp_vreg, new Operand2(i * 4))));
      }
    }

    std::unordered_map<std::string, std::unordered_map<int, Reg*>>
        var_map;  // varname, scope_id, reg

    // 要为每一个ret语句添加epilogue
    auto add_epilogue = [&func](BasicBlock* armbb) {
      // add sp, sp, #stack_size
      armbb->inst_list_.push_back(static_cast<Instruction*>(new BinaryInst(
          BinaryInst::OpCode::ADD, false, Cond::AL, new Reg(ArmReg::sp),
          new Reg(ArmReg::sp), new Operand2(func->stack_size_))));
      // pop {pc} NOTE: same as bx lr
      auto pop_inst = new PushPop(PushPop::OpKind::POP, Cond::AL);
      pop_inst->reg_list_.push_back(new Reg(ArmReg::pc));
      armbb->inst_list_.push_back(static_cast<Instruction*>(pop_inst));
    };

    // TODO: 立即数处理
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

      auto resolve_imm2operand2 = [&armbb, &new_virtual_reg](int imm) {
        int encoding = imm;
        for (int ror = 0; ror < 32; ror += 2) {
          if (!(encoding & ~0xFFu)) {
            return new Operand2(imm);
          }
          encoding = (encoding << 2u) | (encoding >> 30u);
        }
        // use move or ldr
        auto vreg = new_virtual_reg();
        armbb->inst_list_.push_back(
            static_cast<Instruction*>(new LdrStr(vreg, std::to_string(imm))));
        return new Operand2(vreg);
      };

      auto resolve_opn2operand2 = [&sp_vreg, &armbb, &var_map, &new_virtual_reg,
                                   &resolve_imm2operand2](ir::Opn* opn) {
        // 如果是立即数 返回立即数类型的Operand2
        // 如果是变量 需要判断是否为中间变量
        //  是中间变量则先查varmap 没找到则生成一个新的reg类型的Op2 记录 返回
        //  如果不是 要先ldr到一个新的vreg中 不用记录 返回reg类型的Op2
        // 如果是数组 先找基址 要先ldr到一个新的vreg中 不记录 返回
        if (opn->type_ == ir::Opn::Type::Imm) {
          return resolve_imm2operand2(opn->imm_num_);
          return new Operand2(opn->imm_num_);
        } else if (opn->type_ == ir::Opn::Type::Array) {
          // Array 找到基址 如果存在rx中 (ldr, rv, rx, offset)
          // 如果没找到 则(add, rbase, vsp, offset)记录(ldr, rv, rbase, offset)
          // 返回一个rv的op2
          Reg* rbase = nullptr;
          const auto& iter = var_map.find(opn->name_);
          if (iter != var_map.end()) {
            const auto& iter2 = (*iter).second.find(opn->scope_id_);
            if (iter2 != (*iter).second.end()) {
              rbase = ((*iter2).second);
            }
          }
          Reg* vreg = nullptr;
          if (nullptr != rbase) {
            vreg = new_virtual_reg();
          } else {
            rbase = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = rbase;
            vreg = new_virtual_reg();
            if (opn->scope_id_ != 0) {
              auto& symbol =
                  ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new BinaryInst(
                      BinaryInst::OpCode::ADD, false, Cond::AL, rbase, sp_vreg,
                      new Operand2(symbol.offset_))));
            } else {
              // 全局数组量
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new LdrStr(rbase, opn->name_)));
            }
          }
          armbb->inst_list_.push_back(static_cast<Instruction*>(
              new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL,
                         vreg, rbase, new Operand2(opn->offset_->imm_num_))));
          return new Operand2(vreg);
        } else {
          if (opn->scope_id_ == 0) {
            const auto& iter = var_map.find(opn->name_);
            if (iter != var_map.end()) {
              const auto& iter2 = (*iter).second.find(opn->scope_id_);
              if (iter2 != (*iter).second.end()) {
                return new Operand2((*iter2).second);
              }
            }
            Reg* vreg = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = vreg;
            // 全局变量
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new LdrStr(vreg, opn->name_)));
            return new Operand2(vreg);
          }
          // Var offset为-1或者以temp-开头表示中间变量
          auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
          if (symbol.offset_ == -1) {
            const auto& iter = var_map.find(opn->name_);
            if (iter != var_map.end()) {
              const auto& iter2 = (*iter).second.find(opn->scope_id_);
              if (iter2 != (*iter).second.end()) {
                return new Operand2((*iter2).second);
              }
            }
            Reg* vreg = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = vreg;
            return new Operand2(vreg);
          } else {
            // 非中间变量 ldr rd, [sp, #offset]
            auto vreg = new_virtual_reg();
            auto offset = new Operand2(symbol.offset_);
            armbb->inst_list_.push_back({static_cast<Instruction*>(
                new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL,
                           vreg, sp_vreg, offset))});
            return new Operand2(vreg);
          }
        }
      };

      auto resolve_opn2reg = [&sp_vreg, &armbb, &var_map, &new_virtual_reg,
                              &resolve_opn2operand2](ir::Opn* opn) {
        // 如果是立即数 先move到一个vreg中 返回这个vreg
        // 如果是变量 需要判断是否为中间变量
        //  是中间变量则先查varmap 如果没有的话生成一个新的vreg 记录 返回
        //  如果不是 要先ldr到一个新的vreg中 不用记录 返回
        // 如果是数组 先找基址 要先ldr到一个新的vreg中 不记录 返回
        if (opn->type_ == ir::Opn::Type::Imm) {
          Reg* rd = new_virtual_reg();
          // mov rd(vreg) op2
          auto op2 = resolve_opn2operand2(opn);
          armbb->inst_list_.push_back(
              static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
          return rd;
        } else if (opn->type_ == ir::Opn::Type::Array) {
          // Array 找到基址 如果存在rx中 (ldr, rv, rx, offset)
          // 如果没找到 则(add, rbase, vsp, offset)记录(ldr, rv, rbase, offset)
          // 返回rv
          Reg* rbase = nullptr;
          const auto& iter = var_map.find(opn->name_);
          if (iter != var_map.end()) {
            const auto& iter2 = (*iter).second.find(opn->scope_id_);
            if (iter2 != (*iter).second.end()) {
              rbase = ((*iter2).second);
            }
          }
          Reg* vreg = nullptr;
          if (nullptr != rbase) {
            vreg = new_virtual_reg();
          } else {
            rbase = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = rbase;
            vreg = new_virtual_reg();
            if (opn->scope_id_ != 0) {
              auto& symbol =
                  ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new BinaryInst(
                      BinaryInst::OpCode::ADD, false, Cond::AL, rbase, sp_vreg,
                      new Operand2(symbol.offset_))));
            } else {
              // 全局数组量
              armbb->inst_list_.push_back(
                  static_cast<Instruction*>(new LdrStr(rbase, opn->name_)));
            }
          }
          armbb->inst_list_.push_back(static_cast<Instruction*>(
              new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL,
                         vreg, rbase, new Operand2(opn->offset_->imm_num_))));
          return vreg;
        } else {
          if (opn->scope_id_ == 0) {
            const auto& iter = var_map.find(opn->name_);
            if (iter != var_map.end()) {
              const auto& iter2 = (*iter).second.find(opn->scope_id_);
              if (iter2 != (*iter).second.end()) {
                return (*iter2).second;
              }
            }
            Reg* vreg = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = vreg;
            // 全局变量
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new LdrStr(vreg, opn->name_)));
            return vreg;
          }
          // Var offset为-1或者以temp-开头表示中间变量
          auto& symbol = ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
          if (symbol.offset_ == -1) {
            const auto& iter = var_map.find(opn->name_);
            if (iter != var_map.end()) {
              const auto& iter2 = (*iter).second.find(opn->scope_id_);
              if (iter2 != (*iter).second.end()) {
                return (*iter2).second;
              }
            }
            Reg* vreg = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = vreg;
            return vreg;
          } else {
            // 非中间变量 ldr rd, [sp, #offset]
            // TODO: 如何存储sp
            auto vreg = new_virtual_reg();
            auto offset = new Operand2(symbol.offset_);
            armbb->inst_list_.push_back({static_cast<Instruction*>(
                new LdrStr(LdrStr::OpKind::LDR, LdrStr::Type::Norm, Cond::AL,
                           vreg, sp_vreg, offset))});
            return vreg;
          }
        }
      };

      auto load_global_opn2reg = [&var_map, &new_virtual_reg,
                                  &armbb](ir::Opn* opn) {
        // 如果是全局变量并且没在varmap里找到就ldr 然后再str
        if (opn->scope_id_ == 0) {
          Reg* rglo = nullptr;
          const auto& iter = var_map.find(opn->name_);
          if (iter != var_map.end()) {
            const auto& iter2 = (*iter).second.find(opn->scope_id_);
            if (iter2 != (*iter).second.end()) {
              rglo = (*iter2).second;
            }
          }
          if (nullptr == rglo) {
            rglo = new_virtual_reg();
            var_map[opn->name_][opn->scope_id_] = rglo;
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new LdrStr(rglo, opn->name_)));
          }
        }
      };

      auto resolve_resopn2rdreg_with_biinst =
          [&var_map, &armbb, &sp_vreg, &new_virtual_reg, &load_global_opn2reg](
              ir::Opn* opn, BinaryInst::OpCode opcode, Reg* rn, Operand2* op2) {
            // 只能是Var类型 或者Array？
            assert(opn->type_ == ir::Opn::Type::Var);
            load_global_opn2reg(opn);
            // Var offset为-1或者以temp-开头表示中间变量
            auto& symbol =
                ir::gScopes[opn->scope_id_].symbol_table_[opn->name_];
            // 是中间变量则只生成一条运算指令
            Reg* rd = nullptr;
            if (-1 == symbol.offset_) {
              const auto& iter = var_map.find(opn->name_);
              if (iter != var_map.end()) {
                const auto& iter2 = (*iter).second.find(opn->scope_id_);
                if (iter2 != (*iter).second.end()) {
                  rd = (*iter2).second;
                }
              }
            }
            if (nullptr == rd) {
              rd = new_virtual_reg();
              var_map[opn->name_][opn->scope_id_] = rd;
            }
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new BinaryInst(opcode, false, Cond::AL, rd, rn, op2)));
            // 否则还要生成一条str指令
            if (opn->scope_id_ == 0) {
              armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(
                  LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
                  var_map[opn->name_][opn->scope_id_], new Operand2(0))));
            } else if (-1 != symbol.offset_) {
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL,
                             rd, sp_vreg, new Operand2(symbol.offset_))));
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
            resolve_resopn2rdreg_with_biinst(&(ir.res_),
                                             BinaryInst::OpCode::ADD, rn, op2);
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
            resolve_resopn2rdreg_with_biinst(&(ir.res_), opcode, rn, op2);
            break;
          }
          case ir::IR::OpKind::MUL: {
            // if has imm, add mov inst
            auto rm = resolve_opn2reg(&(ir.opn1_));
            auto rs = resolve_opn2reg(&(ir.opn2_));
            auto op2 = new Operand2(rs);
            resolve_resopn2rdreg_with_biinst(&(ir.res_),
                                             BinaryInst::OpCode::MUL, rm, op2);
            break;
          }
          case ir::IR::OpKind::DIV: {
            // if has imm, add mov inst
            // may can use __aeabi_idiv. r0
            auto rm = resolve_opn2reg(&(ir.opn1_));
            auto rs = resolve_opn2reg(&(ir.opn2_));
            auto op2 = new Operand2(rs);
            resolve_resopn2rdreg_with_biinst(&(ir.res_),
                                             BinaryInst::OpCode::SDIV, rm, op2);
            break;
          }
          case ir::IR::OpKind::MOD: {
            // may can use __aeabi_idivmod. r1
            auto rm = resolve_opn2operand2(&(ir.opn1_));
            auto rn = resolve_opn2operand2(&(ir.opn2_));
            // mov r1, rn
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Move(false, Cond::AL, new Reg(ArmReg::r1), rn)));
            // mov r0, rm
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Move(false, Cond::AL, new Reg(ArmReg::r0), rm)));
            // call __aeabi_idivmod
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Branch(true, false, Cond::AL, "__aeabi_idivmod")));
            // TODO: 维护call_list
            // if (armfunc->call_func_list_.find(mod_func) ==
            //     armfunc->call_func_list_.end()) {
            //   armfunc->call_func_list_.push_back(mod_func);
            // }
            // mov rd, r1
            auto rd = resolve_opn2reg(&(ir.res_));
            armbb->inst_list_.push_back(static_cast<Instruction*>(new Move(
                false, Cond::AL, rd, new Operand2(new Reg(ArmReg::r1)))));
            break;
          }
          case ir::IR::OpKind::NOT: {
            // cmp rn 0
            // moveq rd 1
            // movne rd 0
            auto rn = resolve_opn2reg(&(ir.opn1_));
            load_global_opn2reg(&(ir.res_));
            auto& symbol =
                ir::gScopes[ir.res_.scope_id_].symbol_table_[ir.res_.name_];
            // 是中间变量则只生成一条运算指令
            Reg* rd = nullptr;
            if (-1 == symbol.offset_) {
              const auto& iter = var_map.find(ir.res_.name_);
              if (iter != var_map.end()) {
                const auto& iter2 = (*iter).second.find(ir.res_.scope_id_);
                if (iter2 != (*iter).second.end()) {
                  rd = (*iter2).second;
                }
              }
            }
            if (nullptr == rd) {
              rd = new_virtual_reg();
              var_map[ir.res_.name_][ir.res_.scope_id_] = rd;
            }
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new BinaryInst(
                    BinaryInst::OpCode::CMP, Cond::AL, rn, new Operand2(0))));
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Move(false, Cond::EQ, rd, new Operand2(1))));
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Move(false, Cond::NE, rd, new Operand2(0))));
            // 否则还要生成一条str指令
            if (0 == ir.res_.scope_id_) {
              armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(
                  LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
                  var_map[ir.res_.name_][ir.res_.scope_id_], new Operand2(0))));
            } else if (-1 != symbol.offset_) {
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new LdrStr(LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL,
                             rd, sp_vreg, new Operand2(symbol.offset_))));
            }
            break;
          }
          case ir::IR::OpKind::NEG: {
            // impl by RSB res, opn1, #0
            auto rn = resolve_opn2reg(&(ir.opn1_));
            auto op2 = new Operand2(0);
            resolve_resopn2rdreg_with_biinst(&(ir.res_),
                                             BinaryInst::OpCode::RSB, rn, op2);
            break;
          }
          case ir::IR::OpKind::LABEL: {
            assert(0);
            // 所有label语句都被跳过
            break;
          }
          case ir::IR::OpKind::PARAM: {
            // 所有param语句放到call的时候处理
            break;
          }
          case ir::IR::OpKind::CALL: {
            assert(ir.opn1_.type_ == ir::Opn::Type::Func);
            // 处理param语句
            // ir[start-1] is call
            // NOTE 需要确保IR中函数调用表现为一系列PARAM+一条CALL
            int param_num = ir.opn2_.imm_num_;
            if (param_num > 4) {
              // sub sp, sp, #(param_num-4)*4
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new BinaryInst(BinaryInst::OpCode::SUB, false, Cond::AL,
                                 new Reg(ArmReg::sp), new Reg(ArmReg::sp),
                                 new Operand2((param_num - 4) * 4))));
            }
            int param_start = start - 1 - param_num;
            for (int i = param_num - 1; i >= 0; --i) {
              auto& param_ir = ir::gIRList[param_start + i];
              if (i < 4) {
                auto op2 = resolve_opn2operand2(&(param_ir.opn1_));
                // r0-r3. MOV rx, op2
                auto rd = new Reg(static_cast<ArmReg>(i));
                armbb->inst_list_.push_back(static_cast<Instruction*>(
                    new Move(false, Cond::AL, rd, op2)));
              } else {
                // str rd, sp, #(i-4)*4 靠后的参数放在较高的地方
                auto rd = resolve_opn2reg(&(param_ir.opn1_));
                armbb->inst_list_.push_back(
                    static_cast<Instruction*>(new LdrStr(
                        LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
                        new Reg(ArmReg::sp), new Operand2((i - 4) * 4))));
              }
            }
            // BL label
            armbb->inst_list_.push_back(static_cast<Instruction*>(
                new Branch(true, false, Cond::AL, ir.opn1_.name_)));
            if (ir.res_.type_ != ir::Opn::Type::Null) {
              var_map[ir.res_.name_][ir.res_.scope_id_] = new Reg(ArmReg::r0);
              // dbg_print_var_map();
            }

            if (param_num > 4) {
              // add sp, sp, #(param_num-4)*4
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new BinaryInst(BinaryInst::OpCode::ADD, false, Cond::AL,
                                 new Reg(ArmReg::sp), new Reg(ArmReg::sp),
                                 new Operand2((param_num - 4) * 4))));
            }
            break;
          }
          case ir::IR::OpKind::RET: {
            auto rd = new Reg(ArmReg::r0);
            Operand2* op2 = nullptr;
            if (ir.opn1_.type_ != ir::Opn::Type::Null) {
              op2 = resolve_opn2operand2(&(ir.opn1_));
            } else {
              // NOTE: ret void时 应视为给r0赋某值 保证活跃分析正确
              op2 = new Operand2(0);
            }
            armbb->inst_list_.push_back(
                static_cast<Instruction*>(new Move(false, Cond::AL, rd, op2)));
            add_epilogue(armbb);
            // armbb->inst_list_.push_back(static_cast<Instruction*>(
            //     new Branch(false, true, Cond::AL, "lr")));
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
            Operand2* op2 = resolve_opn2operand2(&(ir.opn1_));
            load_global_opn2reg(&(ir.res_));
            if (ir.res_.type_ == ir::Opn::Type::Array) {
              Reg* rd = nullptr;
              // mov,rd(new vreg),op2
              rd = new_virtual_reg();
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new Move(false, Cond::AL, rd, op2)));
              // 先找有没有rbase 没有就用一条ldr指令加载到rbase里并记录
              // 有了rbase之后 str rd, rbase, offset
              Reg* rbase = nullptr;
              const auto& iter = var_map.find(ir.res_.name_);
              if (iter != var_map.end()) {
                const auto& iter2 = (*iter).second.find(ir.res_.scope_id_);
                if (iter2 != (*iter).second.end()) {
                  rbase = ((*iter2).second);
                }
              }
              if (nullptr == rbase) {
                rbase = new_virtual_reg();
                var_map[ir.res_.name_][ir.res_.scope_id_] = rbase;
                auto& symbol =
                    ir::gScopes[ir.res_.scope_id_].symbol_table_[ir.res_.name_];
                armbb->inst_list_.push_back(
                    static_cast<Instruction*>(new BinaryInst(
                        BinaryInst::OpCode::ADD, false, Cond::AL, rbase,
                        sp_vreg, new Operand2(symbol.offset_))));
              }
              armbb->inst_list_.push_back(static_cast<Instruction*>(new LdrStr(
                  LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd, rbase,
                  new Operand2(ir.res_.offset_->imm_num_))));
            } else {
              // Var offset为-1或者以temp-开头表示中间变量
              auto& symbol =
                  ir::gScopes[ir.res_.scope_id_].symbol_table_[ir.res_.name_];
              // 是中间变量则只生成一条运算指令
              Reg* rd = nullptr;
              if (-1 == symbol.offset_) {
                const auto& iter = var_map.find(ir.res_.name_);
                if (iter != var_map.end()) {
                  const auto& iter2 = (*iter).second.find(ir.res_.scope_id_);
                  if (iter2 != (*iter).second.end()) {
                    rd = (*iter2).second;
                  }
                }
              }
              if (nullptr == rd) {
                rd = new_virtual_reg();
                var_map[ir.res_.name_][ir.res_.scope_id_] = rd;
              }
              // mov,rd,op2
              armbb->inst_list_.push_back(static_cast<Instruction*>(
                  new Move(false, Cond::AL, rd, op2)));
              // 否则还要生成一条str指令
              if (0 == ir.res_.scope_id_) {
                armbb->inst_list_.push_back(
                    static_cast<Instruction*>(new LdrStr(
                        LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
                        var_map[ir.res_.name_][ir.res_.scope_id_],
                        new Operand2(0))));
              } else if (-1 != symbol.offset_) {
                armbb->inst_list_.push_back(
                    static_cast<Instruction*>(new LdrStr(
                        LdrStr::OpKind::STR, LdrStr::Type::Norm, Cond::AL, rd,
                        sp_vreg, new Operand2(symbol.offset_))));
              }
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

    // epilogue: NOTE: 中间代码保证了函数执行流最后一定有一条return语句
    // auto last_bb = bb_map[func->bb_list_.back()];
    // add_epilogue(last_bb);
  }

  // maintain call_func_list
  for (auto func : module->func_list_) {
    auto armfunc = func_map[func];
    for (auto callee : func->call_func_list_) {
      auto iter = func_map.find(callee);
      if (iter == func_map.end()) {
        // buildin system library function
        armfunc->call_func_list_.push_back(new Function(
            callee->func_name_, callee->arg_num_, callee->stack_size_));
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
  prefix += CondToString(this->cond_) + " " + std::string(*(this->rd_)) + ", ";
  switch (this->type_) {
    // NOTE: offset为0也要输出
    case Type::Norm: {
      prefix += "[" + std::string(*(this->rn_)) + ", " +
                std::string(*(this->offset_)) + "]";
      break;
    }
    case Type::Pre: {
      prefix += "[" + std::string(*(this->rn_)) + ", " +
                std::string(*(this->offset_)) + "]!";
      break;
    }
    case Type::Post: {
      prefix += "[" + std::string(*(this->rn_)) + "], " +
                std::string(*(this->offset_));
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
  outfile << std::endl;
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