#include "pass_manager.h"

class LoopUnroll:public Pass
{
public:
    int x;
    LoopUnroll(Module** m):Pass(m){}
    void Run();
};