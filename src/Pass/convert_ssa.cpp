#include "../../include/Pass/convert_ssa.h"

#include "../../include/Pass/ir_liveness_analysis.h"
#include "../../include/ir_getdefuse.h"
#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"
#define ASSERT_ENABLE  // enable assert for this file.
#include "../../include/myassert.h"

// #define DEBUG_CONVERT_SSA_PROCESS
// #define DEBUG_CONSTRUCT_SSA_PROCESS

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
  // m->EmitCode();
  auto ssam = this->ConstructSSA(m);
  *(this->m_) = static_cast<Module*>(ssam);
  delete m;
}

Value* ConvertSSA::FindValueFromOpn(Opn* opn, bool is_global) {
  return this->FindValueFromCompName(opn->GetCompName(), is_global);
}

Value* ConvertSSA::FindValueFromCompName(const std::string& comp_name, bool is_global) {
  if (is_global) {
    auto it = glob_map.find(comp_name);
    if (it != glob_map.end()) return (*it).second;
  } else {
    auto it = work_map.find(comp_name);
    if (it != work_map.end()) return (*it).second;
  }
  return nullptr;
}

void ConvertSSA::ProcessResValue(const std::string& comp_name, Opn* opn, Value* val, SSABasicBlock* ssabb) {
  // 如果是全局量就加一条store语句 否则记录映射即可
  MyAssert(opn->type_ == Opn::Type::Var);
  if (0 == opn->scope_id_) {  // 全局变量需要store
    // maintain used_glob_var
    ssabb->GetFunction()->AddUsedGlobVar(dynamic_cast<GlobalVariable*>(val));

    new StoreInst(val, glob_map[comp_name], ssabb);
  } else {
    work_map[comp_name] = val;
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
  } else if (type == Opn::Type::Func || 0 == opn->scope_id_) {
    auto comp_name = opn->GetCompName();
    res = this->FindValueFromCompName(comp_name, true);
    MyAssert(nullptr != res && res->GetType()->IsPointer());

    if (type != Opn::Type::Func) {  // var & array
      // maintain used_glob_var
      ssabb->GetFunction()->AddUsedGlobVar(dynamic_cast<GlobalVariable*>(res));
    }

    // new load load出的结果不需要记录 全局量每次都要load
    if (type == Opn::Type::Var) {
      static int glob_temp_index = 1;
      res = new LoadInst(new Type(Type::IntegerTyID), "glotmp-" + std::to_string(glob_temp_index++), res, ssabb);
    }  // 全局数组直接返回i32 pointer

  } else {  // label标签 或 局部变量数组 局部变量可能未定义 局部数组只会返回i32 pointer类型的value
    auto comp_name = opn->GetCompName();
    res = this->FindValueFromCompName(comp_name, false);

    if (nullptr == res) {  // 未定义变量
      // std::cout << "Undefined variable:" << std::string(*opn) << std::endl;
      MyAssert(type == Opn::Type::Var);
      // new undefval
      res = new UndefVariable(new Type(Type::IntegerTyID), comp_name);
      work_map[comp_name] = res;
    }
  }
  MyAssert(nullptr != res);
  return res;
  // std::cout << "Resolve End." << std::endl;
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
      return BranchInst::Cond::LT;
      break;
    }
    case ir::IR::OpKind::JLE: {
      return BranchInst::Cond::LE;
      break;
    }
    case ir::IR::OpKind::JGT: {
      return BranchInst::Cond::GT;
      break;
    }
    case ir::IR::OpKind::JGE: {
      return BranchInst::Cond::GE;
      break;
    }
    default: {
      return -1;
      break;
    }
  }
  return -1;
}

void ConvertSSA::ProcessGlobalVariable(IRModule* irm, SSAModule* ssam) {
#ifdef DEBUG_CONSTRUCT_SSA_PROCESS
  std::cout << "Process All Global Variables Start:" << std::endl;
#endif

  for (auto& [symbol, symbol_item] : irm->global_scope_.symbol_table_) {
    // new glob. record size: int-4 array-4*n. as a i32 pointer.
    auto glob_var = new GlobalVariable(symbol, symbol_item.is_array_ ? symbol_item.width_[0] : 4);
    if (symbol_item.is_const_) {  // NOTE 此时一定是常量数组 原符号表项中的initval失效
      // glob_var->val_.swap(symbol_item.initval_);
      glob_var->val_ = symbol_item.initval_;
    }
    ssam->AddGlobalVariable(glob_var);
    glob_map[symbol] = glob_var;
  }

#ifdef DEBUG_CONSTRUCT_SSA_PROCESS
  std::cout << "Process All Global Variables Finish." << std::endl;
#endif
}

