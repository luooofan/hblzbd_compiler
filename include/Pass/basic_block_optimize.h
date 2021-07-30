#ifndef __BASIC_BLOCK_OPTIMIZE__
#define __BASIC_BLOCK_OPTIMIZE__

#include "pass_manager.h"
#include "../DAG.h"

class BasicBlockOptimize : Transform{
public:
	BasicBlockOptimize(Module **m) : Transform(m) {}

	void Run();
	void BasicBlockSubExpElim();
	void DeadCodeEliminate();
	void DAG2TAC();
};

#endif