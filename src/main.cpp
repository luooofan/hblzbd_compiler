#include <fstream>

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

  yyset_lineno(1);
  std::cout << "Start Parser()" << std::endl;
  yyparse();  // if success, ast_root is valid
  if (nullptr != ast_root) {
    std::cout << "PrintNode()" << std::endl;
    ast_root->PrintNode(0, std::cout);
    std::cout << "\nGenerate IR:" << std::endl;
    ir::ContextInfo ctx;
    ast_root->GenerateIR(ctx);
    ir::PrintFuncTable();
    ir::PrintScopes();
    std::cout << "IRList:" << std::endl;
    for (auto &ir : ir::gIRList) {
      ir.PrintIR();
    }
    // ir::Module *ir_module = ir::ConstructModule();
    // arm::Module *arm_module = arm::GenerateAsm(ir_module);
    // std::ofstream outfile;
    // outfile.open("./arm_code.s");
    // std::cout << "EmitCode Begin" << std::endl;
    // arm_module->EmitCode(outfile);
    // std::cout << "EmitCode End" << std::endl;
    // outfile.close();
  }

  yylex_destroy();

  delete ast_root;

  return 0;
}