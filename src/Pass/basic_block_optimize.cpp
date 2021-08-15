#include "../../include/Pass/basic_block_optimize.h"

void BasicBlockOptimize::Run(){
    DAG_analysis dag_analysis = DAG_analysis(m_);
    dag_analysis.Run();
}