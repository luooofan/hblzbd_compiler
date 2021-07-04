/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

#ifndef YY_YY_SRC_PARSER_HPP_INCLUDED
# define YY_YY_SRC_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    ADD = 258,
    SUB = 259,
    NOT = 260,
    MUL = 261,
    DIV = 262,
    MOD = 263,
    LT = 264,
    LE = 265,
    GT = 266,
    GE = 267,
    EQ = 268,
    NE = 269,
    AND = 270,
    OR = 271,
    ASSIGN = 272,
    LPAREN = 273,
    RPAREN = 274,
    LBRACE = 275,
    RBRACE = 276,
    LSQUARE = 277,
    RSQUARE = 278,
    COMMA = 279,
    SEMI = 280,
    IF = 281,
    ELSE = 282,
    WHILE = 283,
    FOR = 284,
    BREAK = 285,
    CONTINUE = 286,
    RETURN = 287,
    CONST = 288,
    INT = 289,
    VOID = 290,
    IDENT = 291,
    NUMBER = 292
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 25 "src/parser.y"

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

#line 119 "src/parser.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SRC_PARSER_HPP_INCLUDED  */
