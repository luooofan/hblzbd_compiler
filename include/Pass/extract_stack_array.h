#ifndef __EXTRACT_STACK_ARRAY_H__
#define __EXTRACT_STACK_ARRAY_H__

#include "./pass_manager.h"

class ExtractStackArray : public Transform {
 public:
  ExtractStackArray(Module** m) : Transform(m) {}
  void Run() override;
};

#endif