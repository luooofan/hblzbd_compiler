#ifndef __DEAD_CODE_ELIMINATE_H__
#define __DEAD_CODE_ELIMINATE_H__

#include "ir_liveness_analysis.h"

class DeadCodeEliminate : public Transform{
public:
  DeadCodeEliminate(Module **m) : Transform(m){}

  void Run();
  // void test_def_use();
};

#endif