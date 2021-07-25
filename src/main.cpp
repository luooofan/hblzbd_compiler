#include <unistd.h>

#include <cstring>
#include <fstream>

#include "../include/Pass/allocate_register.h"
#include "../include/Pass/arm_liveness_analysis.h"
#include "../include/Pass/generate_arm.h"
#include "../include/Pass/pass_manager.h"
#include "../include/arm.h"
#include "../include/arm_struct.h"
#include "../include/ast.h"
#include "../include/ir.h"
#include "../include/ir_struct.h"
#include "parser.hpp"
ast::Root *ast_root;  // the root node of final AST
extern int yyparse();
extern int yylex_destroy();
extern void yyset_lineno(int _line_number);

// #define DEBUG_PROCESS

#define ASSERT_ENABLE
// assert(res);
#ifdef ASSERT_ENABLE
#define MyAssert(res)                                                    \
  if (!(res)) {                                                          \
    std::cerr << "Assert: " << __FILE__ << " " << __LINE__ << std::endl; \
    exit(255);                                                           \
  }
#else
#define MyAssert(res) ;
#endif

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
#ifdef DEBUG_PROCESS
  std::cout << "Parser End." << std::endl;
#endif
  yylex_destroy();

  MyAssert(nullptr != ast_root);
  if (logfile.is_open()) {
    logfile << "PrintNode:" << std::endl;
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
  delete ast_root;

  if (logfile.is_open()) {
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
  pm.AddPass<GenerateArm>(false);
  pm.AddPass<RegAlloc>(false);
  if (logfile.is_open()) {
    pm.Run(true, logfile);
  } else {
    pm.Run();
  }
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

#undef MyAssert
#ifdef ASSERT_ENABLE
#undef ASSERT_ENABLE
#endif