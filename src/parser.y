%{

#include "../include/ast.h"
#include <cstdio>
#include <cstdlib>

#define YYERROR_VERBOSE true
#define YYDEBUG 1

extern ast::Root *ast_root; // the root node of final AST
extern int yydebug;
extern int yylex();
extern int yylex_destroy();
extern int yyget_lineno();

void yyerror(const char *s) {
     std::printf("Error(line: %d): %s\n", yyget_lineno(), s); 
     yylex_destroy(); 
     if (!yydebug) std::exit(1); 
}

%}

%union
{
    int token;
    std::string* string;
    ast::Root* root;
    ast::CompUnit* compile_unit;
    ast::DeclareStatement* declare;
    ast::Define* define;
    ast::Expression* expr;
    ast::ConditionExpression* cond_expr;
    ast::Number* number;
    ast::LeftValue* left_val;
    ast::Identifier* ident;
    ast::ArrayIdentifier* array_ident;
    ast::ArrayInitVal* array_initval;
    ast::Statement* statement;
    ast::Block* block;
    ast::FunctionDefine* funcdef;
    ast::FunctionCall* funccall;
    ast::FunctionFormalParameter* funcfparam;
    ast::FunctionFormalParameterList* funcfparams;
    ast::FunctionActualParameterList* funcaparams;
}

%token <token> ADD SUB NOT
%token <token> MUL DIV MOD 
%token <token> LT LE GT GE
%token <token> EQ NE 
%token <token> AND 
%token <token> OR

%token <token> ASSIGN
%token <token> LPAREN RPAREN LBRACE RBRACE LSQUARE RSQUARE
%token <token> COMMA SEMI

%token <token> IF ELSE WHILE FOR BREAK CONTINUE RETURN
%token <token> CONST INT VOID

%token <string> IDENT
%token <token> NUMBER

%type <token> BType EqOp RelOp AddOp MulOp UnaryOp

%type <declare> Decl ConstDecl VarDecl
%type <define> Def DefOne DefArray ConstDef ConstDefOne ConstDefArray

%type <expr> InitVal Exp AddExp MulExp UnaryExp PrimaryExp Cond LOrExp LAndExp EqExp RelExp
%type <ident> Ident
%type <array_ident> ArrayIdent
%type <left_val> LVal
%type <number> Number
%type <funccall> FuncCall

%type <statement> BlockItem Stmt AssignStmt IfStmt WhileStmt ReturnStmt
%type <block> Block BlockItemList

%type <funcfparam> FuncFParam FuncFParamOne FuncFParamArray
%type <funcfparams> FuncFParamList
%type <funcaparams> FuncRParamList
%type <funcdef> FuncDef

%type <root> Root CompUnit

%type <array_initval> InitValArray InitValArrayList

%start Root

%%

Root: CompUnit {
    $$=$1;
    ast_root=$$;
}
    ;

CompUnit: Decl {
    $$=new ast::Root();
    $$->compunit_list_.push_back(static_cast<ast::CompUnit*>($1));
}
    | CompUnit Decl {
        $$=$1;
        $$->compunit_list_.push_back(static_cast<ast::CompUnit*>($2));
    }
    | FuncDef {
        $$=new ast::Root();
        $$->compunit_list_.push_back(static_cast<ast::CompUnit*>($1));
    }
    | CompUnit FuncDef {
        $$=$1;
        $$->compunit_list_.push_back(static_cast<ast::CompUnit*>($2));
    }
    ;

Decl: ConstDecl SEMI
    | VarDecl SEMI
    ;

ConstDecl: CONST BType ConstDef {
    $$=new ast::DeclareStatement($2);
    $$->define_list_.push_back($3);
}
    | ConstDecl COMMA ConstDef {
        $$=$1;
        $$->define_list_.push_back($3);
    }
    ;

BType: INT
      ;

ConstDef: ConstDefOne
    | ConstDefArray
    ;

ConstDefOne: Ident ASSIGN InitVal {
    $$=static_cast<ast::Define*>(new ast::VariableDefineWithInit(*$1,*$3,true));
}
    ;

InitVal: AddExp
    ;

ConstDefArray: ArrayIdent ASSIGN InitValArray {
    $$=static_cast<ast::Define*>(new ast::ArrayDefineWithInit(*$1,*$3,true));
}
    ;

ArrayIdent: Ident LSQUARE Exp RSQUARE {
    $$=new ast::ArrayIdentifier(*$1);
    $$->shape_list_.push_back($3);
}
    | ArrayIdent LSQUARE Exp RSQUARE {
        $$=$1;
        $$->shape_list_.push_back($3);
    }
    ;

InitValArray: LBRACE RBRACE {
    $$=new ast::ArrayInitVal(false, nullptr);
}
    | LBRACE InitValArrayList RBRACE {
        $$=$2;
    }
    ;

// InitValArrayList是InitVal(AddExp)和InitValArray的任意组合
InitValArrayList: InitValArray {
    $$=new ast::ArrayInitVal(false, nullptr);
    $$->initval_list_.push_back($1);
}
    | InitValArrayList COMMA InitValArray {
        $$=$1;
        $$->initval_list_.push_back($3);
    }
    | InitVal {
        $$=new ast::ArrayInitVal(false, nullptr);
        $$->initval_list_.push_back(new ast::ArrayInitVal(true, $1));
    }
    | InitValArrayList COMMA InitVal {
        $$=$1;
        $$->initval_list_.push_back(new ast::ArrayInitVal(true,$3));
    }
    ;

VarDecl: BType Def {
    $$=new ast::DeclareStatement($1);
    $$->define_list_.push_back($2);
}
    | VarDecl COMMA Def {
        $$=$1;
        $$->define_list_.push_back($3);
    }
    ;

Def: DefOne
    | DefArray
    ;

DefOne: Ident {
    $$=static_cast<ast::Define*>(new ast::VariableDefine(*$1));
}
    | Ident ASSIGN InitVal {
        $$=static_cast<ast::Define*>(new ast::VariableDefineWithInit(*$1,*$3,false));
    }
    ;

DefArray: ArrayIdent {
    $$=static_cast<ast::Define*>(new ast::ArrayDefine(*$1));
}
    | ArrayIdent ASSIGN InitValArray {
        $$=static_cast<ast::Define*>(new ast::ArrayDefineWithInit(*$1,*$3,false));
    }
    ;

// ===============函数相关=================
FuncDef: VOID Ident LPAREN FuncFParamList RPAREN Block {
    $$=new ast::FunctionDefine($1,*$2,*$4,*$6);
}
    | VOID Ident LPAREN RPAREN Block {
        $$=new ast::FunctionDefine($1, *$2, *(new ast::FunctionFormalParameterList()), *$5);
    }
    | BType Ident LPAREN FuncFParamList RPAREN Block {
        $$=new ast::FunctionDefine($1,*$2,*$4,*$6);
    }
    | BType Ident LPAREN RPAREN Block {
        $$=new ast::FunctionDefine($1, *$2, *(new ast::FunctionFormalParameterList()), *$5);
    }

    ;

FuncFParamList: FuncFParam {
    $$=new ast::FunctionFormalParameterList();
    $$->arg_list_.push_back($1);
}
    | FuncFParamList COMMA FuncFParam {
        $$=$1;
        $$->arg_list_.push_back($3);
    }
    ;

FuncFParam: FuncFParamOne
    | FuncFParamArray
    ;

FuncFParamOne: BType Ident {
    $$=new ast::FunctionFormalParameter($1, *static_cast<ast::LeftValue*>($2));
}
    ;

FuncFParamArray: BType Ident LSQUARE RSQUARE {
    auto array_ident = new ast::ArrayIdentifier(*$2);
    // NOTE
    // array_ident->shape_list_.push_back(static_cast<ast::Expression*>(new ast::Number(1)));
    $$=new ast::FunctionFormalParameter($1,
                           static_cast<ast::LeftValue&>(*(array_ident))
                          );
}
    | FuncFParamArray LSQUARE Exp RSQUARE {
        $$=$1;
        dynamic_cast<ast::ArrayIdentifier&>($$->name_).shape_list_.push_back($3);
    }
    ;

Block: LBRACE RBRACE {
    $$=new ast::Block();
} 
    | LBRACE BlockItemList RBRACE {
        $$=$2;
    }
    ;

BlockItemList: BlockItem {
    $$=new ast::Block();
    $$->statement_list_.push_back($1);
}
    | BlockItemList BlockItem {
        $$=$1;
        $$->statement_list_.push_back($2);
    }
    ;

BlockItem: Stmt
    | Decl {
        $$=static_cast<ast::Statement*>($1);
    }
    ;

