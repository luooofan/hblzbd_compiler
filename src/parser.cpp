/* A Bison parser, made by GNU Bison 3.3.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2019 Free Software Foundation,
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.3.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 1 "src/parser.y" /* yacc.c:337  */


#include "../include/ast.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>

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


#line 94 "src/parser.cpp" /* yacc.c:337  */
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "parser.hpp".  */
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
#line 26 "src/parser.y" /* yacc.c:352  */

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

#line 199 "src/parser.cpp" /* yacc.c:352  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SRC_PARSER_HPP_INCLUDED  */



#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  14
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   232

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  38
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  50
/* YYNRULES -- Number of rules.  */
#define YYNRULES  106
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  176

#define YYUNDEFTOK  2
#define YYMAXUTOK   292

/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  ((unsigned) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,    96,    96,   102,   106,   110,   114,   120,   121,   124,
     128,   134,   137,   138,   141,   146,   149,   154,   158,   164,
     167,   175,   179,   183,   187,   193,   197,   203,   204,   207,
     210,   215,   218,   224,   227,   230,   233,   239,   243,   249,
     250,   253,   258,   266,   272,   275,   280,   284,   290,   291,
     297,   298,   299,   300,   301,   304,   307,   310,   313,   318,
     323,   327,   333,   340,   343,   349,   352,   355,   358,   363,
     366,   369,   374,   375,   378,   383,   386,   391,   395,   401,
     402,   407,   408,   413,   414,   419,   420,   425,   426,   431,
     435,   440,   445,   450,   451,   454,   455,   456,   457,   460,
     461,   464,   465,   466,   469,   470,   471
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "ADD", "SUB", "NOT", "MUL", "DIV", "MOD",
  "LT", "LE", "GT", "GE", "EQ", "NE", "AND", "OR", "ASSIGN", "LPAREN",
  "RPAREN", "LBRACE", "RBRACE", "LSQUARE", "RSQUARE", "COMMA", "SEMI",
  "IF", "ELSE", "WHILE", "FOR", "BREAK", "CONTINUE", "RETURN", "CONST",
  "INT", "VOID", "IDENT", "NUMBER", "$accept", "Root", "CompUnit", "Decl",
  "ConstDecl", "BType", "ConstDef", "ConstDefOne", "InitVal",
  "ConstDefArray", "ArrayIdent", "InitValArray", "InitValArrayList",
  "VarDecl", "Def", "DefOne", "DefArray", "FuncDef", "FuncFParamList",
  "FuncFParam", "FuncFParamOne", "FuncFParamArray", "Block",
  "BlockItemList", "BlockItem", "Stmt", "AssignStmt", "IfStmt",
  "WhileStmt", "ReturnStmt", "Exp", "Cond", "LVal", "PrimaryExp",
  "UnaryExp", "FuncCall", "FuncRParamList", "MulExp", "AddExp", "RelExp",
  "EqExp", "LAndExp", "LOrExp", "Ident", "Number", "EqOp", "RelOp",
  "AddOp", "MulOp", "UnaryOp", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292
};
# endif

