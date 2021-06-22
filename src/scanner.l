%option noyywrap nodefault yylineno
%x COMMENT

UCN (\\u[0-9a-fA-F]{4}|\\U[0-9a-fA-F]{8})
ILEN ([Uu](L|l|LL|ll)?|(L|l|LL|ll)[Uu]?)

%{

#include "../include/ast.h"

#include "../include/parser.hpp"

#define SAVE_TOKEN  yylval.string = new std::string(yytext, yyleng)

int cal8(char *s);
int cal10(char *s);
int cal16(char *s);

%}

%%

"/*"		{BEGIN(COMMENT);}
<COMMENT>"*/"  {BEGIN(INITIAL);}
<COMMENT>([^*]|\n)+|.
<COMMENT><<EOF>>   {printf("Unterminated comment\n"); return 0;}
"//".*\n


int 		{ yylval.token = INT; return INT; }
void 		{ yylval.token = VOID; return VOID; }
			 
const 		{ yylval.token = CONST; return CONST; }
			  
break		{ yylval.token = BREAK; return BREAK; }
continue	{ yylval.token = CONTINUE; return CONTINUE; }
if			{ yylval.token = IF; return IF; }
else 		{ yylval.token = ELSE; return ELSE; }
return		{ yylval.token = RETURN; return RETURN; }
while		{ yylval.token = WHILE; return WHILE; }
			  
"+"			{ yylval.token = ADD; return ADD; }
"-"			{ yylval.token = SUB; return SUB; }
"*"			{ yylval.token = MUL; return MUL; }
"/"			{ yylval.token = DIV; return DIV; }
"%"			{ yylval.token = MOD; return MOD; }
"="			{ yylval.token = ASSIGN; return ASSIGN; }
"=="		{ yylval.token = EQ; return EQ; }
"!="		{ yylval.token = NE; return NE; }
"<"			{ yylval.token = LT; return LT; }
">"			{ yylval.token = GT; return GT; }
"<="		{ yylval.token = LE; return LE; }
">="		{ yylval.token = GE; return GE; }
"!"			{ yylval.token = NOT; return NOT; }
"&&"		{ yylval.token = AND; return AND; }
"||"		{ yylval.token = OR; return OR; }
"["			{ yylval.token = LSQUARE; return LSQUARE; }
"]"			{ yylval.token = RSQUARE; return RSQUARE; }
"("			{ yylval.token = LPAREN; return LPAREN; }
")"			{ yylval.token = RPAREN; return RPAREN; }
"{"			{ yylval.token = LBRACE; return LBRACE; }
"}"			{ yylval.token = RBRACE; return RBRACE; }
","			{ yylval.token = COMMA; return COMMA; }
";"			{ yylval.token = SEMI; return SEMI; }

0[0-7]*{ILEN}?				{ yylval.token = cal8(yytext); return NUMBER; }
[1-9][0-9]*{ILEN}?			{ yylval.token = cal10(yytext); return NUMBER; }
0[Xx][0-9a-fA-F]+{ILEN}?	{ yylval.token = cal16(yytext); return NUMBER; }

([_a-zA-Z]|{UCN})([_a-zA-Z0-9]|{UCN})* { SAVE_TOKEN; return IDENT; }

[ \t\n]+ 	{ /*忽略空白符*/ }
.   { printf("mystery chacators in %d lines: %s", yylineno, yytext);}

%%

int cal10(char *s)
{
    int ans=0,len=strlen(s);
    for(int i=0;i<len;i++)ans=ans*10+(s[i]^48);
    return ans;
}

int cal8(char *s)
{
    int ans=0,len=strlen(s);
    for(int i=1;i<len;i++)ans=ans*8+(s[i]^48);
    return ans;
}

int cal16(char *s)
{
    int ans=0,len=strlen(s);
    for(int i=2;i<len;i++)
    {
        if(s[i]<'A')ans=ans*16+(s[i]^48);
        else if(s[i]<'Z')ans=ans*16+10+(s[i]-'A');
        else ans=ans*16+10+(s[i]-'a');
    }
    return ans;
}


/* main 		{ printf("MAIN: %d %s\n", MAIN, yytext); return MAIN; }*/


// int main(int argc, char ** argv)
// {
// 	int i;
	
// 	if(argc < 2){
// 		//curfilename = "(stdin)";
// 		yylineno = 1;
// 		while(ENDF != yylex()){
// 			yytokentype next = yylex();
// 			if(next != WHITESPACE || next != SEMI)
// 			{
// 				printf("mystery chacators in %d lines: %s\n", yylineno, yytext);
// 			}
// 		}
// 	}
// 	else for(i = 1; i < argc; ++i)
// 	{
// 		FILE *f = fopen(argv[i], "r");
		
// 		if(!f){
// 			perror(argv[i]);
// 			return 1;             
// 		}
// 		//curfilename = argv[i];  
		
		
// 		yyrestart(f);
// 		yylineno = 1;
// 		while(ENDF != yylex()){
// 			yytokentype next = yylex();
// 			if(next != WHITESPACE || next != SEMI)
// 			{
// 				printf("mystery chacators in %d lines: %s\n", yylineno, yytext);
// 			}
// 		}
// 		fclose(f);
// 	}
	
// 	//printrefs();
// 	return 0;
// }