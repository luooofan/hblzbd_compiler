#include "../../include/Pass/reach_define.h"
#define ASSERT_ENABLE
#include <algorithm>

#include "../../include/myassert.h"

// #define PRINT_USE_DEF_CHAIN
// #define PRINT_SET

void print_map(std::unordered_map<std::string, std::unordered_set<ir::IR *>> umap) {
  for (auto iter : umap) {
    std::cout << iter.first << ": " << iter.second.size() << std::endl;
    for (auto ir : iter.second) {
      ir->PrintIR();
    }
  }
}

void print_use_def_chain(IRModule *m) {
  for (auto func : m->func_list_) {
    for (auto bb : func->bb_list_) {
      for (auto map_ir : bb->use_def_chains_) {
        for (auto map_opn : map_ir) {
          auto opn = map_opn.first;

          std::cout << "变量：" << opn->name_ + "_#" + std::to_string(opn->scope_id_) << "的使用-定值链" << std::endl;

          for (auto ir : map_opn.second) {
            ir->PrintIR();
          }
        }
        std::cout << "\n";
      }
    }
  }
}

void print_set(IRModule *m) {
  for (auto func : m->func_list_) {
    for (auto bb : func->bb_list_) {
      std::cout << "BasicBlock: " << bb->IndexInFunc() << std::endl;
      std::cout << "reach_in_: \n";
      for (auto ir : bb->reach_in_) {
        ir->PrintIR();
      }
      std::cout << "reach_out_: \n";
      for (auto ir : bb->reach_out_) {
        ir->PrintIR();
      }
      std::cout << "gen_: \n";
      for (auto ir : bb->gen_) {
        ir->PrintIR();
      }
      std::cout << "kill_: \n";
      for (auto ir : bb->kill_) {
        ir->PrintIR();
      }
    }
  }
}

void ReachDefine::Run() {
  auto m = dynamic_cast<IRModule *>(*m_);

  MyAssert(nullptr != m);

  for (auto func : m->func_list_) {
    for (auto bb : func->bb_list_) {
      for (auto ir : bb->ir_list_) {
        if (ir->res_.type_ == ir::Opn::Type::Var && ir->res_.scope_id_ != 0) {
          std::string var_name = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);
          if (0 == func->def_pre_var_.count(var_name)) {
            std::unordered_set<ir::IR *> temp_set;
            temp_set.insert(ir);
            func->def_pre_var_.emplace(var_name, temp_set);
          } else {
            func->def_pre_var_[var_name].insert(ir);
          }
        }
      }
      // print_map(func->def_pre_var_);
    }
  }

  for (auto func : m->func_list_) {
    // std::cout << "analysis func:" << func->name_ << std::endl;
    Run4Func(func);
  }

  // 计算每个程序点处的使用-定值链
  GetUseDefChain(m);

#ifdef PRINT_USE_DEF_CHAIN
  print_use_def_chain(m);
#endif

#ifdef PRINT_SET
  print_set(m);
#endif
}

bool isSameOpn(ir::Opn opn1, ir::Opn opn2) {
  if (opn1.type_ == opn2.type_) {
    if (ir::Opn::Type::Var == opn1.type_) {
      // std::cout << "opn1:" << opn1.name_ + "_#" + std::to_string(opn1.scope_id_) + "\n";
      // std::cout << "opn2:" << opn2.name_ + "_#" + std::to_string(opn2.scope_id_) + "\n";
      return opn1.name_ == opn2.name_ && opn1.scope_id_ == opn2.scope_id_;
    }
    return false;
  }

  return false;
}

ir::IR *finddef(IRBasicBlock *bb, ir::IR *ir, ir::Opn *opn) {
  auto it = find(bb->ir_list_.begin(), bb->ir_list_.end(), ir);

  for (auto iter = it - 1; iter != bb->ir_list_.begin() - 1; --iter) {
    auto def = *iter;

    if (isSameOpn(def->res_, *opn)) {
      // printf("/*************************\n");
      return def;
    }
  }

  return nullptr;
}

