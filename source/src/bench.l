%{

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "ast.h"
#include "bench.tab.hh"

#define YY_DECL extern "C" int yylex()
%}

%s IN_COMMENT
%%
<INITIAL>{
\#                                      BEGIN(IN_COMMENT);
INPUT                                   return INPUT;
OUTPUT                                  return OUTPUT;
\(                                      return LPAREN;
\)                                      return RPAREN;
,                                       return COMMA;
=                                       return EQUALS;
[_A-Za-z][A-Za-z0-9_$]*                 yylval.id = new std::string(yytext); return ID;
[ \t+\r]+                               /* ignore whitespace. */
\n                                      yylineno++;
}

<IN_COMMENT>{
\n                                      yylineno++; BEGIN(INITIAL);
[^\n]+                                  // eat up other stuff.
}
%%
