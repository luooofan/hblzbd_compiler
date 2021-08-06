#ifndef __IR_GETDEFUSE_H__
#define __IR_GETDEFUSE_H__
#include "./ir_struct.h"
using namespace ir;
std::pair<std::vector<Opn *>, std::vector<Opn *>> GetDefUsePtr(IR *ir, bool consider = true);
std::pair<std::vector<std::string>, std::vector<std::string>> GetDefUse(IR *ir, bool consider = true);
void GetDefUse4Func(IRFunction *f, bool consider = true);
#endif