void ReachDefine::GetUseDefChain(IRModule *m) {
  for (auto func : m->func_list_) {
    for (auto bb : func->bb_list_) {
      bb->use_def_chains_.clear();
      for (auto ir : bb->ir_list_) {
        std::unordered_map<ir::Opn *, std::unordered_set<ir::IR *>> map_this_ir;
        auto process_opn = [&bb, &map_this_ir](ir::IR *ir, ir::Opn *opn) {
          if (opn->type_ != ir::Opn::Type::Var) return;
          if (opn->scope_id_ == 0) return;
          ir::IR *def_opn = finddef(bb, ir, opn);
          std::unordered_set<ir::IR *> opn_set;

          if (nullptr != def_opn) {
            opn_set.insert(def_opn);
            map_this_ir.emplace(opn, opn_set);
          } else {
            for (auto in : bb->reach_in_) {
              if (isSameOpn(in->res_, *opn)) {
                opn_set.insert(in);
              }
            }
            map_this_ir.emplace(opn, opn_set);
          }
        };
        if (ir::IR::OpKind::ADD == ir->op_ || ir::IR::OpKind::SUB == ir->op_ || ir::IR::OpKind::MUL == ir->op_ ||
            ir::IR::OpKind::DIV == ir->op_ || ir::IR::OpKind::MOD == ir->op_) {
          map_this_ir.clear();
          process_opn(ir, &(ir->opn1_));
          process_opn(ir, &(ir->opn2_));

          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::NOT == ir->op_ || ir::IR::OpKind::NEG == ir->op_) {
          map_this_ir.clear();
          process_opn(ir, &(ir->opn1_));

          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::LABEL == ir->op_) {
          map_this_ir.clear();
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::PARAM == ir->op_) {
          map_this_ir.clear();
          process_opn(ir, &(ir->opn1_));

          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::CALL == ir->op_) {
          map_this_ir.clear();

          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::RET == ir->op_) {
          map_this_ir.clear();
          // TODO: 关于return语句返回立即数的处理
          if (ir->opn1_.type_ == ir::Opn::Type::Var || ir->opn1_.type_ == ir::Opn::Type::Array) {
            process_opn(ir, &(ir->opn1_));
          }
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::GOTO == ir->op_) {
          map_this_ir.clear();
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::ASSIGN == ir->op_) {
          map_this_ir.clear();
          process_opn(ir, &(ir->opn1_));
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::JEQ == ir->op_ || ir::IR::OpKind::JNE == ir->op_ || ir::IR::OpKind::JLT == ir->op_ ||
                   ir::IR::OpKind::JLE == ir->op_ || ir::IR::OpKind::JGT == ir->op_ || ir::IR::OpKind::JGE == ir->op_) {
          map_this_ir.clear();
          process_opn(ir, &(ir->opn1_));
          process_opn(ir, &(ir->opn2_));

          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::VOID == ir->op_) {
          map_this_ir.clear();
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::ASSIGN_OFFSET == ir->op_) {
          map_this_ir.clear();

          process_opn(ir, &(ir->opn1_));
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::PHI == ir->op_) {
          map_this_ir.clear();
          bb->use_def_chains_.push_back(map_this_ir);
          printf("我就不信我这个pass会有PHI\n");
        } else if (ir::IR::OpKind::ALLOCA == ir->op_) {
          map_this_ir.clear();
          bb->use_def_chains_.push_back(map_this_ir);
        } else if (ir::IR::OpKind::DECLARE == ir->op_) {
          map_this_ir.clear();
          bb->use_def_chains_.push_back(map_this_ir);
        } else {
          MyAssert(0);
        }
      }
    }
  }
}

