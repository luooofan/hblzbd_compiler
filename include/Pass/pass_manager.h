#ifndef __PASS_MANAGER_H__
#define __PASS_MANAGER_H__

#include <string>

#define DEBUG_PASSES_PROCESS

#include "../general_struct.h"

extern clock_t START_TIME, END_TIME;

class Pass {
 protected:
  Module** const m_;
  std::string name_;
  bool emit_ = false;

 public:
  Pass(Module** const m) : m_(m) {}

  virtual void Run() = 0;
  void SetName(std::string name) { name_ = name; }
  std::string GetName() { return name_; }
  void SetEmit(bool emit) { emit_ = emit; }
  bool IsEmit() { return emit_; }
};

class Analysis : public Pass {
 public:
  Analysis(Module** m) : Pass(m) {}
};

class Transform : public Pass {
 public:
  Transform(Module** m) : Pass(m) {}
};

class PassManager {
 private:
  std::vector<Pass*> passes_;
  Module** const m_;

 public:
  PassManager(Module** const m) : m_(m) {}

  template <typename PassTy>
  void AddPass(bool emit = false) {
    try {
      Pass* pass = new PassTy(m_);
      pass->SetName(typeid(PassTy).name());
      pass->SetEmit(emit);
      passes_.push_back(pass);
    } catch (...) {
      std::cerr << "ERROR class type:" << typeid(PassTy).name() << " be passed to AddPass();" << std::endl;
      exit(255);
    }
  }

  void Run(bool emit = false, std::ostream& out = std::cout) {
    int i = 1;
    for (auto pass : this->passes_) {
#ifdef DEBUG_PASSES_PROCESS
      std::cout << ">>>>>>>>>>>> Start pass " << pass->GetName() << " <<<<<<<<<<<<" << std::endl;
#endif
      pass->Run();

      END_TIME = clock();
      if ((END_TIME - START_TIME) / CLOCKS_PER_SEC > 60) exit(20 + i++);

      if (emit || pass->IsEmit()) {
        out << ">>>>>>>>>>>> After pass " << pass->GetName() << " <<<<<<<<<<<<" << std::endl;
        (*m_)->EmitCode(out);
      }
    }
  }
};

#endif