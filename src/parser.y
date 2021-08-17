%{

#include "../include/ast.h"
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <string>

#define YYERROR_VERBOSE true
#define YYDEBUG 1

extern ast::Root *ast_root; // the root node of final AST
extern int yydebug;
extern int yylex();
extern int yylex_destroy();
extern int yyget_lineno();
extern std::unordered_map<std::string, std::unordered_set<ast::FunctionCall*>> called_func_map;
extern clock_t START_TIME, END_TIME;

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
    std::vector<ast::ArrayInitVal*>* array_initval_list;
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

%type <array_initval> InitValArray
%type <array_initval_list> InitValArrayList

%start Root

%%

Root: CompUnit {
    $$=$1;
    ast_root=$$;
}
    ;

CompUnit: Decl {
    $$=new ast::Root(yyget_lineno());
    $$->compunit_list_.push_back(static_cast<ast::CompUnit*>($1));
}
    | CompUnit Decl {
        $$=$1;
        $$->compunit_list_.push_back(static_cast<ast::CompUnit*>($2));
    }
    | FuncDef {
        $$=new ast::Root(yyget_lineno());
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
    $$=new ast::DeclareStatement(yyget_lineno(), $2);
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
    $$=static_cast<ast::Define*>(new ast::VariableDefineWithInit(yyget_lineno(), *$1,*$3,true));
}
    ;

InitVal: AddExp
    ;

ConstDefArray: ArrayIdent ASSIGN InitValArray {
    $$=static_cast<ast::Define*>(new ast::ArrayDefineWithInit(yyget_lineno(), *$1,*$3,true));
}
    ;

ArrayIdent: Ident LSQUARE Exp RSQUARE {
    $$=new ast::ArrayIdentifier(yyget_lineno(), *$1);
    $$->shape_list_.push_back(std::shared_ptr<ast::Expression>($3));
}
    | ArrayIdent LSQUARE Exp RSQUARE {
        $$=$1;
        $$->shape_list_.push_back(std::shared_ptr<ast::Expression>($3));
    }
    ;

InitValArray: LBRACE RBRACE {
    $$=new ast::ArrayInitVal(yyget_lineno(), false, nullptr);
}
    | LBRACE InitValArrayList RBRACE {
        $$=new ast::ArrayInitVal(yyget_lineno(), false, nullptr);
        $$->initval_list_.swap(*$2);
        delete $2;
    }
    ;

// InitValArrayList是InitVal(AddExp)和InitValArray的任意组合
InitValArrayList: InitValArray {
    $$=new std::vector<ast::ArrayInitVal*>;
    $$->push_back($1);
}
    | InitValArrayList COMMA InitValArray {
        $$=$1;
        $$->push_back($3);
    }
    | InitVal {
        $$=new std::vector<ast::ArrayInitVal*>;
        $$->push_back(new ast::ArrayInitVal(yyget_lineno(), true, $1));
    }
    | InitValArrayList COMMA InitVal {
        $$=$1;
        $$->push_back(new ast::ArrayInitVal(yyget_lineno(), true, $3));
    }
    ;

VarDecl: BType Def {
    $$=new ast::DeclareStatement(yyget_lineno(), $1);
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
    $$=static_cast<ast::Define*>(new ast::VariableDefine(yyget_lineno(), *$1));
}
    | Ident ASSIGN InitVal {
        $$=static_cast<ast::Define*>(new ast::VariableDefineWithInit(yyget_lineno(), *$1,*$3,false));
    }
    ;

DefArray: ArrayIdent {
    $$=static_cast<ast::Define*>(new ast::ArrayDefine(yyget_lineno(), *$1));
}
    | ArrayIdent ASSIGN InitValArray {
        $$=static_cast<ast::Define*>(new ast::ArrayDefineWithInit(yyget_lineno(), *$1,*$3,false));
    }
    ;

// ===============函数相关=================
FuncDef: VOID Ident LPAREN FuncFParamList RPAREN Block {
    $$=new ast::FunctionDefine(yyget_lineno(), $1,*$2,*$4,*$6);
}
    | VOID Ident LPAREN RPAREN Block {
        $$=new ast::FunctionDefine(yyget_lineno(), $1, *$2, *(new ast::FunctionFormalParameterList(yyget_lineno())), *$5);
    }
    | BType Ident LPAREN FuncFParamList RPAREN Block {
        $$=new ast::FunctionDefine(yyget_lineno(), $1,*$2,*$4,*$6);
    }
    | BType Ident LPAREN RPAREN Block {
        $$=new ast::FunctionDefine(yyget_lineno(), $1, *$2, *(new ast::FunctionFormalParameterList(yyget_lineno())), *$5);
    }

    ;

FuncFParamList: FuncFParam {
    $$=new ast::FunctionFormalParameterList(yyget_lineno());
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
    $$=new ast::FunctionFormalParameter(yyget_lineno(), $1, *static_cast<ast::LeftValue*>($2));
}
    ;

FuncFParamArray: BType Ident LSQUARE RSQUARE {
    auto array_ident = new ast::ArrayIdentifier(yyget_lineno(), *$2);
    // NOTE
    // array_ident->shape_list_.push_back(static_cast<ast::Expression*>(new ast::Number(yyget_lineno(), 1)));
    $$=new ast::FunctionFormalParameter(yyget_lineno(),$1,
                           static_cast<ast::LeftValue&>(*(array_ident))
                          );
}
    | FuncFParamArray LSQUARE Exp RSQUARE {
        $$=$1;
        dynamic_cast<ast::ArrayIdentifier&>($$->name_).shape_list_.push_back(std::shared_ptr<ast::Expression>($3));
    }
    ;

Block: LBRACE RBRACE {
    $$=new ast::Block(yyget_lineno());
} 
    | LBRACE BlockItemList RBRACE {
        $$=$2;
    }
    ;

BlockItemList: BlockItem {
    $$=new ast::Block(yyget_lineno());
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
        $$=static_cast<ast::Statement*>(new ast::BreakStatement(yyget_lineno()));
    }
    | CONTINUE SEMI {
        $$=static_cast<ast::Statement*>(new ast::ContinueStatement(yyget_lineno()));
    }
    | SEMI {
        $$=static_cast<ast::Statement*>(new ast::VoidStatement(yyget_lineno()));
    }
    | Exp SEMI {
        $$=static_cast<ast::Statement*>(new ast::EvalStatement(yyget_lineno(), *$1));
    }
    ; 

AssignStmt: LVal ASSIGN Exp SEMI {
    $$=static_cast<ast::Statement*>(new ast::AssignStatement(yyget_lineno(), *$1,*$3));
}
    ;

IfStmt: IF LPAREN Cond RPAREN Stmt {
    $$=static_cast<ast::Statement*>(
        new ast::IfElseStatement(yyget_lineno(), (dynamic_cast<ast::ConditionExpression&>(*$3)), *$5, nullptr));
}
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        $$=static_cast<ast::Statement*>(
            new ast::IfElseStatement(yyget_lineno(), (dynamic_cast<ast::ConditionExpression&>(*$3)), *$5, $7));
    }
    ;   

