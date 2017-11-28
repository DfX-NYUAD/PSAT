#include "lcmp.h"
#include <iostream>
#include "ast.h"
#include "ckt.h"
#include "sld.h"

int lcmp_main(int argc, char* argv[])
{
    if(argc != 4) {
        std::cerr << "Syntax error." << std::endl << "Usage: " << std::endl;
        std::cerr << "    " << argv[0] << " <ckt-orignal> <ckt-encrypted> key=<value>"  << std::endl;
        return 1;
    }
    
    // read ckt1.
    yyin = NULL;
    yyin = fopen(argv[1], "rt");
    if(yyin == NULL) {
        perror(argv[1]);
        return 1;
    }
    if(yyparse() != 0) {
        std::cerr << "Syntax error in " << argv[1] << std::endl;
        return 1;
    }
    ckt_n::ckt_t ckt1(*ast_n::statements);
    delete ast_n::statements;
    fclose(yyin);

    if(ckt1.num_key_inputs() > 0) {
        std::cerr << "Error. The original circuit must not have key inputs." << std::endl;
        return 1;
    }


    // read ckt2
    yyin = NULL;
    yyin = fopen(argv[2], "rt");
    if(yyin == NULL) {
        perror(argv[2]);
        return 1;
    }
    if(yyparse() != 0) {
        std::cerr << "Syntax error in " << argv[1] << std::endl;
        return 1;
    }
    ckt_n::ckt_t ckt2(*ast_n::statements);
    delete ast_n::statements;
    fclose(yyin);


    if(strstr(argv[3], "key=") != argv[3]) {
        std::cout << "Syntax error. Third argument must start with 'key='." << std::endl;
        return 1;
    }
    const char*  keyptr = argv[3] + 4;

    if(ckt2.num_key_inputs() != strlen(keyptr)) {
        std::cerr << "Error. The encrypted circuit is expected to have the same" << std::endl;
        std::cerr << "number of key inputs as the third argument (" << strlen(keyptr) << ")." << std::endl;
        return 1;
    }

    ckt_n::ckt_t c(ckt1, ckt2);
    if(c.check_equiv(keyptr)) {
        std::cout << "equivalent" << std::endl;
    } else {
        std::cout << "different" << std::endl;
    }

    return 0;
}