#define YYPACT_NINF -145

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-145)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      69,    -1,  -145,    18,    56,    69,  -145,    22,    18,    75,
    -145,    18,  -145,    79,  -145,  -145,  -145,    18,  -145,    26,
    -145,  -145,  -145,    19,    18,  -145,  -145,  -145,  -145,    27,
      38,    -5,  -145,    20,   195,   195,     4,   195,  -145,    40,
      20,   195,    73,    18,    39,  -145,  -145,    85,   151,  -145,
    -145,  -145,  -145,   195,  -145,    86,    87,  -145,  -145,  -145,
    -145,   129,    23,    49,  -145,   195,  -145,    23,    73,    53,
      92,  -145,  -145,    48,  -145,   101,    73,    -1,   195,  -145,
    -145,  -145,    68,    95,  -145,  -145,  -145,  -145,   195,  -145,
    -145,   195,   178,  -145,  -145,    73,  -145,  -145,  -145,   106,
     108,   102,   107,   186,  -145,    18,  -145,   113,  -145,  -145,
    -145,  -145,  -145,  -145,   115,   125,   134,  -145,  -145,   135,
    -145,   157,  -145,  -145,   129,  -145,  -145,    64,  -145,   195,
     195,  -145,  -145,  -145,   123,  -145,  -145,  -145,   195,  -145,
    -145,  -145,  -145,  -145,   195,   140,    23,   110,    51,   149,
     147,   146,  -145,   142,  -145,   148,  -145,  -145,  -145,  -145,
     195,  -145,  -145,   195,   195,   195,   148,  -145,   143,    23,
     110,    51,   149,  -145,   148,  -145
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,    11,     0,     0,     2,     3,     0,     0,     0,
       5,     0,    91,     0,     1,     4,     6,     0,     7,    31,
      25,    27,    28,    29,     0,     8,     9,    12,    13,     0,
       0,     0,    10,     0,     0,     0,     0,     0,    26,    29,
       0,     0,     0,     0,     0,    37,    39,    40,     0,    32,
     104,   105,   106,     0,    92,    68,     0,    71,    72,    79,
      74,    81,    65,    67,    70,     0,    30,    15,     0,     0,
       0,    16,    14,     0,    34,    41,     0,     0,     0,    19,
      23,    21,     0,     0,    18,   101,   102,   103,     0,    99,
     100,     0,     0,    73,    36,     0,    17,    44,    57,     0,
       0,     0,     0,     0,    49,     0,    54,     0,    46,    48,
      50,    51,    52,    53,     0,    71,     0,    33,    38,     0,
      20,     0,    69,    80,    82,    75,    77,     0,    35,     0,
       0,    55,    56,    63,     0,    45,    47,    58,     0,    42,
      43,    24,    22,    76,     0,     0,    83,    85,    87,    89,
      66,     0,    64,     0,    78,     0,    97,    98,    95,    96,
       0,    93,    94,     0,     0,     0,     0,    59,    60,    84,
      86,    88,    90,    62,     0,    61
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -145,  -145,  -145,    91,  -145,     2,   169,  -145,   -20,  -145,
       8,   -31,  -145,  -145,   168,  -145,  -145,   190,   165,   126,
    -145,  -145,   -34,  -145,    98,  -144,  -145,  -145,  -145,  -145,
     -33,    72,   -68,  -145,   -53,  -145,  -145,   116,   -35,    43,
      44,    45,  -145,     7,  -145,  -145,  -145,  -145,  -145,  -145
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     4,     5,   104,     7,    43,    26,    27,    66,    28,
      55,    49,    82,     9,    20,    21,    22,    10,    44,    45,
      46,    47,   106,   107,   108,   109,   110,   111,   112,   113,
     114,   145,    57,    58,    59,    60,   127,    61,    62,   147,
     148,   149,   150,    63,    64,   163,   160,    91,    88,    65
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      67,    56,     8,    11,    70,   115,    67,     8,    74,    71,
      13,   168,    93,    67,    42,    23,    19,    81,    30,    29,
      83,    72,   173,    68,    30,    29,    89,    90,    80,     2,
     175,    39,    19,     2,    94,   123,    35,    36,     2,   115,
      48,    37,   117,    33,    40,   119,    17,    18,    34,    34,
      75,    50,    51,    52,    12,    41,    14,    35,    76,   126,
      37,   128,    37,    77,   161,   162,    53,    92,    73,    97,
     134,    37,    95,    98,    99,   105,   100,    77,   101,   102,
     103,     1,     2,   143,    12,    54,    67,   115,   144,   120,
     142,     6,   121,    73,   146,   146,    15,    31,   115,    24,
      25,   141,     1,     2,     3,   153,   115,    78,    34,   105,
      84,   154,    39,    19,   122,    96,    50,    51,    52,   156,
     157,   158,   159,   116,   129,   169,   130,   131,   146,   146,
     146,    53,   132,    73,   135,    85,    86,    87,    98,    99,
     137,   100,   138,   101,   102,   103,     1,     2,   152,    12,
      54,    50,    51,    52,    50,    51,    52,   139,   140,   155,
      50,    51,    52,   165,   164,   166,    53,   167,    73,    53,
     174,    48,    79,    98,    99,    53,   100,    48,   101,   102,
     103,    50,    51,    52,    12,    54,    32,    12,    54,    50,
      51,    52,    38,    12,    54,    16,    53,   125,    50,    51,
      52,    69,   151,   118,    53,   136,   170,   124,   171,     0,
     172,   133,     0,    53,    12,    54,     0,     0,     0,     0,
       0,     0,    12,    54,     0,     0,     0,     0,     0,     0,
       0,    12,    54
};

