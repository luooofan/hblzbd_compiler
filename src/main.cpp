#include "../include/ast.h"
#include "../include/ir.h"
#include "parser.hpp"
ast::Root *ast_root;  // the root node of final AST
extern int yyparse();
extern int yylex_destroy();
extern void yyset_lineno(int _line_number);
extern void PrintFuncTable();
extern void PrintSymbolTables();

int main(int argc, char **argv) {
  char *in = nullptr;

  for (int i = 1; i < argc; ++i) {
    if (nullptr != argv[i]) in = argv[i];
  }

  if (nullptr != in) {
    freopen(in, "r", stdin);
  }

  yyset_lineno(1);
  yyparse();  // if success, ast_root is valid
  if (nullptr != ast_root) {
    ast_root->PrintNode(0,std::cout);
    std::cout << "\nGenerate IR:" << std::endl;
    ast_root->GenerateIR();
    PrintFuncTable();
    PrintSymbolTables();
    std::cout << "IRList:" << std::endl;
    for (auto &ir : gIRList) {
      ir.PrintIR();
    }
  }

  yylex_destroy();
  delete ast_root;

  return 0;
}