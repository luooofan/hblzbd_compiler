#include "../../include/Pass/dead_code_eliminate.h"
#include "../../include/Pass/ir_liveness_analysis.h"

void DeadCodeEliminate::Run(){
  std::cout << "/*******************  pass: dead code eliminate  ***********************/" << std::endl;
  test_def_use();
}

// void DeadCodeEliminate::test_def_use(){
//   IRLivenessAnalysis var_live(m_);

//   var_live.Run();
//   auto m = dynamic_cast<IRModule *>(*(this->m_));

//   for(auto func : m->func_list_){
//     std::cout << "function: " << func->name_ << std::endl;
//     int bb_num = 0;
//     for(auto bb : func->bb_list_){
//       std::cout << "def:" << std::endl;
//       for(auto var : bb->def_){
//         var->printVar();
//       }
//       std::cout << "use:" << std::endl;
//       for(auto var : bb->use_){
//         var->printVar();
//       }    
//       std::cout << "livein:" << std::endl;
//       for(auto var : bb->livein_){
//         var->printVar();
//       }
//       std::cout << "liveout:" << std::endl;
//       for(auto var : bb->liveout_){
//         var->printVar();
//       }        
//     }
//   }
// }