static const yytype_int16 yycheck[] =
{
      35,    34,     0,     1,    37,    73,    41,     5,    42,    40,
       3,   155,    65,    48,    19,     8,     8,    48,    11,    11,
      53,    41,   166,    19,    17,    17,     3,     4,    48,    34,
     174,    24,    24,    34,    68,    88,    17,    18,    34,   107,
      20,    22,    76,    17,    17,    78,    24,    25,    22,    22,
      43,     3,     4,     5,    36,    17,     0,    17,    19,    92,
      22,    95,    22,    24,    13,    14,    18,    18,    20,    21,
     103,    22,    19,    25,    26,    73,    28,    24,    30,    31,
      32,    33,    34,    19,    36,    37,   121,   155,    24,    21,
     121,     0,    24,    20,   129,   130,     5,    18,   166,    24,
      25,   121,    33,    34,    35,   138,   174,    22,    22,   107,
      23,   144,   105,   105,    19,    23,     3,     4,     5,     9,
      10,    11,    12,    22,    18,   160,    18,    25,   163,   164,
     165,    18,    25,    20,    21,     6,     7,     8,    25,    26,
      25,    28,    17,    30,    31,    32,    33,    34,    25,    36,
      37,     3,     4,     5,     3,     4,     5,    23,    23,    19,
       3,     4,     5,    16,    15,    19,    18,    25,    20,    18,
      27,    20,    21,    25,    26,    18,    28,    20,    30,    31,
      32,     3,     4,     5,    36,    37,    17,    36,    37,     3,
       4,     5,    24,    36,    37,     5,    18,    19,     3,     4,
       5,    36,   130,    77,    18,   107,   163,    91,   164,    -1,
     165,    25,    -1,    18,    36,    37,    -1,    -1,    -1,    -1,
      -1,    -1,    36,    37,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    36,    37
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    33,    34,    35,    39,    40,    41,    42,    43,    51,
      55,    43,    36,    81,     0,    41,    55,    24,    25,    48,
      52,    53,    54,    81,    24,    25,    44,    45,    47,    48,
      81,    18,    44,    17,    22,    17,    18,    22,    52,    81,
      17,    17,    19,    43,    56,    57,    58,    59,    20,    49,
       3,     4,     5,    18,    37,    48,    68,    70,    71,    72,
      73,    75,    76,    81,    82,    87,    46,    76,    19,    56,
      68,    49,    46,    20,    60,    81,    19,    24,    22,    21,
      46,    49,    50,    68,    23,     6,     7,     8,    86,     3,
       4,    85,    18,    72,    60,    19,    23,    21,    25,    26,
      28,    30,    31,    32,    41,    43,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    70,    22,    60,    57,    68,
      21,    24,    19,    72,    75,    19,    68,    74,    60,    18,
      18,    25,    25,    25,    68,    21,    62,    25,    17,    23,
      23,    46,    49,    19,    24,    69,    76,    77,    78,    79,
      80,    69,    25,    68,    68,    19,     9,    10,    11,    12,
      84,    13,    14,    83,    15,    16,    19,    25,    63,    76,
      77,    78,    79,    63,    27,    63
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    38,    39,    40,    40,    40,    40,    41,    41,    42,
      42,    43,    44,    44,    45,    46,    47,    48,    48,    49,
      49,    50,    50,    50,    50,    51,    51,    52,    52,    53,
      53,    54,    54,    55,    55,    55,    55,    56,    56,    57,
      57,    58,    59,    59,    60,    60,    61,    61,    62,    62,
      63,    63,    63,    63,    63,    63,    63,    63,    63,    64,
      65,    65,    66,    67,    67,    68,    69,    70,    70,    71,
      71,    71,    72,    72,    72,    73,    73,    74,    74,    75,
      75,    76,    76,    77,    77,    78,    78,    79,    79,    80,
      80,    81,    82,    83,    83,    84,    84,    84,    84,    85,
      85,    86,    86,    86,    87,    87,    87
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     2,     1,     2,     2,     2,     3,
       3,     1,     1,     1,     3,     1,     3,     4,     4,     2,
       3,     1,     3,     1,     3,     2,     3,     1,     1,     1,
       3,     1,     3,     6,     5,     6,     5,     1,     3,     1,
       1,     2,     4,     4,     2,     3,     1,     2,     1,     1,
       1,     1,     1,     1,     1,     2,     2,     1,     2,     4,
       5,     7,     5,     2,     3,     1,     1,     1,     1,     3,
       1,     1,     1,     2,     1,     3,     4,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return (YYSIZE_T) (yystpcpy (yyres, yystr) - yyres);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yynewstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  *yyssp = (yytype_int16) yystate;

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = (YYSIZE_T) (yyssp - yyss + 1);

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 96 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.root)=(yyvsp[0].root);
    ast_root=(yyval.root);
}
#line 1436 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 3:
#line 102 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.root)=new ast::Root(yyget_lineno());
    (yyval.root)->compunit_list_.push_back(static_cast<ast::CompUnit*>((yyvsp[0].declare)));
}
#line 1445 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 4:
#line 106 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.root)=(yyvsp[-1].root);
        (yyval.root)->compunit_list_.push_back(static_cast<ast::CompUnit*>((yyvsp[0].declare)));
    }