// ===============语句相关=================
Stmt: AssignStmt 
    | IfStmt
    | WhileStmt
    | ReturnStmt
    | Block {
        $$=static_cast<ast::Statement*>($1);
    }
    | BREAK SEMI {
        $$=static_cast<ast::Statement*>(new ast::BreakStatement());
    }
    | CONTINUE SEMI {
        $$=static_cast<ast::Statement*>(new ast::ContinueStatement());
    }
    | SEMI {
        $$=static_cast<ast::Statement*>(new ast::VoidStatement());
    }
    | Exp SEMI {
        $$=static_cast<ast::Statement*>(new ast::EvalStatement(*$1));
    }
    ; 

AssignStmt: LVal ASSIGN Exp SEMI {
    $$=static_cast<ast::Statement*>(new ast::AssignStatement(*$1,*$3));
}
    ;

IfStmt: IF LPAREN Cond RPAREN Stmt {
    $$=static_cast<ast::Statement*>(
        new ast::IfElseStatement(*(dynamic_cast<ast::ConditionExpression*>($3)),
                                 *$5,
                                 nullptr));
}
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        $$=static_cast<ast::Statement*>(
            new ast::IfElseStatement(*(dynamic_cast<ast::ConditionExpression*>($3)),
                                    *$5,
                                    $7)
        );
    }
    ;   

WhileStmt: WHILE LPAREN Cond RPAREN Stmt {
    $$=static_cast<ast::Statement*>(
        new ast::WhileStatement(*(dynamic_cast<ast::ConditionExpression*>($3)),*$5)
    );
}
    ;

ReturnStmt: RETURN SEMI {
    $$=static_cast<ast::Statement*>(new ast::ReturnStatement(nullptr));
}
    | RETURN Exp SEMI {
        $$=static_cast<ast::Statement*>(new ast::ReturnStatement($2));
    }
    ;

// ==============表达式相关=================
Exp: AddExp
    ;

Cond: LOrExp
    ;

LVal: Ident {
    $$=static_cast<ast::LeftValue*>($1);
}
    | ArrayIdent {
        $$=static_cast<ast::LeftValue*>($1);
    }
    ;

PrimaryExp: LPAREN Exp RPAREN {  // TODO: Exp or Cond
    $$=static_cast<ast::Expression*>($2);
}
    | Number {
        $$=static_cast<ast::Expression*>($1);
    }
    | LVal {
        $$=static_cast<ast::Expression*>($1);
    }
    ;

UnaryExp: PrimaryExp
    | UnaryOp UnaryExp {
        $$=static_cast<ast::Expression*>(new ast::UnaryExpression($1,*$2));
    }
    | FuncCall {
        $$=static_cast<ast::Expression*>($1);
    }
    ;

FuncCall: Ident LPAREN RPAREN {
    $$=new ast::FunctionCall(*$1,*(new ast::FunctionActualParameterList()));
}
    | Ident LPAREN FuncRParamList RPAREN {
        $$=new ast::FunctionCall(*$1,*$3);
    }
    ;

FuncRParamList: Exp {
    $$=new ast::FunctionActualParameterList();
    $$->arg_list_.push_back($1);
}
    | FuncRParamList COMMA Exp {
        $$=$1;
        $$->arg_list_.push_back($3);
    }
    ;

MulExp: UnaryExp
    | MulExp MulOp UnaryExp {
        $$=static_cast<ast::Expression*>(new ast::BinaryExpression($2,*$1,*$3));
    }
    ;

AddExp: MulExp
    | AddExp AddOp MulExp {
        $$=static_cast<ast::Expression*>(new ast::BinaryExpression($2,*$1,*$3));
    }
    ;

RelExp: AddExp 
    | RelExp RelOp AddExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression($2,*$1,*$3));
    }
    ;

EqExp: RelExp
    | EqExp EqOp RelExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression($2,*$1,*$3));
    }
    ;

LAndExp: EqExp
    | LAndExp AND EqExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression($2,*$1,*$3));
    }
    ;

LOrExp: LAndExp
    | LOrExp OR LAndExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression($2,*$1,*$3));
    }
    ;

Ident: IDENT {
    $$=new ast::Identifier(*$1);
}
    ;

Number:NUMBER {
    $$=new ast::Number($1);
}
    ;

EqOp: EQ
    | NE
    ;

RelOp: GT
    | GE
    | LT
    | LE
    ;

AddOp: ADD
    | SUB
    ;

MulOp: MUL
    | DIV
    | MOD
    ;

UnaryOp: ADD
    | SUB
    | NOT
    ;

%%