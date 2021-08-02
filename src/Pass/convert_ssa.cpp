#include "../../include/Pass/convert_ssa.h"

#include "../../include/Pass/ir_liveness_analysis.h"
#include "../../include/ir_getdefuse.h"
#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"
#define ASSERT_ENABLE  // enable assert for this file.
#include "../../include/myassert.h"

void ConvertSSA::Run() {
  auto m = dynamic_cast<IRModule*>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto f : m->func_list_) {
    InsertPhiIR(f);
    // f->EmitCode(std::cout);
    this->count_.clear();
    this->stack_.clear();
    Rename(f->bb_list_.front());
  }
  m->EmitCode();
  auto ssam = this->ConstructSSA(m);
  ssam->EmitCode();
}

Value* ConvertSSA::FindValueFromOpn(Opn* opn) { return this->FindValueFromCompName(opn->GetCompName()); }
Value* ConvertSSA::FindValueFromCompName(const std::string& comp_name) {
  auto it = work_map.find(comp_name);
  if (it != work_map.end()) {
    return (*it).second;
  } else {
    return nullptr;
  }
}

Value* ConvertSSA::ResolveOpn2Value(Opn* opn, SSABasicBlock* ssabb) {
  // std::cout << "Resolve Begin:" << std::endl;
  Value* res = nullptr;
  auto type = opn->type_;
  if (type == Opn::Type::Null) {
    MyAssert(0);
  } else if (type == Opn::Type::Imm) {  // new一个常量value直接返回
    res = new ConstantInt(opn->imm_num_);
  } else {  // 先找work_map中有没有目标value 没有的话根据opn new一个value 并记录映射
    auto comp_name = opn->GetCompName();
    res = this->FindValueFromCompName(comp_name);
    if (0 == opn->scope_id_) {  // 全局量
      MyAssert(nullptr != res && res->GetType()->IsPointer());
      // new load 不需要记录 全局量每次都要load
      if (type == Opn::Type::Var) {
        res = new LoadInst(new Type(Type::IntegerTyID), "glo_temp_" + opn->name_, res, ssabb);
      } else {  // global array
        auto offset = ResolveOpn2Value(opn->offset_, ssabb);
        res = new LoadInst(new Type(Type::IntegerTyID), "glo_temp_" + opn->name_, res, offset, ssabb);
      }
    }
    if (nullptr == res) {  // 未定义变量
      MyAssert(type == Opn::Type::Var);
      // new undefval
      res = new UndefVariable(new Type(Type::IntegerTyID), comp_name);
      work_map[comp_name] = res;
    }
  }
  // std::cout << "Resolve End." << std::endl;
  MyAssert(nullptr != res);
  return res;
}

int GetOpKind(ir::IR::OpKind srcop) {
  switch (srcop) {
    case ir::IR::OpKind::ADD: {
      return BinaryOperator::ADD;
      break;
    }
    case ir::IR::OpKind::SUB: {
      return BinaryOperator::SUB;
      break;
    }
    case ir::IR::OpKind::MUL: {
      return BinaryOperator::MUL;
      break;
    }
    case ir::IR::OpKind::DIV: {
      return BinaryOperator::DIV;
      break;
    }
    case ir::IR::OpKind::MOD: {
      return BinaryOperator::MOD;
      break;
    }
    case ir::IR::OpKind::NOT: {
      return UnaryOperator::NOT;
      break;
    }
    case ir::IR::OpKind::NEG: {
      return UnaryOperator::NEG;
      break;
    }
    case ir::IR::OpKind::JEQ: {
      return BranchInst::Cond::EQ;
      break;
    }
    case ir::IR::OpKind::JNE: {
      return BranchInst::Cond::NE;
      break;
    }
    case ir::IR::OpKind::JLT: {
      return BranchInst::Cond::GT;
      break;
    }
    case ir::IR::OpKind::JLE: {
      return BranchInst::Cond::GE;
      break;
    }
    case ir::IR::OpKind::JGT: {
      return BranchInst::Cond::LT;
      break;
    }
    case ir::IR::OpKind::JGE: {
      return BranchInst::Cond::LE;
      break;
    }
    default: {
      return -1;
      break;
    }
  }
  return -1;
}

