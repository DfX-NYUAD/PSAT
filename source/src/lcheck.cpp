#include "lcheck.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "ckt.h"
#include "sim.h"
#include "tvsolver.h"

int lcheck_main(int argc, char* argv[])
{
    using namespace ckt_n;
    using namespace AllSAT;

    std::string graph_out_file;
    int c;
    while ((c = getopt (argc, argv, "ho:")) != -1) {
        switch (c) {
            case 'h':
                print_lcheck_usage(argv[0]);
                return 1;
                break;
            case 'o':
                graph_out_file = optarg;
                break;
            default:
                std::cerr << "Unknown option: '" << c << "'" << std::endl;
                print_lcheck_usage(argv[0]);
                return 1;
        }
    }

    if(optind == argc || optind != argc-1) {
        print_lcheck_usage(argv[0]);
        return 1;
    }

    yyin = fopen(argv[optind], "rt");
    if(yyin == NULL) {
        perror(argv[optind]);
        return 1;
    }

    if(yyparse() == 0) {
        using namespace ast_n;
        ckt_n::ckt_t ckt(*statements);

        ckt_eval_t sim(ckt, ckt.ckt_inputs);

        tv_solver_t tvs(ckt, sim);
        std::map<std::string, int> keysFound;
        tvs.solveSingleKeys(keysFound);
        std::cout << "solveSingleKeys found " << keysFound.size() << " keys." << std::endl;

        int cnt = -1;
        if(graph_out_file.size()) {
            std::ofstream fout(graph_out_file.c_str());
            if(fout) {
                cnt = tvs.countNonMutablePairs(fout);
            } else {
                std::cerr << "Error writing file: " << graph_out_file << std::endl;
                cnt = tvs.countNonMutablePairs(std::cout);
            }
        } else {
            cnt = tvs.countNonMutablePairs(std::cout);
        }
        std::cout << "# of keys=" << ckt.num_key_inputs() << std::endl;
        std::cout << "# of non-mutable pairs=" << cnt << std::endl;
    } else {
        std::cerr << "Parsing error in: " << argv[optind] << std::endl;
        return 1;
    }

    return 0;
}

void print_lcheck_usage(const char* argv0)
{
    std::cerr << "Syntax error. Usage: " << std::endl;
    std::cerr << "    " << argv0 << " [-o non-mutability-graph-file] <bench>" << std::endl;
}



