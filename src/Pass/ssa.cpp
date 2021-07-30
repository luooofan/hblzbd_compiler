#include "../../include/Pass/ssa.h"

#include "../../include/Pass/ir_liveness_analysis.h"
#define ASSERT_ENABLE  // enable assert for this file.
#include "../../include/myassert.h"

void ConvertSSA::Run() {
  auto m = dynamic_cast<IRModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto f : m->func_list_) {
    InsertPhiIR(f);
    this->count_.clear();
    this->stack_.clear();
    Rename(f->bb_list_.front());
  }
}

void ConvertSSA::Rename(IRBasicBlock* bb) {
  std::unordered_map<std::string, int> deftimes;
  // 对bb中每条ir的使用和定值var rename
  for (auto ir : bb->ir_list_) {
    auto [def, use] = GetDefUsePtr(ir);
    auto [defvar, usevar] = GetDefUse(ir);
    if (ir->op_ != IR::OpKind::PHI) {
      for (int i = 0; i < use.size(); ++i) {
        if (this->stack_.find(usevar[i]) == this->stack_.end()) {
          MyAssert(this->count_.find(usevar[i]) == this->count_.end());
          this->stack_[usevar[i]].push(0);
          this->count_[usevar[i]] = 0;
        }
        use[i]->ssa_id_ = this->stack_[usevar[i]].top();
      }
    }
    for (int i = 0; i < def.size(); ++i) {
      if (deftimes.find(defvar[i]) == deftimes.end()) {
        deftimes[defvar[i]] = 1;
      } else {
        ++deftimes[defvar[i]];
      }
      if (this->count_.find(defvar[i]) == this->count_.end()) {
        MyAssert(this->stack_.find(defvar[i]) == this->stack_.end());
        this->stack_[defvar[i]].push(0);
        this->count_[defvar[i]] = 1;
        this->stack_[defvar[i]].push(1);
      }
      int cnt = ++this->count_[defvar[i]];
      this->stack_[defvar[i]].push(cnt);
      def[i]->ssa_id_ = cnt;
    }
  }  // end of ir for-loop
  // 填写bb的succ中的phi ir
  for (auto succ : bb->succ_) {
    int order = std::distance(succ->pred_.begin(), std::find(succ->pred_.begin(), succ->pred_.end(), succ));
    for (auto phi_ir : succ->ir_list_) {
      if (phi_ir->op_ != IR::OpKind::PHI) break;
      phi_ir->phi_args_[order] = phi_ir->res_;
      auto varname = phi_ir->res_.name_ + "#" + std::to_string(phi_ir->res_.scope_id_);
      phi_ir->phi_args_[order].ssa_id_ = this->stack_[varname].top();
    }
  }
  // 遍历支配树子结点
  for (auto dom : bb->doms_) {
    Rename(dom);
  }
  // 维护stack
  for (auto it = deftimes.begin(); it != deftimes.end(); ++it) {
    auto& stack = this->stack_[(*it).first];
    for (int i = 0; i < (*it).second; ++i) {
      MyAssert(!stack.empty());
      stack.pop();
    }
  }
}

void ConvertSSA::InsertPhiIR(IRFunction* f) {
  std::unordered_map<std::string, std::unordered_set<IRBasicBlock*>> defsites;  // 变量被定值的基本块集合
  std::unordered_map<IRBasicBlock*, std::unordered_set<std::string>> phi_vars;  // 一个基本块内拥有phi函数的变量
  // fill defsites
  for (auto bb : f->bb_list_) {
    for (auto& def : bb->def_) {
      defsites[def].insert(bb);
    }
  }
  for (auto& [var, defbbs] : defsites) {
    while (!defbbs.empty()) {
      auto bb = *defbbs.begin();
      defbbs.erase(defbbs.begin());
      for (auto df : bb->df_) {  // 对于变量定值所在bb的每一个df都要加一个phi函数
        if (phi_vars.find(df) == phi_vars.end() || phi_vars[df].find(var) == phi_vars[df].end()) {
          phi_vars[df].insert(var);
          auto res = ir::Opn();
          df->ir_list_.insert(df->ir_list_.begin(), new ir::IR{ir::IR::OpKind::PHI, res, df->pred_.size()});
          if (df->def_.find(var) == df->def_.end()) {
            df->def_.insert(var);
            defbbs.insert(df);
          }
        }
      }  // end of df for
    }    // end of defbbs while
  }      // end of defsites for
}

#undef ASSERT_ENABLE  // disable assert. this should be placed at the end of every file.