void ConvertSSA::AddBuiltInFunction() {
  glob_map["memset"] = new FunctionValue("memset", new SSAFunction("memset"));
  glob_map["getint"] = new FunctionValue("getint", new SSAFunction("getint"));
  glob_map["getch"] = new FunctionValue("getch", new SSAFunction("getch"));
  glob_map["getarray"] = new FunctionValue("getarray", new SSAFunction("getarray"));
  glob_map["putint"] = new FunctionValue("putint", new SSAFunction("putint"));
  glob_map["putch"] = new FunctionValue("putch", new SSAFunction("putch"));
  glob_map["putarray"] = new FunctionValue("putarray", new SSAFunction("putarray"));
  glob_map["_sysy_starttime"] = new FunctionValue("_sysy_starttime", new SSAFunction("_sysy_starttime"));
  glob_map["_sysy_stoptime"] = new FunctionValue("_sysy_stoptime", new SSAFunction("_sysy_stoptime"));
}

void ConvertSSA::GenerateSSABasicBlocks(IRFunction* func, SSAFunction* ssafunc,
                                        std::unordered_map<IRBasicBlock*, SSABasicBlock*>& bb_map) {
  // create armbb according to irbb
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
        auto label = "." + func->name_ + ".bb." + std::to_string(bb->IndexInFunc());
        ssabb->SetLabel(label);
        new BasicBlockValue(label, ssabb);
      }
      bb->ir_list_.erase(bb->ir_list_.begin());
    } else {
      auto label = "." + func->name_ + ".bb." + std::to_string(bb->IndexInFunc());
      ssabb->SetLabel(label);
      new BasicBlockValue(label, ssabb);
    }
  }
}