#line 1454 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 5:
#line 110 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.root)=new ast::Root(yyget_lineno());
        (yyval.root)->compunit_list_.push_back(static_cast<ast::CompUnit*>((yyvsp[0].funcdef)));
    }
#line 1463 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 6:
#line 114 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.root)=(yyvsp[-1].root);
        (yyval.root)->compunit_list_.push_back(static_cast<ast::CompUnit*>((yyvsp[0].funcdef)));
    }
#line 1472 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 9:
#line 124 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.declare)=new ast::DeclareStatement(yyget_lineno(), (yyvsp[-1].token));
    (yyval.declare)->define_list_.push_back((yyvsp[0].define));
}
#line 1481 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 10:
#line 128 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.declare)=(yyvsp[-2].declare);
        (yyval.declare)->define_list_.push_back((yyvsp[0].define));
    }
#line 1490 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 14:
#line 141 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.define)=static_cast<ast::Define*>(new ast::VariableDefineWithInit(yyget_lineno(), *(yyvsp[-2].ident),*(yyvsp[0].expr),true));
}
#line 1498 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 16:
#line 149 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.define)=static_cast<ast::Define*>(new ast::ArrayDefineWithInit(yyget_lineno(), *(yyvsp[-2].array_ident),*(yyvsp[0].array_initval),true));
}
#line 1506 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 17:
#line 154 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.array_ident)=new ast::ArrayIdentifier(yyget_lineno(), *(yyvsp[-3].ident));
    (yyval.array_ident)->shape_list_.push_back(std::shared_ptr<ast::Expression>((yyvsp[-1].expr)));
}
#line 1515 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 18:
#line 158 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.array_ident)=(yyvsp[-3].array_ident);
        (yyval.array_ident)->shape_list_.push_back(std::shared_ptr<ast::Expression>((yyvsp[-1].expr)));
    }
