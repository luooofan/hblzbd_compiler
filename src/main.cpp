#include <unistd.h>

#include <cstring>
#include <fstream>

#include "../include/Pass/loop_unroll.h"
#include "../include/Pass/loop.h"
#include "../include/Pass/allocate_register.h"
#include "../include/Pass/arm_if_to_cond.h"
#include "../include/Pass/arm_liveness_analysis.h"
#include "../include/Pass/arm_offset_fixup.h"
#include "../include/Pass/convert_ssa.h"
#include "../include/Pass/dead_code_eliminate.h"
#include "../include/Pass/dominant.h"
#include "../include/Pass/generate_arm_from_ssa.h"
#include "../include/Pass/global_value_numbering.h"
#include "../include/Pass/loop.h"
#include "../include/Pass/pass_manager.h"
#include "../include/Pass/simplify_armcode.h"
#include "../include/Pass/simplify_cfg.h"
#include "../include/Pass/ssa_simple_optimize.h"
#include "../include/arm.h"
#include "../include/arm_struct.h"
#include "../include/ast.h"
#include "../include/ir.h"
#include "../include/ir_struct.h"
#include "../include/ssa.h"
#include "../include/ssa_struct.h"
#include "parser.hpp"
ast::Root *ast_root;  // the root node of final AST
extern int yyparse();
extern int yylex_destroy();
extern void yyset_lineno(int _line_number);

// #define DEBUG_PROCESS

bool AST_LOG = false;
bool IRLIST_LOG = false;
bool PASS_LOG = false;

#define ASSERT_ENABLE
#include "../include/myassert.h"

int main(int argc, char **argv) {
  // MyAssert(0);
  bool opt = false, print_usage = false;
  char *src = nullptr, *output = nullptr, *log_file = nullptr;
  // parse command line options and check
  for (int ch; (ch = getopt(argc, argv, "Sl:o:O:h")) != -1;) {
    switch (ch) {
      case 'S':
        // do nothing
        break;
      case 'l':
        log_file = strdup(optarg);
        break;
      case 'o':
        output = strdup(optarg);
        break;
      case 'O':
        opt = atoi(optarg) > 0;
        break;
      case 'h':
        print_usage = true;
        break;
      default:
        break;
    }
  }

  if (optind <= argc) {
    src = argv[optind];
  }

  if (src == nullptr || print_usage) {
    std::cerr << "Usage: " << argv[0] << " [-S] [-l log_file] [-o output_file] [-O level] input_file" << std::endl;
    return 1;
  }

  if (nullptr != src) {
    freopen(src, "r", stdin);
  }
  std::ofstream logfile;
  if (nullptr != log_file) {
    logfile.open(log_file);
  }

#ifdef DEBUG_PROCESS
  std::cout << "Parser Start:" << std::endl;
#endif
  yyset_lineno(1);
  yyparse();  // if success, ast_root is valid
  // MyAssert(0);
#ifdef DEBUG_PROCESS
  std::cout << "Parser End." << std::endl;
#endif
  yylex_destroy();

  MyAssert(nullptr != ast_root);
  if (logfile.is_open() && AST_LOG) {
    logfile << "AST:" << std::endl;
    ast_root->PrintNode(0, logfile);
  }

#ifdef DEBUG_PROCESS
  std::cout << "Generate IR Start:" << std::endl;
#endif
  ir::ContextInfo ctx;
  ast_root->GenerateIR(ctx);
#ifdef DEBUG_PROCESS
  std::cout << "Generate IR End." << std::endl;
#endif
  // MyAssert(0);
  delete ast_root;

  if (logfile.is_open() && IRLIST_LOG) {
    ir::PrintFuncTable(logfile);
    ir::PrintScopes(logfile);
    logfile << "IRList:" << std::endl;
    for (auto &ir : ir::gIRList) {
      ir.PrintIR(logfile);
    }
  }

  // it represents IRModule before GenerateArm Pass and ArmModule after GenerateArm Pass.
  // the source space will be released when running GenerateArm Pass.
  Module *module_ptr = static_cast<Module *>(ConstructModule(std::string(src)));
  // module_ptr->EmitCode(std::cout);
  Module **const module_ptr_addr = &module_ptr;

#ifdef DEBUG_PROCESS
  std::cout << "Passes Start:" << std::endl;
#endif
  PassManager pm(module_ptr_addr);

  // ==================Add Quad-Pass Below==================
  pm.AddPass<LoopUnroll>(false);
  pm.AddPass<InvariantExtrapolation>(false);
  // ==================Add Quad-Pass Above==================
  pm.AddPass<SimplifyCFG>(false);  // necessary
  pm.AddPass<ComputeDominance>(false);
  pm.AddPass<ConvertSSA>(true);
  // ==================Add SSA-Pass Below==================
  pm.AddPass<DeadCodeEliminate>(false);
  pm.AddPass<SimpleOptimize>(false);
  pm.AddPass<SimpleOptimize>(false);
  pm.AddPass<DeadCodeEliminate>(false);
  pm.AddPass<GlobalValueNumbering>(true);  // actually redundant common expression eliminate
  // ==================Add SSA-Pass Above==================
  pm.AddPass<GenerateArmFromSSA>(true);  // define macro control MUL_TO_SHIFT optimize
  // ==================Add Arm(vreg)-Pass Below==================
  pm.AddPass<SimplifyArm>(true);
  pm.AddPass<IfToCond>(true);
  // ==================Add Arm(vreg)-Pass Above==================
  pm.AddPass<RegAlloc>(false);
  pm.AddPass<SPOffsetFixup>(true);
  // ==================Add Arm-Pass Below==================
  pm.AddPass<SimplifyArm>(false);
  pm.AddPass<IfToCond>(true);
  // ==================Add Arm-Pass Above==================
  if (logfile.is_open()) {
    pm.Run(PASS_LOG, logfile);
  } else {
    pm.Run();
  }
  // exit(0);
#ifdef DEBUG_PROCESS
  std::cout << "Passes End." << std::endl;
#endif
  MyAssert(typeid(*module_ptr) == typeid(ArmModule));

#ifdef DEBUG_PROCESS
  std::cout << "Emit Start:" << std::endl;
#endif
  {
    std::ofstream outfile;
    std::string file_name = "";
    if (nullptr == output) {
      file_name = std::string(src, src + strlen(src) - 1);
    } else {
      file_name = std::string(output);
    }
    outfile.open(file_name);
    if (logfile.is_open()) {
      logfile << "Emitting code to file: " << file_name << std::endl;
    }
    module_ptr->EmitCode(outfile);
    MyAssert(outfile.is_open());
    outfile << std::endl;
    if (logfile.is_open()) {
      logfile << "EmitCode finish." << std::endl;
    }
    outfile.close();
  }
#ifdef DEBUG_PROCESS
  std::cout << "Emit End." << std::endl;
#endif

  if (logfile.is_open()) {
    logfile.close();
  }

  // release the arm module space.
  delete module_ptr;
  module_ptr = nullptr;

  return 0;
}

#undef ASSERT_ENABLE