void ReachDefine::Run4Func(IRFunction *f) {
  for (auto bb : f->bb_list_) {
    bb->gen_.clear();
    bb->kill_.clear();
    bb->reach_in_.clear();
    bb->reach_out_.clear();

    // 反向遍历bb的ir_list，对于得到的gen和kill
    // 如果kill中的元素不在bb的gen中，说明它未被后面的生成语句取代，所以才可以加入到bb的kill中
    // 而对于gen中的元素，如果被后面的kill杀死，则就不能加进去
    for (auto iter = bb->ir_list_.rbegin(); iter != bb->ir_list_.rend(); ++iter) {
      auto ir = *iter;

      auto [gen, kill] = GetGenKill(ir, f);

      for (auto k : kill) {
        if (bb->gen_.find(k) == bb->gen_.end()) {
          bb->kill_.insert(k);
        }
      }

      for (auto g : gen) {
        if (bb->kill_.find(g) == bb->kill_.end()) {
          bb->gen_.insert(g);
        }
      }

      bb->reach_out_ = bb->gen_;
    }
  }

  bool changed = true;
  while (changed) {
    changed = false;
    for (auto bb : f->bb_list_) {
      std::unordered_set<ir::IR *> new_in;

      for (auto pred : bb->pred_) {
        new_in.insert(pred->reach_out_.begin(), pred->reach_out_.end());
      }

      if (new_in != bb->reach_in_) {
        changed = true;
        bb->reach_in_ = new_in;

        for (auto s : bb->reach_in_) {
          if (bb->kill_.find(s) == bb->kill_.end()) {
            bb->reach_out_.insert(s);
          }
        }
        for (auto g : bb->gen_) {
          bb->reach_out_.insert(g);
        }
      }
    }
  }
}

std::pair<std::unordered_set<ir::IR *>, std::unordered_set<ir::IR *>> ReachDefine::GetGenKill(ir::IR *ir,
                                                                                              IRFunction *func) {
  std::unordered_set<ir::IR *> gen;
  std::unordered_set<ir::IR *> kill;

  if (ir->res_.scope_id_ == 0) return {gen, kill};

  if (ir::IR::OpKind::ADD == ir->op_ || ir::IR::OpKind::SUB == ir->op_ || ir::IR::OpKind::MUL == ir->op_ ||
      ir::IR::OpKind::DIV == ir->op_ || ir::IR::OpKind::MOD == ir->op_) {
    gen.insert(ir);

    std::string res_str = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);
    for (auto m : func->def_pre_var_[res_str]) {
      if (ir != m) {
        kill.insert(m);
      }
    }
  } else if (ir::IR::OpKind::NOT == ir->op_ || ir::IR::OpKind::NEG == ir->op_) {
    gen.insert(ir);

    std::string res_str = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);

    for (auto m : func->def_pre_var_[res_str]) {
      if (ir != m) {
        kill.insert(m);
      }
    }
  } else if (ir::IR::OpKind::LABEL == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::PARAM == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::CALL == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::RET == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::GOTO == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::ASSIGN == ir->op_) {
    gen.insert(ir);
    std::string res_str = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);

    for (auto m : func->def_pre_var_[res_str]) {
      if (ir != m) {
        kill.insert(m);
      }
    }
  } else if (ir::IR::OpKind::JEQ == ir->op_ || ir::IR::OpKind::JNE == ir->op_ || ir::IR::OpKind::JLT == ir->op_ ||
             ir::IR::OpKind::JLE == ir->op_ || ir::IR::OpKind::JGT == ir->op_ || ir::IR::OpKind::JGE == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::VOID == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::ASSIGN_OFFSET == ir->op_) {
    gen.insert(ir);
    std::string res_str = ir->res_.name_ + "_#" + std::to_string(ir->res_.scope_id_);

    for (auto m : func->def_pre_var_[res_str]) {
      if (ir != m) {
        kill.insert(m);
      }
    }
  } else if (ir::IR::OpKind::PHI == ir->op_) {
    printf("我就不信我这个pass会有PHI\n");
  } else if (ir::IR::OpKind::ALLOCA == ir->op_) {
    return {gen, kill};
  } else if (ir::IR::OpKind::DECLARE == ir->op_) {
    return {gen, kill};
  } else {
    MyAssert(0);
  }

  return {gen, kill};
}

#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif