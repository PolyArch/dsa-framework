%{
  #include <stdio.h>
  #include "dfg-parser.tab.h"
  #include "float.h"
  #include "math.h"
  #include <stdbool.h>
  #include <string.h>
  #include <cstdint>
  #include "ssdfg.h"
  #include <string>

  int yyparse();
  
  extern int sym_type(const char *);  /* returns type from symbol table */
  
  #define sym_type(identifier) IDENT /* with no symbol table, fake it */
  
  static void comment(void);
  static int check_type(void);
  void si(YYSTYPE* lval,  char* yytext);
  void sf(YYSTYPE* lval,  char* yytext);
  void iow(YYSTYPE* lval,  char* yytext);

%}

%option yylineno

O   [0-7]
D   [0-9]
NZ  [1-9]
L   [a-zA-Z_]
A   [a-zA-Z_0-9]
H   [a-fA-F0-9]
HP  (0[xX])
E   ([Ee][+-]?{D}+)
P   ([Pp][+-]?{D}+)
FS  (f|F|l|L)
IS  (((u|U)(l|L|ll|LL)?)|((l|L|ll|LL)(u|U)?))
CP  (u|U|L)
SP  (u8|u|U|L)
ES  (\\(['"\?\\abfnrtv]|[0-7]{1,3}|x[a-fA-F0-9]+))
WS  [ \t\v\f]
M   [-]

%%
"/*"                                    { comment(); }
"#"[^\n]*                               {
                                          /* consume //-comment */
                                          if(strlen(yytext) > 6 && strncmp(&yytext[1],"pragma",6) == 0) {
                                            REJECT;
                                          }
                                        }
"---"[^\n]*                             { return NEW_DFG; }

"Input"("8"|"16"|"32"|"64")*            { iow(&yylval, yytext); return(INPUT); }
"InputVec"                              { yylval.i = 64; return(INPUT); }
"Output"("8"|"16"|"32"|"64")*           { iow(&yylval, yytext); return(OUTPUT); }
"OutputVec"                             { yylval.i = 64; return(OUTPUT); }
\n                                      { return EOLN; }


{L}{A}*					{ yylval.s = new std::string(yytext); return IDENT; }

{HP}{H}+{IS}?				{ si(&yylval,yytext); return I_CONST; }
{NZ}{D}*{IS}?				{ si(&yylval,yytext); return I_CONST; }
"0"{O}*{IS}?				{ si(&yylval,yytext); return I_CONST; }
{CP}?"'"([^'\\\n]|{ES})+"'"		{ si(&yylval,yytext); return I_CONST; }

{D}+{E}{FS}?				{ sf(&yylval,yytext); return F_CONST; }
{D}*"."{D}+{E}?{FS}?			{ sf(&yylval,yytext); return F_CONST; }
{D}+"."{E}?{FS}?			{ sf(&yylval,yytext); return F_CONST; }
{HP}{H}+{P}{FS}?			{ sf(&yylval,yytext); return F_CONST; }
{HP}{H}*"."{H}+{P}{FS}?			{ sf(&yylval,yytext); return F_CONST; }
{HP}{H}+"."{P}{FS}?			{ sf(&yylval,yytext); return F_CONST; }

({SP}?\"([^"\\\n]|{ES})*\"{WS}*)+	{ return STRING_LITERAL; }

"#pragma"                               { return PRAGMA; }

"->"                                    { return ARROW;}

"|"                                     { return '|'; }

";"					{ return ';'; }
("{")					{ return '{'; }
("}")					{ return '}'; }
","					{ return ','; }
":"					{ return ':'; }
"="					{ return '='; }
"("					{ return '('; }
")"					{ return ')'; }
("[")					{ return '['; }
("]")					{ return ']'; }

{WS}+					{ /* whitespace separates tokens */ }

<*>.|\n                                 { 
  fprintf(stderr, "Bad Symbol at line %d: '%s'\n", yylineno, yytext); 
  exit(1);}

%%

static void comment(void)
{
  int c;
  while ((c = yyinput()) != 0) {
    if (c == '*') {
        while ((c = yyinput()) == '*');
  
        if (c == '/') return;
        if (c == 0)   break;
    }
  }
  fprintf(stderr, "unterminated comment\n");
}

void si(YYSTYPE* lval,  char* yytext) {
  if(yyleng>2 && yytext[0]=='0' && yytext[1]!='x') {
    lval->i = strtoll(yytext,NULL,10);
  }

  lval->i = strtoll(yytext,NULL,0);
}

void sf(YYSTYPE* lval,char* yytext) {
  lval->d = strtod(yytext,NULL);
}

void iow(YYSTYPE* lval,char* yytext) {
  if (strcmp(yytext, "Input") == 0 || strcmp(yytext, "Output") == 0) {
    lval->i = 64;
    return;
  }
  int n = strlen(yytext);
  if (yytext[n - 1] == '8') {
    lval->i = 8;
    return;
  }
  lval->i = (yytext[n - 2] - '0') * 10 + (yytext[n - 1] - '0');
}

int yywrap(void) {      /* called at end of input */
    return 1;           /* terminate now */
}



//static int check_type(void)
//{
//    switch (sym_type(yytext))
//    {
//    case TYPEDEF_NAME:                /* previously defined */
//        return TYPEDEF_NAME;
//    case ENUMERATION_CONSTANT:        /* previously defined */
//        return ENUMERATION_CONSTANT;
//    default:                          /* includes undefined */
//        return IDENT;
//    }
//}



/*
\= 						{ 
							yylval.fun->equals = true;
							return EQUALS; 
						}
[a-zA-Z][0-9]*			{
							yylval.fun->find = strdup(yytext);
							if (!yylval.fun->equals)
								yylval.fun->var = strdup(yytext);
							return VARIABLE; 
						}
[0-9\.]+ 				{ 
							sscanf(yytext, "%lf", &yylval.fun->dub); 
							return NUMBER; 
						}
[ \t]					;
. 						{ 
							printf("Invalid input. Exiting.\n"); 
							exit(-1); 
						}

*/
