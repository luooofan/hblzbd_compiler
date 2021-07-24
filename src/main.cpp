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

int main(int argc, char **argv) {
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
  // std::cout << "Start Parser:" << std::endl;
  yyset_lineno(1);
  yyparse();  // if success, ast_root is valid
  yylex_destroy();

  // assert(nullptr != ast_root);
  if (nullptr == ast_root) {
    std::cout << "Assert: " << __FILE__ << " " << __LINE__ << std::endl;
    exit(1);
  }
  if (logfile.is_open()) {
    logfile << "PrintNode:" << std::endl;
    ast_root->PrintNode(0, logfile);
  }

  // std::cout << "Generate IR:" << std::endl;
  ir::ContextInfo ctx;
  ast_root->GenerateIR(ctx);

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

  PassManager pm(module_ptr_addr);
  pm.AddPass<GenerateArm>(false);
  pm.AddPass<RegAlloc>(false);
  if (logfile.is_open()) {
    pm.Run(true, logfile);
  } else {
    pm.Run();
  }

  // assert(typeid(*module_ptr) == typeid(ArmModule));
  if (typeid(*module_ptr) != typeid(ArmModule)) {
    std::cout << "Assert: " << __FILE__ << " " << __LINE__ << std::endl;
    exit(1);
  }
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

  if (logfile.is_open()) {
    logfile.close();
  }

  // release the arm module space.
  delete module_ptr;
  module_ptr = nullptr;

  return 0;
}