void ConvertSSA::ConstructSSA4BB(IRBasicBlock* bb, SSAFunction* ssafunc,
                                 std::unordered_map<IRBasicBlock*, SSABasicBlock*>& bb_map) {
  if (nullptr == bb) return;
#ifdef DEBUG_CONSTRUCT_SSA_PROCESS
  std::cout << "---Process BB:" << std::endl;
#endif

  auto ssabb = bb_map[bb];
  MyAssert(nullptr != ssabb);

  // for every ir
  for (auto ir_iter = bb->ir_list_.begin(); ir_iter != bb->ir_list_.end(); ++ir_iter) {
    auto ir = **ir_iter;
#ifdef DEBUG_CONSTRUCT_SSA_PROCESS
    // std::cout << "----Process IR:" << std::endl;
    ir.PrintIR(std::cout);
#endif
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
        ProcessResValue(res_comp_name, &(ir.res_), res, ssabb);
        break;
      }
      case ir::IR::OpKind::NOT:
      case ir::IR::OpKind::NEG: {
        auto lhs = ResolveOpn2Value(&(ir.opn1_), ssabb);
        auto res_comp_name = ir.res_.GetCompName();
        auto res = new UnaryOperator(new Type(Type::IntegerTyID), static_cast<UnaryOperator::OpKind>(GetOpKind(ir.op_)),
                                     res_comp_name, lhs, ssabb);
        ProcessResValue(res_comp_name, &(ir.res_), res, ssabb);
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
        auto func_value = dynamic_cast<FunctionValue*>(glob_map[ir.opn1_.GetCompName()]);
        MyAssert(nullptr != func_value);
        // maintain call_func
        ssafunc->AddCallFunc(func_value);

        CallInst* call_inst = nullptr;
        if (ir.res_.type_ == Opn::Type::Null) {
          call_inst = new CallInst(func_value, ssabb);
        } else {
          auto res_comp_name = ir.res_.GetCompName();
          call_inst = new CallInst(new Type(Type::IntegerTyID), res_comp_name, func_value, ssabb);
          ProcessResValue(res_comp_name, &(ir.res_), call_inst, ssabb);
        }

        // 添加参数
        MyAssert(ir.opn2_.type_ == Opn::Type::Imm);
        // NOTE: a[10][10][10]  func(a[10]) many_params1|2 chaos_token
        static int temp_for_array_address_compute = 1;
        int arg_num = ir.opn2_.imm_num_;
        for (int i = 1; i <= arg_num; ++i) {
          auto param_ir = **(ir_iter - i);
          auto val = ResolveOpn2Value(&(param_ir.opn1_), ssabb);
          if (param_ir.opn1_.type_ == Opn::Type::Array) {
            // 先把基址+偏移算到一个temp value里 再把temp value加到arg里 不必记录
            // param_ir.PrintIR(std::cout);
            auto offset_val = ResolveOpn2Value(param_ir.opn1_.offset_, ssabb) /*一定找得到 不会新加指令*/;
            auto final_address = new BinaryOperator(val->GetType(), BinaryOperator::ADD,
                                                    std::to_string(temp_for_array_address_compute++) + "-temp", val,
                                                    offset_val, call_inst);
            call_inst->AddArg(final_address);
          } else {
            call_inst->AddArg(val);
          }
        }
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
          if (0 == ir.res_.scope_id_) {
            auto glob_var = glob_map[res_comp_name];
            new StoreInst(val, glob_var, ssabb);

            // maintain used_glob_var
            ssafunc->AddUsedGlobVar(dynamic_cast<GlobalVariable*>(glob_var));
          } else {
            res = new MovInst(val->GetType(), res_comp_name, val, ssabb);
            work_map[res_comp_name] = res;
          }
        } else {
          auto arr_base_ptr = ResolveOpn2Value(&(ir.res_), ssabb);
          MyAssert(arr_base_ptr->GetType()->IsPointer());
          auto offset = ResolveOpn2Value(ir.res_.offset_, ssabb);
          new StoreInst(val, arr_base_ptr, offset, ssabb);
        }
        break;
      }
      case ir::IR::OpKind::ASSIGN_OFFSET: {
        MyAssert(ir.opn1_.type_ == ir::Opn::Type::Array);
        auto arr_base_ptr = ResolveOpn2Value(&(ir.opn1_), ssabb);
        MyAssert(arr_base_ptr->GetType()->IsPointer());
        auto offset = ResolveOpn2Value(ir.opn1_.offset_, ssabb);

        auto res_comp_name = ir.res_.GetCompName();
        auto res = new LoadInst(new Type(Type::IntegerTyID), res_comp_name, arr_base_ptr, offset, ssabb);
        ProcessResValue(res_comp_name, &(ir.res_), res, ssabb);
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
        MyAssert(0 != ir.res_.scope_id_);  // 不会出现全局变量的phi结点
        auto res_comp_name = ir.res_.GetCompName();
        auto phi_inst = new PhiInst(new Type(Type::IntegerTyID), res_comp_name, ssabb);
        // 此时有的变量还未定义 不能在现在填
        // for (int i = 0; i < ir.phi_args_.size(); ++i) {
        //   auto arg_val = ResolveOpn2Value(&ir.phi_args_[i], ssabb);
        //   phi_inst->AddParam(arg_val, bb_map[bb->pred_[i]]->GetValue());
        // }
        ProcessResValue(res_comp_name, &(ir.res_), phi_inst, ssabb);
        break;
      }
      case ir::IR::OpKind::ALLOCA: {
        // alloca opn - imm : alloca quad-ir only for array. regard opn as one dimensional array
        MyAssert(ir.res_.type_ == ir::Opn::Type::Imm);

        // create a value which owns i32 pointer type
        auto ptr_type = new Type(Type::PointerTyID);
        const auto& array_name = ir.opn1_.GetCompName();
        auto value = new Value(ptr_type, array_name);
        work_map[array_name] = value;

        // auto type = new ArrayType(ir.res_.imm_num_);
        // new AllocaInst(type, ssabb);
        new AllocaInst(new ConstantInt(ir.res_.imm_num_), value, ssabb);

        break;
      }
      case ir::IR::OpKind::DECLARE: {
        // 生成argument 记录映射 以防函数参数被认为是未定义变量
        auto arg_comp_name = ir.opn1_.GetCompName();
        if (ir.opn1_.type_ == Opn::Type::Var) {
          work_map[arg_comp_name] =
              new Argument(new Type(Type::IntegerTyID), arg_comp_name, ir.res_.imm_num_, ssafunc->GetValue());
        } else {
          work_map[arg_comp_name] =
              new Argument(new Type(Type::PointerTyID), arg_comp_name, ir.res_.imm_num_, ssafunc->GetValue());
        }
        break;
      }
      default: {
        MyAssert(0);
        break;
      }
    }  // end of ir_op switch
  }    // end of ir for loop
}