#line 1524 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 19:
#line 164 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.array_initval)=new ast::ArrayInitVal(yyget_lineno(), false, nullptr);
}
#line 1532 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 20:
#line 167 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.array_initval)=new ast::ArrayInitVal(yyget_lineno(), false, nullptr);
        (yyval.array_initval)->initval_list_.swap(*(yyvsp[-1].array_initval_list));
        delete (yyvsp[-1].array_initval_list);
    }
#line 1542 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 21:
#line 175 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.array_initval_list)=new std::vector<ast::ArrayInitVal*>;
    (yyval.array_initval_list)->push_back((yyvsp[0].array_initval));
}
#line 1551 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 22:
#line 179 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.array_initval_list)=(yyvsp[-2].array_initval_list);
        (yyval.array_initval_list)->push_back((yyvsp[0].array_initval));
    }
#line 1560 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 23:
#line 183 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.array_initval_list)=new std::vector<ast::ArrayInitVal*>;
        (yyval.array_initval_list)->push_back(new ast::ArrayInitVal(yyget_lineno(), true, (yyvsp[0].expr)));
    }
#line 1569 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 24:
#line 187 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.array_initval_list)=(yyvsp[-2].array_initval_list);
        (yyval.array_initval_list)->push_back(new ast::ArrayInitVal(yyget_lineno(), true, (yyvsp[0].expr)));
    }
#line 1578 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 25:
#line 193 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.declare)=new ast::DeclareStatement(yyget_lineno(), (yyvsp[-1].token));
    (yyval.declare)->define_list_.push_back((yyvsp[0].define));
}
#line 1587 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 26:
#line 197 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.declare)=(yyvsp[-2].declare);
        (yyval.declare)->define_list_.push_back((yyvsp[0].define));
    }
#line 1596 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 29:
#line 207 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.define)=static_cast<ast::Define*>(new ast::VariableDefine(yyget_lineno(), *(yyvsp[0].ident)));
}
#line 1604 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 30:
#line 210 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.define)=static_cast<ast::Define*>(new ast::VariableDefineWithInit(yyget_lineno(), *(yyvsp[-2].ident),*(yyvsp[0].expr),false));
    }
#line 1612 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 31:
#line 215 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.define)=static_cast<ast::Define*>(new ast::ArrayDefine(yyget_lineno(), *(yyvsp[0].array_ident)));
}
#line 1620 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 32:
#line 218 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.define)=static_cast<ast::Define*>(new ast::ArrayDefineWithInit(yyget_lineno(), *(yyvsp[-2].array_ident),*(yyvsp[0].array_initval),false));
    }
#line 1628 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 33:
#line 224 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.funcdef)=new ast::FunctionDefine(yyget_lineno(), (yyvsp[-5].token),*(yyvsp[-4].ident),*(yyvsp[-2].funcfparams),*(yyvsp[0].block));
}
#line 1636 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 34:
#line 227 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funcdef)=new ast::FunctionDefine(yyget_lineno(), (yyvsp[-4].token), *(yyvsp[-3].ident), *(new ast::FunctionFormalParameterList(yyget_lineno())), *(yyvsp[0].block));
    }
#line 1644 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 35:
#line 230 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funcdef)=new ast::FunctionDefine(yyget_lineno(), (yyvsp[-5].token),*(yyvsp[-4].ident),*(yyvsp[-2].funcfparams),*(yyvsp[0].block));
    }
#line 1652 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 36:
#line 233 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funcdef)=new ast::FunctionDefine(yyget_lineno(), (yyvsp[-4].token), *(yyvsp[-3].ident), *(new ast::FunctionFormalParameterList(yyget_lineno())), *(yyvsp[0].block));
    }