WhileStmt: WHILE LPAREN Cond RPAREN Stmt {
    $$=static_cast<ast::Statement*>(
        new ast::WhileStatement(yyget_lineno(), (dynamic_cast<ast::ConditionExpression&>(*$3)), *$5)
    );
}
    ;

ReturnStmt: RETURN SEMI {
    $$=static_cast<ast::Statement*>(new ast::ReturnStatement(yyget_lineno(), nullptr));
}
    | RETURN Exp SEMI {
        $$=static_cast<ast::Statement*>(new ast::ReturnStatement(yyget_lineno(), $2));
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
        $$=static_cast<ast::Expression*>(new ast::UnaryExpression(yyget_lineno(), $1,*$2));
    }
    | FuncCall {
        $$=static_cast<ast::Expression*>($1);
    }
    ;

FuncCall: Ident LPAREN RPAREN {
    $$=new ast::FunctionCall(yyget_lineno(), *$1,*(new ast::FunctionActualParameterList(yyget_lineno())));
    called_func_map[$1->name_].insert($$);
}
    | Ident LPAREN FuncRParamList RPAREN {
        $$=new ast::FunctionCall(yyget_lineno(), *$1,*$3);
        called_func_map[$1->name_].insert($$);
    }
    ;

FuncRParamList: Exp {
    $$=new ast::FunctionActualParameterList(yyget_lineno());
    $$->arg_list_.push_back($1);
}
    | FuncRParamList COMMA Exp {
        $$=$1;
        $$->arg_list_.push_back($3);
    }
    ;

MulExp: UnaryExp
    | MulExp MulOp UnaryExp {
        $$=static_cast<ast::Expression*>(new ast::BinaryExpression(yyget_lineno(), $2, std::shared_ptr<ast::Expression>($1), *$3));
    }
    ;

AddExp: MulExp
    | AddExp AddOp MulExp {
        $$=static_cast<ast::Expression*>(new ast::BinaryExpression(yyget_lineno(), $2, std::shared_ptr<ast::Expression>($1), *$3));
    }
    ;

RelExp: AddExp 
    | RelExp RelOp AddExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), $2,*$1,*$3));
    }
    ;

EqExp: RelExp
    | EqExp EqOp RelExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), $2,*$1,*$3));
    }
    ;

LAndExp: EqExp
    | LAndExp AND EqExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), $2,*$1,*$3));
    }
    ;

LOrExp: LAndExp {
    if(auto ptr=dynamic_cast<ast::ConditionExpression*>($1)){ $$ = $1; }
    else{// NOTE: ($1 || 0)
        $$ = static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), OR, *$1, *(new ast::Number(yyget_lineno(), 0)))); }
}
    | LOrExp OR LAndExp {
        $$=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), $2,*$1,*$3));
    }
    ;

Ident: IDENT {
    $$=new ast::Identifier(yyget_lineno(), *$1);
}
    ;

Number:NUMBER {
    $$=new ast::Number(yyget_lineno(), $1);
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