SSAModule* ConvertSSA::ConstructSSA(IRModule* module) {
  // 由ir module直接构建ssa module 构建的过程中把ir中的label语句删掉 转化成了基本块的label
  // NOTE: 可能会出现一些空的bb 有些基本块是unnamed
  SSAModule* ssamodule = new SSAModule(module->name_);
  std::unordered_map<IRFunction*, SSAFunction*> func_map;

  // process all global variable
  for (auto& [symbol, symbol_item] : module->global_scope_.symbol_table_) {
    // new glob. record size: int-4 array-4*n. as a i32 pointer.
    auto glob_var = new GlobalVariable(symbol, symbol_item.width_[0]);
    if (symbol_item.is_const_) {  // NOTE 此时一定是常量数组 原符号表项中的initval失效
      glob_var->val_.swap(symbol_item.initval_);
    }
    ssamodule->AddGlobalVariable(glob_var);
    glob_map[symbol] = glob_var;
  }

  // for every ir func
  for (auto func : module->func_list_) {
    std::cout << "--Process Func:" << func->name_ << std::endl;
    SSAFunction* ssafunc = new SSAFunction(func->name_, ssamodule);
    func_map.insert({func, ssafunc});
    // glob_map[func->name_] = new FunctionValue(new FunctionType(), func->name_, ssafunc);

    work_map.clear();

    // create armbb according to irbb
    std::unordered_map<IRBasicBlock*, SSABasicBlock*> bb_map;
    for (auto bb : func->bb_list_) {
      SSABasicBlock* ssabb = new SSABasicBlock(ssafunc);
      bb_map.insert({bb, ssabb});
    }

    // maintain pred and succ
    for (auto bb : func->bb_list_) {
      for (auto pred : bb->pred_) {  // NOTE: it's OK?
        bb_map[bb]->AddPredBB(bb_map[pred]);
        // bb_map[pred]->AddSuccBB(bb_map[bb]);
      }
      for (auto succ : bb->succ_) {
        bb_map[bb]->AddSuccBB(bb_map[succ]);
      }
    }

    // 处理所有label语句 给ssabasicblock绑定bbvalue 把bbvalue添加到workmap中
    for (auto bb : func->bb_list_) {
      auto ssabb = bb_map[bb];
      auto first_ir = bb->ir_list_.front();
      if (first_ir->op_ == ir::IR::OpKind::LABEL) {
        if (first_ir->opn1_.type_ == ir::Opn::Type::Label) {  // 如果是label就把label赋给这个bb的label
          auto& label = first_ir->opn1_.name_;
          ssabb->SetLabel(label);
          MyAssert(work_map.find(label) == work_map.end());
          work_map[label] = new BasicBlockValue(label, ssabb);
        }  // 如果是func label不用管 直接删了就行
        else {
          ssabb->SetLabel("unamed");
          new BasicBlockValue("unamed", ssabb);
        }
        bb->ir_list_.erase(bb->ir_list_.begin());
      } else {
        ssabb->SetLabel("unamed");
        new BasicBlockValue("unamed", ssabb);
      }
    }

    // for every ir basicblock
    for (auto bb : func->bb_list_) {
      std::cout << "---Process BB:" << std::endl;
      auto ssabb = bb_map[bb];
      MyAssert(nullptr != ssabb);

      // for every ir
      for (auto ir_iter = bb->ir_list_.begin(); ir_iter != bb->ir_list_.end(); ++ir_iter) {
        auto ir = **ir_iter;
        // std::cout << "----Process IR:" << std::endl;
        ir.PrintIR(std::cout);
        switch (ir.op_) {
          // NOTE: assign和param可能传递i32 pointer 其他传递i32或者void arraytype只在alloca中不会传递
          // 从数组中取元素值只会出现在assign offset ir中 给数组元素赋值只会出现在assign ir中
          case ir::IR::OpKind::ADD:
          case ir::IR::OpKind::SUB:
          case ir::IR::OpKind::MUL:
          case ir::IR::OpKind::DIV:
          case ir::IR::OpKind::MOD: {
            auto lhs = ResolveOpn2Value(&(ir.opn1_), ssabb);
            auto rhs = ResolveOpn2Value(&(ir.opn2_), ssabb);
            auto res_comp_name = ir.res_.GetCompName();
            auto res =
                new BinaryOperator(new Type(Type::IntegerTyID), static_cast<BinaryOperator::OpKind>(GetOpKind(ir.op_)),
                                   res_comp_name, lhs, rhs, ssabb);
            work_map[res_comp_name] = res;
            break;
          }
          case ir::IR::OpKind::NOT:
          case ir::IR::OpKind::NEG: {
            auto lhs = ResolveOpn2Value(&(ir.opn1_), ssabb);
            auto res_comp_name = ir.res_.GetCompName();
            auto res =
                new UnaryOperator(new Type(Type::IntegerTyID), static_cast<UnaryOperator::OpKind>(GetOpKind(ir.op_)),
                                  res_comp_name, lhs, ssabb);
            work_map[res_comp_name] = res;
            break;
          }
          case ir::IR::OpKind::LABEL: {  // label ir已经全被删了 执行流不会到这里
            MyAssert(0);
            break;
          }
          case ir::IR::OpKind::PARAM: {  // param ir全部跳过 等call的时候处理
            break;
          }
          case ir::IR::OpKind::CALL: {
            break;
          }
          case ir::IR::OpKind::RET: {
            if (ir.opn1_.type_ != ir::Opn::Type::Null) {  // return int;
              auto ret_val = ResolveOpn2Value(&(ir.opn1_), ssabb);
              new ReturnInst(ret_val, ssabb);
            } else {  // return void;
              new ReturnInst(ssabb);
            }
            break;
          }
          case ir::IR::OpKind::GOTO: {
            auto bb_val = dynamic_cast<BasicBlockValue*>(ResolveOpn2Value(&(ir.opn1_), ssabb));
            MyAssert(nullptr != bb_val);
            new BranchInst(bb_val, ssabb);
            break;
          }
          case ir::IR::OpKind::ASSIGN: {
            auto val = ResolveOpn2Value(&(ir.opn1_), ssabb);
            auto res_comp_name = ir.res_.GetCompName();
            Value* res = nullptr;
            if (ir.res_.type_ != ir::Opn::Type::Array) {  // NOTE: consider array address
              val->GetType();
              res = new MovInst(val->GetType(), res_comp_name, val, ssabb);
            } else {
              auto arr_base_ptr = ResolveOpn2Value(&(ir.res_), ssabb);
              MyAssert(nullptr != dynamic_cast<PointerType*>(arr_base_ptr->GetType()));
              auto offset = ResolveOpn2Value(ir.res_.offset_, ssabb);
              res = new StoreInst(new Type(Type::IntegerTyID), res_comp_name, arr_base_ptr, offset, ssabb);
            }
            work_map[res_comp_name] = res;
            break;
          }
          case ir::IR::OpKind::ASSIGN_OFFSET: {
            MyAssert(ir.opn1_.type_ == ir::Opn::Type::Array);
            auto arr_base_ptr = ResolveOpn2Value(&(ir.opn1_), ssabb);
            MyAssert(nullptr != dynamic_cast<PointerType*>(arr_base_ptr->GetType()));
            auto offset = ResolveOpn2Value(ir.opn1_.offset_, ssabb);
            auto res_comp_name = ir.res_.GetCompName();
            auto res = new LoadInst(new Type(Type::IntegerTyID), res_comp_name, arr_base_ptr, offset, ssabb);
            work_map[res_comp_name] = res;
            break;
          }
          case ir::IR::OpKind::JEQ:
          case ir::IR::OpKind::JNE:
          case ir::IR::OpKind::JLT:
          case ir::IR::OpKind::JLE:
          case ir::IR::OpKind::JGT:
          case ir::IR::OpKind::JGE: {
            auto lhs = ResolveOpn2Value(&(ir.opn1_), ssabb);
            auto rhs = ResolveOpn2Value(&(ir.opn2_), ssabb);
            auto bb_val = dynamic_cast<BasicBlockValue*>(ResolveOpn2Value(&(ir.res_), ssabb));
            MyAssert(nullptr != bb_val);
            new BranchInst(static_cast<BranchInst::Cond>(GetOpKind(ir.op_)), lhs, rhs, bb_val, ssabb);
            break;
          }
          case ir::IR::OpKind::VOID: {
            MyAssert(0);
            break;
          }
          case ir::IR::OpKind::PHI: {
            auto res_comp_name = ir.res_.GetCompName();
            auto phi_inst = new PhiInst(new Type(Type::IntegerTyID), res_comp_name, ssabb);
            for (int i = 0; i < ir.phi_args_.size(); ++i) {
              auto arg_val = ResolveOpn2Value(&ir.phi_args_[i], ssabb);
              MyAssert(nullptr != arg_val);
              MyAssert(bb_map.find(bb->pred_[i]) != bb_map.end());
              std::cout << i << "label:" << bb_map[bb->pred_[i]]->GetLabel() << std::endl;
              MyAssert(bb_map[bb->pred_[i]]->GetValue() != nullptr);
              phi_inst->AddParam(arg_val, bb_map[bb->pred_[i]]->GetValue());
            }
            break;
          }
          case ir::IR::OpKind::ALLOCA: {
            // alloca opn - imm : alloca quad-ir only for array. regard opn as one dimensional array
            MyAssert(ir.res_.type_ == ir::Opn::Type::Imm);
            auto type = new ArrayType(ir.res_.imm_num_);
            new AllocaInst(type, ssabb);

            // create a value which owns i32 pointer type
            auto ptr_type = new PointerType(new Type(Type::IntegerTyID));
            const auto& array_name = ir.opn1_.GetCompName();
            work_map[array_name] = new Value(ptr_type, array_name);
            break;
          }
          default: {
            MyAssert(0);
            break;
          }
        }  // end of ir_op switch
      }    // end of ir for loop
    }      // end of ir basicblock loop
  }        // end of ir function loop

  return ssamodule;
}

