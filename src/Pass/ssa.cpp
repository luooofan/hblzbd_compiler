#define ASSERT_ENABLE  // enable assert for this file. place this at the top of a file.
#include "../../include/Pass/ssa.h"

#include "../../include/myassert.h"

void ConvertSSA::Run() {
  auto m = dynamic_cast<IRModule*>(*(this->m_));
  MyAssert(nullptr != m);
}

void ConvertSSA::InsertPhiIR(IRFunction* f) {
  std::unordered_map<std::string, std::unordered_set<IRBasicBlock*>> defsites;
  for (auto bb : f->bb_list_) {
    // for bb->def_
    // defsites[].insert()
  }
  for (auto& [var, defbbs] : defsites) {
    while (!defbbs.empty()) {
      auto bb = *defbbs.begin();
      defbbs.erase(defbbs.begin());
      for (auto df : bb->df_) {
        // bb* vector<opn> insert
      }
    }
  }
}

#undef ASSERT_ENABLE  // disable assert. this should be placed at the end of every file.
