#include <unordered_set>

#include "../ir_struct.h"
#include "pass_manager.h"
using namespace ir;

class ConvertSSA : public Transform {
 public:
  ConvertSSA(Module** m) : Transform(m) {}
  void Run();
  void InsertPhiIR(IRFunction* f);
};