#line 1660 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 37:
#line 239 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.funcfparams)=new ast::FunctionFormalParameterList(yyget_lineno());
    (yyval.funcfparams)->arg_list_.push_back((yyvsp[0].funcfparam));
}
#line 1669 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 38:
#line 243 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funcfparams)=(yyvsp[-2].funcfparams);
        (yyval.funcfparams)->arg_list_.push_back((yyvsp[0].funcfparam));
    }
#line 1678 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 41:
#line 253 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.funcfparam)=new ast::FunctionFormalParameter(yyget_lineno(), (yyvsp[-1].token), *static_cast<ast::LeftValue*>((yyvsp[0].ident)));
}
#line 1686 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 42:
#line 258 "src/parser.y" /* yacc.c:1652  */
    {
    auto array_ident = new ast::ArrayIdentifier(yyget_lineno(), *(yyvsp[-2].ident));
    // NOTE
    // array_ident->shape_list_.push_back(static_cast<ast::Expression*>(new ast::Number(yyget_lineno(), 1)));
    (yyval.funcfparam)=new ast::FunctionFormalParameter(yyget_lineno(),(yyvsp[-3].token),
                           static_cast<ast::LeftValue&>(*(array_ident))
                          );
}
#line 1699 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 43:
#line 266 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funcfparam)=(yyvsp[-3].funcfparam);
        dynamic_cast<ast::ArrayIdentifier&>((yyval.funcfparam)->name_).shape_list_.push_back(std::shared_ptr<ast::Expression>((yyvsp[-1].expr)));
    }
#line 1708 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 44:
#line 272 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.block)=new ast::Block(yyget_lineno());
}
#line 1716 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 45:
#line 275 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.block)=(yyvsp[-1].block);
    }
#line 1724 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 46:
#line 280 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.block)=new ast::Block(yyget_lineno());
    (yyval.block)->statement_list_.push_back((yyvsp[0].statement));
}
#line 1733 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 47:
#line 284 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.block)=(yyvsp[-1].block);
        (yyval.block)->statement_list_.push_back((yyvsp[0].statement));
    }
#line 1742 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 49:
#line 291 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>((yyvsp[0].declare));
    }
#line 1750 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 54:
#line 301 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>((yyvsp[0].block));
    }
#line 1758 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 55:
#line 304 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>(new ast::BreakStatement(yyget_lineno()));
    }
#line 1766 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 56:
#line 307 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>(new ast::ContinueStatement(yyget_lineno()));
    }
#line 1774 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 57:
#line 310 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>(new ast::VoidStatement(yyget_lineno()));
    }
#line 1782 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 58:
#line 313 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>(new ast::EvalStatement(yyget_lineno(), *(yyvsp[-1].expr)));
    }
#line 1790 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 59:
#line 318 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.statement)=static_cast<ast::Statement*>(new ast::AssignStatement(yyget_lineno(), *(yyvsp[-3].left_val),*(yyvsp[-1].expr)));
}
#line 1798 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 60:
#line 323 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.statement)=static_cast<ast::Statement*>(
        new ast::IfElseStatement(yyget_lineno(), (dynamic_cast<ast::ConditionExpression&>(*(yyvsp[-2].expr))), *(yyvsp[0].statement), nullptr));
}
#line 1807 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 61:
#line 327 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>(
            new ast::IfElseStatement(yyget_lineno(), (dynamic_cast<ast::ConditionExpression&>(*(yyvsp[-4].expr))), *(yyvsp[-2].statement), (yyvsp[0].statement)));
    }
#line 1816 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 62:
#line 333 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.statement)=static_cast<ast::Statement*>(
        new ast::WhileStatement(yyget_lineno(), (dynamic_cast<ast::ConditionExpression&>(*(yyvsp[-2].expr))), *(yyvsp[0].statement))
    );
}
#line 1826 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 63:
#line 340 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.statement)=static_cast<ast::Statement*>(new ast::ReturnStatement(yyget_lineno(), nullptr));
}
#line 1834 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 64:
#line 343 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.statement)=static_cast<ast::Statement*>(new ast::ReturnStatement(yyget_lineno(), (yyvsp[-1].expr)));
    }
