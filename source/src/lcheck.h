#ifndef _LCHECK_H_DEFINED_
#define _LCHECK_H_DEFINED_

#include <stdio.h>

extern int yyparse();
extern FILE* yyin;

int lcheck_main(int argc, char* argv[]);
void print_lcheck_usage(const char* argv0);

#endif
