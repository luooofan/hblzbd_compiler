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
  char *in = nullptr;

  for (int i = 1; i < argc; ++i) {
    if (nullptr != argv[i]) in = argv[i];
  }

  if (nullptr != in) {
    freopen(in, "r", stdin);
  }

  std::cout << "Start Parser:" << std::endl;
  yyset_lineno(1);
  yyparse();  // if success, ast_root is valid

  if (nullptr == ast_root) {
  }
  std::cout << "PrintNode:" << std::endl;
  ast_root->PrintNode(0, std::cout);

  std::cout << "Generate IR:" << std::endl;
  ir::ContextInfo ctx;
  ast_root->GenerateIR(ctx);

  yylex_destroy();
  delete ast_root;

  ir::PrintFuncTable();
  ir::PrintScopes();

  std::cout << "IRList:" << std::endl;
  for (auto &ir : ir::gIRList) {
    ir.PrintIR();
  }

  // it represents IRModule before GenerateArm Pass
  // and ArmModule after GenerateArm Pass.
  // the source space will be released when running GenerateArm Pass.
  Module *module_ptr = static_cast<Module *>(ConstructModule(std::string(in)));
  module_ptr->EmitCode(std::cout);
  Module **module_ptr_addr = &module_ptr;

  PassManager pm(module_ptr_addr);
  pm.AddPass<GenerateArm>(false);
  pm.AddPass<RegAlloc>(false);
  pm.Run(true, std::cout);

  assert(typeid(*module_ptr) == typeid(ArmModule));

  {
    std::ofstream outfile;
    int len = strlen(in);
    std::string file_name = std::string(in, in + len - 1);
    outfile.open(file_name);
    std::cout << "Emitting code to file: " << file_name << std::endl;
    module_ptr->EmitCode(outfile);
    std::cout << "EmitCode finish." << std::endl;
    outfile.close();
  }

  // release the arm module space.
  delete module_ptr;
  module_ptr = nullptr;

  return 0;
}