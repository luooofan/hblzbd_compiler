#include "../include/ir_struct.h"
using namespace ir;

IRModule* ConstructModule(const std::string& module_name) {
  // Construct BasicBlocks Functions and Module from gIRList

  // 基本块的首指令
  //  由于IR中使用了label 所有跳转语句都跳到某个label
  //  并且每个出现在四元式中的label都有跳转到该label的指令(不一定)
  //  所以首指令为每个label和每个跳转语句(call,ret,goto,jxx)的下一句
  //  (即便不一定为真正的首指令也可以接受)

  // TODO: 函数的第一个基本块需要和调用处的基本块链接吗
  // TODO: 假设函数之间的基本块是毫无关系的

  // 先遍历一遍IR 把所有func和有label的BB创建完成 创建两个map
  std::unordered_map<std::string, IRBasicBlock*> label2bb;
  std::unordered_map<std::string, IRFunction*> label2func;

  // add library function to label2func
  // not add them to module
  label2func.insert({"getint", new IRFunction("getint", 0, 0)});
  label2func.insert({"getch", new IRFunction("getch", 0, 0)});
  label2func.insert({"getarray", new IRFunction("getarray", 0, 0)});
  label2func.insert({"putint", new IRFunction("putint", 1, 0)});
  label2func.insert({"putch", new IRFunction("putch", 1, 0)});
  label2func.insert({"putarray", new IRFunction("putarray", 1, 0)});
  label2func.insert({"_sysy_starttime", new IRFunction("_sysy_starttime", 1, 0)});
  label2func.insert({"_sysy_stoptime", new IRFunction("_sysy_stoptime", 1, 0)});

  IRModule* module = new IRModule(module_name, gScopes[0]);

  for (int i = 0; i < gIRList.size(); ++i) {
    auto& ir = gIRList[i];
    if (ir.op_ == IR::OpKind::LABEL) {
      auto& label_name = ir.opn1_.name_;
      IRBasicBlock* bb = new IRBasicBlock(i);
      label2bb.insert({label_name, bb});
      if (ir.opn1_.type_ == Opn::Type::Func) {
        // is a function begin
        auto& func_item = (*ir::gFuncTable.find(label_name)).second;
        IRFunction* func = new IRFunction(label_name, func_item.shape_list_.size(), func_item.size_);
        func->bb_list_.push_back(bb);
        label2func.insert({label_name, func});
        module->func_list_.push_back(func);
      }
    }
  }
  // 此时module的所有func已经放好 func的第一个基本块已经放好
  bool is_leader = false;
  IRFunction* curr_func = nullptr;
  IRBasicBlock* pre_bb = nullptr;
  IRBasicBlock* curr_bb = nullptr;
  for (int i = 0; i < gIRList.size(); ++i) {
    auto& ir = gIRList[i];
    if (ir.op_ == IR::OpKind::LABEL) {
      is_leader = true;
    }
    if (is_leader) {
      // 当前ir是一条首指令 意味着上一个BB的结束和一个新的BB的开始
      pre_bb = curr_bb;
      if (nullptr != pre_bb) {
        pre_bb->end_ = i;
      }
      // 更新curr_bb curr_func
      if (ir.op_ == IR::OpKind::LABEL) {
        curr_bb = label2bb[ir.opn1_.name_];
        if (ir.opn1_.type_ == Opn::Type::Func) {
          curr_func = label2func[ir.opn1_.name_];
          is_leader = false;  // 下一条语句非首指令
          continue;
        }
      } else {
        // new BB
        curr_bb = new IRBasicBlock(i);
      }
      curr_func->bb_list_.push_back(curr_bb);
      // 维护前一个基本块的终点 维护两个基本块之间的关系
      if (nullptr != pre_bb) {
        // pre_bb->end_ = i;
        pre_bb->succ_.push_back(curr_bb);
        curr_bb->pred_.push_back(pre_bb);
      }
    }
    // 此时is_leader表示下一条ir是否为首指令
    // 维护当前基本块和跳转目标基本块之间的关系 维护is_leader
    switch (ir.op_) {
      case IR::OpKind::RET:
        is_leader = true;
        break;
      case IR::OpKind::CALL:
        // NOTE: 此时不维护调用双方两个基本块之间的关系
        curr_func->call_func_list_.push_back(label2func[ir.opn1_.name_]);
        is_leader = true;
        break;
      case IR::OpKind::GOTO: {
        auto succ_bb = label2bb[ir.opn1_.name_];
        curr_bb->succ_.push_back(succ_bb);
        succ_bb->pred_.push_back(curr_bb);
        is_leader = true;
        break;
      }
      case IR::OpKind::JEQ:
      case IR::OpKind::JNE:
      case IR::OpKind::JLT:
      case IR::OpKind::JLE:
      case IR::OpKind::JGT:
      case IR::OpKind::JGE: {
        auto succ_bb = label2bb[ir.res_.name_];
        curr_bb->succ_.push_back(succ_bb);
        succ_bb->pred_.push_back(curr_bb);
        is_leader = true;
        break;
      }
      default:
        is_leader = false;
        break;
    }
  }
  curr_bb->end_ = gIRList.size();

  return module;
}
