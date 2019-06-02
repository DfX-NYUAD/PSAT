#include <iostream>
#include "ckt.h"
#include "sld.h"
#include "simplify.h"
#include "SATInterface.h"

int simplify_main(int argc, char* argv[])
{
    if(argc < 2 || argc > 3) {
        std::cerr << "Syntax error. Usage: "
                  << argv[0] << " <bench-file> [key=values]" << std::endl;
        std::cerr << "Unspecified key defaults to all zeros." << std::endl;
        return 1;
    }

    yyin = fopen(argv[1], "rt");
    if(yyin == NULL) {
        perror(argv[1]);
        return 1;
    }
    if(yyparse() == 0) {
        using namespace ast_n;
        ckt_n::ckt_t ckt(*statements);
        std::vector<sat_n::lbool> keyvector(ckt.num_key_inputs(), sat_n::l_False);
        if(argc == 3) {
            if(strstr(argv[2], "key=") != argv[2]) {
                std::cerr << "Error. Key argument start with 'key='." << std::endl;
                return 1;
            }
            const char* keys = argv[2]+4;
            if(strlen(keys) != ckt.num_key_inputs()) {
                std::cerr << "Error. Number of keys doesn't match key argument." 
                          << std::endl;
                return 1;
            }
            for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
                if(keys[i] == '1') {
                    keyvector[i] = sat_n::l_True;
                } else if(keys[i] == '0') {
                    keyvector[i] = sat_n::l_False;
                } else if(keys[i] == 'x' || keys[i] == 'X') {
                    keyvector[i] = sat_n::l_Undef;
                } else {
                    std::cout << "Error. unrecognized key value: '" << keys[i] 
                              << "'. Must be 0, 1 or x." << std::endl;
                    return 1;
                }
            }
        }
        ckt.rewrite_keys(keyvector);
        std::cout << ckt << std::endl;
    }
    return 0;
}
