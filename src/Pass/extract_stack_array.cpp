#include "../../include/Pass/extract_stack_array.h"

#include <vector>

#include "../../include/ssa.h"
#include "../../include/ssa_struct.h"
#define ASSERT_ENABLE
#include "../../include/myassert.h"

void ExtractStackArray::Run() {
  auto m = dynamic_cast<SSAModule *>(*(this->m_));
  MyAssert(nullptr != m);
  for (auto func : m->GetFuncList()) {
    for (auto bb : func->GetBBList()) {
      // 先找到alloca 即局部数组
      for (auto inst : bb->GetInstList()) {
        if (auto alloc = dynamic_cast<AllocaInst *>(inst)) {
          bool extract = true;
          int size = (dynamic_cast<ConstantInt *>(alloc->GetOperand(0)))->GetImm();
          std::vector<int> initval(size, 0);  // TODO
          std::unordered_set<StoreInst *> stores, stores_before_use;
          CallInst *memset = nullptr;
          for (auto use : alloc->GetUses()) {
            if (auto store_inst = dynamic_cast<StoreInst *>(use->Get())) {
              MyAssert(store_inst->GetNumOperands() == 3);
              if (auto data = dynamic_cast<ConstantInt *>(store_inst->GetOperand(0)),
                  offset = dynamic_cast<ConstantInt *>(store_inst->GetOperand(2));
                  data && offset) {
                initval[offset->GetImm() / 4] = data->GetImm();
                stores.insert(store_inst);
              } else {
                extract = false;
                break;
              }
            }
          }
          //
        }
      }
    }
  }
}