void ConvertSSA::Rename(IRBasicBlock* bb) {
  // std::cout << "rename" << std::endl;
  std::unordered_map<std::string, int> deftimes;
  // 对bb中每条ir的使用和定值var rename
  for (auto ir : bb->ir_list_) {
    // 不考虑对数组的定值和使用
    auto [def, use] = GetDefUsePtr(ir, false);
    auto [defvar, usevar] = GetDefUse(ir, false);
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
        this->count_[defvar[i]] = 0;
      }
      int cnt = ++this->count_[defvar[i]];
      this->stack_[defvar[i]].push(cnt);
      def[i]->ssa_id_ = cnt;
    }
  }  // end of ir for-loop
  // 填写bb的succ中的phi ir
  // std::cout << "fill phi" << std::endl;
  for (auto succ : bb->succ_) {
    int order = std::distance(succ->pred_.begin(), std::find(succ->pred_.begin(), succ->pred_.end(), bb));
    MyAssert(order < succ->pred_.size());
    for (auto phi_ir : succ->ir_list_) {
      if (phi_ir->op_ == IR::OpKind::LABEL) continue;
      if (phi_ir->op_ != IR::OpKind::PHI) break;
      phi_ir->phi_args_[order] = phi_ir->res_;
      const auto&& varname = phi_ir->res_.GetCompName();
      // std::cout << varname << std::endl;
      if (this->stack_.find(varname) == this->stack_.end()) {
        this->count_[varname] = 0;
        this->stack_[varname].push(0);
      }
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

Opn CompName2Opn(const std::string& name) {
  auto scope_id = std::stoi(name.substr(name.rfind('.') + 1));
  MyAssert(scope_id >= 0);
  return Opn{Opn::Type::Var, name.substr(0, name.find('.')), scope_id};
}

void ConvertSSA::InsertPhiIR(IRFunction* f) {
  std::cout << "insert phi" << std::endl;
  std::unordered_map<std::string, std::unordered_set<IRBasicBlock*>> defsites;  // 变量被定值的基本块集合
  std::unordered_map<IRBasicBlock*, std::unordered_set<std::string>> phi_vars;  // 一个基本块内拥有phi函数的变量
  // compute def_use without array
  GetDefUse4Func(f, false);
  // fill defsites
  for (auto bb : f->bb_list_) {
    for (auto& def : bb->def_) {
      defsites[def].insert(bb);
    }
  }
  IRLivenessAnalysis::Run4Func(f);
  for (auto& [var, defbbs] : defsites) {
    std::cout << "var:" << var << "在这些bb中def:" << std::endl;
    while (!defbbs.empty()) {
      auto bb = *defbbs.begin();
      std::cout << bb->IndexInFunc() << std::endl;
      defbbs.erase(defbbs.begin());
      for (auto df : bb->df_) {  // 对于变量定值所在bb的每一个df都要加一个phi函数
        // NOTE: 如果var并不入口活跃就不用加入phi函数
        if (df->livein_.find(var) == df->livein_.end()) continue;
        if (phi_vars.find(df) == phi_vars.end() || phi_vars[df].find(var) == phi_vars[df].end()) {
          phi_vars[df].insert(var);
          auto&& res = CompName2Opn(var);
          // NOTE: phi结点的scope id可能并不正确
          auto ir_list_it = df->ir_list_.begin();
          while ((*ir_list_it)->op_ == IR::OpKind::LABEL) ++ir_list_it;
          df->ir_list_.insert(ir_list_it, new IR{IR::OpKind::PHI, res, static_cast<int>(df->pred_.size())});
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
