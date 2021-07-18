#include <cstring>
#include <fstream>

#include "../include/Pass/pass_manager.h"
#include "../include/allocate_register.h"
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

  std::cout << "Start Parser()" << std::endl;
  yyset_lineno(1);
  yyparse();  // if success, ast_root is valid

  if (nullptr != ast_root) {
    std::cout << "PrintNode()" << std::endl;
    ast_root->PrintNode(0, std::cout);

    std::cout << "Generate IR:" << std::endl;
    ir::ContextInfo ctx;
    ast_root->GenerateIR(ctx);
    ir::PrintFuncTable();
    ir::PrintScopes();

    std::cout << "IRList:" << std::endl;
    for (auto &ir : ir::gIRList) {
      ir.PrintIR();
    }

    IRModule *ir_module = ConstructModule();
    ir_module->EmitCode(std::cout);

    // PassManager passman(ir_module);
    // passman.AddPass<>(false);
    // passman.run(false, std::cout);

    ArmModule *arm_module = GenerateArm(ir_module);
    arm_module->EmitCode(std::cout);

    allocate_register(arm_module);

    std::ofstream outfile;
    int len = strlen(in);
    std::string file_name = std::string(in, in + len - 1);

    // file_name[file_name.size() - 1] = '\0';
    // outfile.open("./arm_code.s");
    std::cout << file_name << std::endl;
    outfile.open(file_name);
    std::cout << "EmitCode Begin" << std::endl;
    arm_module->EmitCode(outfile);
    std::cout << "EmitCode End" << std::endl;
    outfile.close();
  }

  yylex_destroy();

  delete ast_root;

  return 0;
}