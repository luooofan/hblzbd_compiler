#include "../include/ast.h"

#include "../include/parser.hpp"

ast::Root *ast_root; // the root node of final AST
extern int yyparse();
extern int yylex_destroy();
extern void yyset_lineno(int _line_number);

int main(int argc, char **argv)
{
  char *in = nullptr;

  for (int i = 1; i < argc; ++i)
  {
    if (nullptr != argv[i])
      in = argv[i];
  }

  if (nullptr != in)
  {
    freopen(in, "r", stdin);
  }

  yyset_lineno(1);
  yyparse(); // if success, ast_root is valid
  if (nullptr != ast_root)
  {
    ast_root->PrintNode();
  }
  yylex_destroy();
  delete ast_root;

  return 0;
}