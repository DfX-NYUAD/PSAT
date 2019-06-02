%{
#include <stdio.h>
#include <assert.h>
#include <string>
#include <vector>
#include "ast.h"

extern int yylex(void);
extern int yylineno;

void yyerror(const char *str)
{
    fprintf(stderr,"Line %d: %s\n", yylineno, str);
}
 
int yywrap()
{
    return 1;
} 
  
%}

%union {
    std::string             *id;
    ast_n::input_decl_t     *input_stm;
    ast_n::output_decl_t    *output_stm;
    ast_n::gate_decl_t      *gate_stm;
    ast_n::statement_t      *stm;
    ast_n::statements_t     *statements;
    ast_n::gate_inputs_t    *gate_inputs;
}

/* terminal symbols. */
%token INPUT OUTPUT LPAREN RPAREN COMMA EQUALS
%token <id> ID
/* non-terminals */
%type <input_stm>      INPUT_DECL;
%type <output_stm>     OUTPUT_DECL;
%type <gate_stm>       GATE_DECL;
%type <gate_inputs>    GATE_INPUTS;
%type <stm>            STATEMENT;
%type <statements>     STATEMENTS;

%start STATEMENTS

%%

STATEMENTS: STATEMENTS STATEMENT 
             {
                 $$->push_back(*$2);
                 delete $2;
                 ast_n::statements = $$;
             }
             | %empty
             {
                 $$ = new ast_n::statements_t();
                 ast_n::statements = $$;
             }
;

STATEMENT: INPUT_DECL
            {
                $$ = new ast_n::statement_t($1);
            }
         | OUTPUT_DECL
            {
                $$ = new ast_n::statement_t($1);
            }
         | GATE_DECL
            {
                $$ = new ast_n::statement_t($1);
            }
;

INPUT_DECL: INPUT LPAREN ID RPAREN
            {
                $$ = new ast_n::input_decl_t(*$3);
                delete $3;
            }
;

OUTPUT_DECL: OUTPUT LPAREN ID RPAREN
            {
                $$ = new ast_n::output_decl_t(*$3);
                delete $3;
            }
;
GATE_DECL: ID EQUALS ID LPAREN GATE_INPUTS RPAREN
            {
                $$ = new ast_n::gate_decl_t(*$1, *$3, *$5);
                delete $1;
                delete $3;
                delete $5;
            }
;
GATE_INPUTS: ID
            {
                $$ = new ast_n::gate_inputs_t();
                $$->push_back(*$1);
                delete $1;
            }
           | GATE_INPUTS COMMA ID
           {
               $$ = $1;
               $1->push_back(*$3);
               delete $3;
           }
;
%%