#line 1842 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 67:
#line 355 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.left_val)=static_cast<ast::LeftValue*>((yyvsp[0].ident));
}
#line 1850 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 68:
#line 358 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.left_val)=static_cast<ast::LeftValue*>((yyvsp[0].array_ident));
    }
#line 1858 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 69:
#line 363 "src/parser.y" /* yacc.c:1652  */
    {  // TODO: Exp or Cond
    (yyval.expr)=static_cast<ast::Expression*>((yyvsp[-1].expr));
}
#line 1866 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 70:
#line 366 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>((yyvsp[0].number));
    }
#line 1874 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 71:
#line 369 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>((yyvsp[0].left_val));
    }
#line 1882 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 73:
#line 375 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::UnaryExpression(yyget_lineno(), (yyvsp[-1].token),*(yyvsp[0].expr)));
    }
#line 1890 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 74:
#line 378 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>((yyvsp[0].funccall));
    }
#line 1898 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 75:
#line 383 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.funccall)=new ast::FunctionCall(yyget_lineno(), *(yyvsp[-2].ident),*(new ast::FunctionActualParameterList(yyget_lineno())));
}
#line 1906 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 76:
#line 386 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funccall)=new ast::FunctionCall(yyget_lineno(), *(yyvsp[-3].ident),*(yyvsp[-1].funcaparams));
    }
#line 1914 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 77:
#line 391 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.funcaparams)=new ast::FunctionActualParameterList(yyget_lineno());
    (yyval.funcaparams)->arg_list_.push_back((yyvsp[0].expr));
}
#line 1923 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 78:
#line 395 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.funcaparams)=(yyvsp[-2].funcaparams);
        (yyval.funcaparams)->arg_list_.push_back((yyvsp[0].expr));
    }
#line 1932 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 80:
#line 402 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::BinaryExpression(yyget_lineno(), (yyvsp[-1].token), std::shared_ptr<ast::Expression>((yyvsp[-2].expr)), *(yyvsp[0].expr)));
    }
#line 1940 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 82:
#line 408 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::BinaryExpression(yyget_lineno(), (yyvsp[-1].token), std::shared_ptr<ast::Expression>((yyvsp[-2].expr)), *(yyvsp[0].expr)));
    }
#line 1948 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 84:
#line 414 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), (yyvsp[-1].token),*(yyvsp[-2].expr),*(yyvsp[0].expr)));
    }
#line 1956 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 86:
#line 420 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), (yyvsp[-1].token),*(yyvsp[-2].expr),*(yyvsp[0].expr)));
    }
#line 1964 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 88:
#line 426 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), (yyvsp[-1].token),*(yyvsp[-2].expr),*(yyvsp[0].expr)));
    }
#line 1972 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 89:
#line 431 "src/parser.y" /* yacc.c:1652  */
    {
    // NOTE: ($1 || 0)
    (yyval.expr) = static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), OR, *(yyvsp[0].expr), *(new ast::Number(yyget_lineno(), 0))));
}
#line 1981 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 90:
#line 435 "src/parser.y" /* yacc.c:1652  */
    {
        (yyval.expr)=static_cast<ast::Expression*>(new ast::ConditionExpression(yyget_lineno(), (yyvsp[-1].token),*(yyvsp[-2].expr),*(yyvsp[0].expr)));
    }
#line 1989 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 91:
#line 440 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.ident)=new ast::Identifier(yyget_lineno(), *(yyvsp[0].string));
}
#line 1997 "src/parser.cpp" /* yacc.c:1652  */
    break;

  case 92:
#line 445 "src/parser.y" /* yacc.c:1652  */
    {
    (yyval.number)=new ast::Number(yyget_lineno(), (yyvsp[0].token));
}
#line 2005 "src/parser.cpp" /* yacc.c:1652  */
    break;


#line 2009 "src/parser.cpp" /* yacc.c:1652  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 474 "src/parser.y" /* yacc.c:1918  */
