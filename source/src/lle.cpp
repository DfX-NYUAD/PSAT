#include "lle.h"
#include "lutencoder.h"
#include <math.h>
#include "sld.h"

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>


int lle_main(int argc, char* argv[])
{
    int cpu_limit = -1;
    int64_t data_limit = -1;
    bool extended = false;
    std::string output_file;
    int target_keys = -1;


    int c;
    while ((c = getopt (argc, argv, "hc:m:o:k:e")) != -1) {
        switch (c) {
            case 'h':
                return lle_usage(argv[0]);
                break;
            case 'c':
                cpu_limit = atoi(optarg);
                break;
            case 'm':
                data_limit = ((int64_t) atoi(optarg)) * ((int64_t)1048576);
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'k':
                target_keys = atoi(optarg);
                break;
            case 'e':
                extended = true;
                break;
            default:
                break;
        }
    }


    // check if we got a test article.
    if(optind == argc || optind != argc-1) {
        return lle_usage(argv[0]);
    }

    if(cpu_limit != -1) {
        setup_cpux_handler();

        struct rlimit rl;
        rl.rlim_cur = cpu_limit;
        setrlimit(RLIMIT_CPU, &rl);
    }

    if(data_limit != -1) {
        struct rlimit rl;
        rl.rlim_cur = rl.rlim_max = data_limit;
        if(setrlimit(RLIMIT_AS, &rl) != 0) {
            std::cerr << "trouble setting data limit." << std::endl;
        }
    }

    std::cout << "file to encrypt: " << argv[optind] << std::endl;

    yyin = fopen(argv[optind], "rt");
    if(yyin == NULL) {
        perror(argv[optind]);
        return 1;
    }

    if(yyparse() == 0) {
        using namespace ast_n;
        ckt_n::lut_encoder_t enc(*statements, 49371031 /* picked something for a seed. */);
        int num_keys = target_keys == -1 ? (int) (ceil(enc.ckt.num_gates() * 0.2 )) : target_keys;
        std::cout << "target keys=" << num_keys << std::endl;
        std::cout << "keys added=" << enc.encode(num_keys, output_file, extended) << std::endl;
    }


    return 0;
}

int lle_usage(const char* progname)
{
    std::cout << "Usage: " << progname << " [options] <bench-file>" 
              << std::endl;
    std::cout << "Options may be one of the following." << std::endl;
    std::cout << "    -h            : this message." << std::endl;
    std::cout << "    -o <filename> : output file." << std::endl;
    std::cout << "    -k <keys>     : number of keys to introduces (default=10% of num_gates)." << std::endl;
    std::cout << "    -c <value>    : CPU time limit (s)." << std::endl;
    std::cout << "    -m <value>    : mem usage limit (MB)." << std::endl;
    return 0;
}