void ConvertSSA::FillPhiInst(IRFunction* irfunc, SSAFunction* ssafunc,
                             std::unordered_map<IRBasicBlock*, SSABasicBlock*>& bb_map) {
  // 遍历每个irbb 如果bb开头有phi结点 就填充对应的ssabb中的phi结点
  for (auto bb : irfunc->bb_list_) {
    auto ssabb = bb_map[bb];
    int i = 0;
    for (auto ssa_inst : ssabb->GetInstList()) {
      if (auto phi_inst = dynamic_cast<PhiInst*>(ssa_inst)) {
        auto ir = *(bb->ir_list_[i]);
        for (int i = 0; i < ir.phi_args_.size(); ++i) {
          auto arg_val = ResolveOpn2Value(&ir.phi_args_[i], ssabb);
          phi_inst->AddParam(arg_val, bb_map[bb->pred_[i]]->GetValue());
        }
        ++i;
      } else {
        break;
      }
    }
  }
}

SSAModule* ConvertSSA::ConstructSSA(IRModule* module) {
  // 由ir module直接构建ssa module 构建的过程中把ir中的label语句删掉 转化成了基本块的label
  // NOTE: 可能会出现一些空的bb 有些基本块是unnamed
  SSAModule* ssamodule = new SSAModule(module->global_scope_, module->name_);  // FIXME: terrible design
  std::unordered_map<IRFunction*, SSAFunction*> func_map;

  // process all global variable
  this->ProcessGlobalVariable(module, ssamodule);
  this->AddBuiltInFunction();

  // for every ir func
  for (auto func : module->func_list_) {
#ifdef DEBUG_CONSTRUCT_SSA_PROCESS
    std::cout << "--Process Func:" << func->name_ << std::endl;
#endif

    SSAFunction* ssafunc = new SSAFunction(func->name_, ssamodule);
    func_map.insert({func, ssafunc});
    glob_map[func->name_] = new FunctionValue(func->name_, ssafunc);

    work_map.clear();
    std::unordered_map<IRBasicBlock*, SSABasicBlock*> bb_map;
    this->GenerateSSABasicBlocks(func, ssafunc, bb_map);

    // for every ir basicblock
    for (auto bb : func->bb_list_) {
      ConstructSSA4BB(bb, ssafunc, bb_map);
    }

    FillPhiInst(func, ssafunc, bb_map);
  }  // end of ir function loop

  return ssamodule;
}

void ConvertSSA::Rename(IRBasicBlock* bb) {
#ifdef DEBUG_CONVERT_SSA_PROCESS
  std::cout << "rename for irbb: " << std::endl;
  // bb->EmitCode(std::cout);
#endif
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
      // NOTE here!!!
      const auto&& varname = phi_ir->res_.name_ + "." + std::to_string(phi_ir->res_.scope_id_);
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
#ifdef DEBUG_CONVERT_SSA_PROCESS
  std::cout << "insert phi" << std::endl;
#endif
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
  IRLivenessAnalysis::Run4Func(f);  // 用于插入phi node的时候判断是否活跃决定是否插入
  for (auto& [var, defbbs] : defsites) {
    while (!defbbs.empty()) {
      auto bb = *defbbs.begin();
#ifdef DEBUG_CONVERT_SSA_PROCESS
      std::cout << "var:" << var << "在这些bb中def:" << std::endl;
      std::cout << bb->IndexInFunc() << std::endl;
#endif
      defbbs.erase(defbbs.begin());
      for (auto df : bb->df_) {  // 对于变量定值所在bb的每一个df都要加一个phi函数
        // NOTE: 如果var并不入口活跃就不用加入phi函数
        if (df->livein_.find(var) == df->livein_.end()) continue;
        if (phi_vars.find(df) == phi_vars.end() || phi_vars[df].find(var) == phi_vars[df].end()) {
          phi_vars[df].insert(var);
          auto&& res = CompName2Opn(var);
          // NOTE: phi结点的scope id可能并不正确
          auto ir_list_it = df->ir_list_.begin();
          while (ir_list_it != df->ir_list_.end() && (*ir_list_it)->op_ == IR::OpKind::LABEL) ++ir_list_it;
          df->ir_list_.insert(ir_list_it, new IR{IR::OpKind::PHI, res, static_cast<int>(df->pred_.size())});
          if (df->def_.find(var) == df->def_.end()) {
            df->def_.insert(var);
            defbbs.insert(df);
          }
        }
      }  // end of df for
    }    // end of defbbs while
#ifdef DEBUG_CONVERT_SSA_PROCESS
    std::cout << "end." << std::endl;
#endif
  }  // end of defsites for
}

#undef ASSERT_ENABLE  // disable assert. this should be placed at the end of every file.
