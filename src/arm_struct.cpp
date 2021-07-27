#include "../include/arm_struct.h"

#include <algorithm>
#include <iostream>
#include <string>

#define ASSERT_ENABLE
#include "../include/myassert.h"

#define GET_BB_LABEL_STR(bb_ptr) (nullptr != bb_ptr->label_ ? (*(bb_ptr->label_)) : ("UnamedBB"))
#define GET_BB_IDENT(bb_ptr) (GET_BB_LABEL_STR(bb_ptr) + ":" + std::to_string(bb_ptr->IndexInFunc()))

void ArmModule::EmitCode(std::ostream& out) {
  out << ".arch armv7ve" << std::endl;
  out << ".arm" << std::endl;
  out << "@ module: " << this->name_ << std::endl;
  out << std::endl;
  out << R"(
.macro mov32, reg, val
    movw \reg, #:lower16:\val
    movt \reg, #:upper16:\val
.endm
  )" << std::endl;
  out << std::endl;
  // .data
  auto& global_symtab = this->global_scope_.symbol_table_;
  if (!global_symtab.empty()) {
    // out << ".section .data" << std::endl;
    // out << ".align 4" << std::endl;
    // out << std::endl;
    // TODO: order?
    for (auto& symbol : global_symtab) {
      if (symbol.first == "_sysy_idx" || symbol.first == "_sysy_l1" || symbol.first == "_sysy_l2" ||
          symbol.first == "_sysy_h" || symbol.first == "_sysy_m" || symbol.first == "_sysy_s" ||
          symbol.first == "_sysy_us")
        continue;
      // NOTE: 数组常量不能删 可能会用到变量下标
      if (symbol.second.is_const_ && !symbol.second.is_array_) continue;
      bool in_bss = true;
      int last_not0 = 0;
      for (int i = 0; i < symbol.second.initval_.size(); ++i) {
        if (0 != symbol.second.initval_[i]) {
          in_bss = false;
          last_not0 = i;
        }
      }
      if (in_bss) {
        // .comm symbol length align
        out << ".comm " << symbol.first << ", " << (symbol.second.width_.empty() ? 4 : symbol.second.width_[0]) << ", 4"
            << std::endl;
      } else {
        out << "\t.global " << symbol.first << std::endl;
        out << "\t.data" << std::endl;
        out << "\t.align 4" << std::endl;
        out << "\t.type " << symbol.first << ", %object" << std::endl;
        out << symbol.first << ":" << std::endl;
        int cnt0 = 0;
        for (int i = 0; i < symbol.second.initval_.size(); ++i) {
          if (0 == symbol.second.initval_[i]) {
            ++cnt0;
          } else {
            if (0 != cnt0) {
              out << "\t.space " << cnt0 * 4 << std::endl;
              cnt0 = 0;
            }
            out << "\t.word " << symbol.second.initval_[i] << std::endl;
          }
          if (i == last_not0) break;
        }
        int space = (symbol.second.initval_.size() - last_not0 - 1) * 4;
        if (0 != space) {
          out << "\t.space " << space << std::endl;
        }
      }
      out << std::endl;
    }
  }

  // .text
  out << ".section .text" << std::endl;
  out << std::endl;
  // out << ".syntax unified" << std::endl << std::endl;
  for (auto func : this->func_list_) {
    func->EmitCode(out);
    out << std::endl;
  }
}
void ArmModule::Check() {
  for (auto func : func_list_) func->Check();
}

void ArmFunction::EmitCode(std::ostream& out) {
  out << ".global " << this->name_ << std::endl;
  out << "\t.type " << this->name_ << ", %function" << std::endl;
  out << this->name_ << ":" << std::endl;
  out << "@ call_func: ";
  for (auto func : this->call_func_list_) {
    out << func->name_ << " ";
  }
  // out << std::endl;
  out << "@ arg_num: " << this->arg_num_ << " ";        // << std::endl;
  out << "@ stack_size: " << this->stack_size_ << " ";  // << std::endl;
  out << "@ virtual_reg_max: " << this->virtual_reg_max << " ";
  out << std::endl;

  out << "@ Function Begin:" << std::endl;
  for (auto bb : bb_list_) {
    bb->EmitCode(out);
    out << std::endl;
  }
  out << "@ Function End." << std::endl;
}
void ArmFunction::Check() {
  for (auto bb : bb_list_) bb->Check();
}

void ArmBasicBlock::EmitCode(std::ostream& out) {
  if (this->HasLabel()) {
    out << *this->label_ << ":" << std::endl;
  }
  out << "  @ ID: " << this->IndexInFunc() << std::endl;
  out << "  @ BasicBlock Begin:" << std::endl;
  out << "  @ pred: ";
  for (auto pred : this->pred_) {
    out << GET_BB_IDENT(pred) << " ";
  }
  // out << std::endl;
  out << "  @ succ: ";
  for (auto succ : this->succ_) {
    out << GET_BB_IDENT(succ) << " ";
  }
  // out << std::endl;
  out << "  @ use: ";
  for (auto use : this->use_) {
    out << "r" << use << " ";
  }
  // out << std::endl;
  out << "  @ def: ";
  for (auto def : this->def_) {
    out << "r" << def << " ";
  }
  // out << std::endl;
  out << "  @ livein: ";
  for (auto livein : this->livein_) {
    out << "r" << livein << " ";
  }
  // out << std::endl;
  out << "  @ liveout: ";
  for (auto liveout : this->liveout_) {
    out << "r" << liveout << " ";
  }
  out << std::endl;
  for (auto inst : this->inst_list_) {
    inst->EmitCode(out);
  }
  out << "  @ BasicBlock End." << std::endl;
}
void ArmBasicBlock::Check() {
  for (auto inst : inst_list_) inst->Check();
}
int ArmBasicBlock::IndexInFunc() {
  MyAssert(nullptr != this->func_);
  auto& bbs = this->func_->bb_list_;
  MyAssert(std::find(bbs.begin(), bbs.end(), this) != bbs.end());
  return std::distance(bbs.begin(), std::find(bbs.begin(), bbs.end(), this));
}

#undef ASSERT_ENABLE  // disable assert. this should be placed at the end of every file.