#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <algorithm>
#include <boost/algorithm/string/predicate.hpp>
#include "ast.h"
#include "ckt.h"
#include "dbl.h"
#include "sim.h"
#include "sld.h"
#include "util.h"
#include "delist.h"
#include "solver.h"
#include "tvsolver.h"
#include <cudd.h>
#include <cuddObj.hh>

int verbose = 0;
int progress = 0;
int allsat_max_iter = -1;
int backbones = 1;
int PRINT_INTERVAL= 5;
int tvs_en = 0;
int print_info = 0;
int slice = 0;
int tv_quit = 0;
int more_keys = 1;

volatile solver_t* solver = NULL;
std::string known_keystring;

int sld_main(int argc, char* argv[]) 
{
    char c;
    int cpu_limit = -1;
    int64_t data_limit = -1;

    while ((c = getopt (argc, argv, "ihvptTc:m:k:sN:")) != -1) {
        switch (c) {
            case 'h':
                return print_usage(argv[0]);
                break;
            case 's':
                slice = 1;
                break;
            case 't':
                tvs_en = 1;
                break;
            case 'T':
                tv_quit = 1;
                break;
            case 'i':
                print_info = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'c':
                cpu_limit = atoi(optarg);
                break;
            case 'm':
                data_limit = ((int64_t) atoi(optarg)) * ((int64_t)1048576);
                break;
            case 'p':
                progress = 1;
                break;
            case 'k':
                known_keystring = optarg;
                break;
            case 'N':
                more_keys = atoi(optarg);
                break;
            default:
                break;
        }
    }


    // check if we got a test article.
    if(optind != argc-2) {
        return print_usage(argv[0]);
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

    // try to open the verilog file.
    //
    // JOHANN
    // the encrypted benchfile
    yyin = fopen(argv[optind], "rt");
    if(yyin == NULL) {
        perror(argv[optind]);
        return 1;
    }

    if(yyparse() == 0) {
        using namespace ast_n;
        ckt_n::ckt_t ckt(*statements);
        delete statements;
        fclose(yyin);

        // read the circuit for simulation.
	//
	// JOHANN
	// the original benchfile
        yyin = fopen(argv[optind+1], "rt");
        if(yyin == NULL) {
            perror(argv[optind+1]);
            return 1;
        }
        if(yyparse() != 0) {
            return 1;
        }
        ckt_n::ckt_t simckt(*statements);
        delete statements;
        fclose(yyin);

        if(simckt.num_key_inputs() != 0) {
            std::cout << "Error. Circuit for simulation musn't have key inputs." << std::endl;
            return 1;
        }

        if(!simckt.compareIOs(ckt, 1)) {
            std::cout << "Error. Original (simulation) and encrypted designs don't match." << std::endl;
            return 1;
        }


        // test_ckt(ckt);
        if(progress) {
            setup_timer();
        }

        if(print_info) {
            std::cout << argv[optind] << " " << ckt.num_ckt_inputs() << " " << ckt.num_outputs()
                      << " " << ckt.num_gates() <<  " " << ckt.num_key_inputs() << std::endl;
        } else {
            solve(ckt, simckt);
        }
    }

    return 0;
}

void debug_g311(ckt_n::ckt_t& ckt)
{
    using namespace ckt_n;
    using namespace sat_n;

    Solver S;
    index2lit_map_t lmap;
    ckt.init_solver(S, lmap);

    int out_index = -1;
    for(unsigned i=0; i != ckt.num_outputs(); i++) {
        if(ckt.outputs[i]->name == "G311") {
            out_index = i;
            break;
        }
    }
    assert(out_index != -1);
    node_t* out = ckt.nodes[out_index];
    Lit l_out = ckt.getLit(lmap, out);

    bool result = S.solve(l_out);
    assert(result);

    for(unsigned i=0; i != ckt.num_ckt_inputs(); i++) {
        Lit l_inp = ckt.getLit(lmap, ckt.ckt_inputs[i]);
        std::cout << std::setw(6) << ckt.ckt_inputs[i]->name << ":" << S.modelValue(l_inp).getBool() << " ";
        if((i+1) % 8 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;
}

int tv_solve(std::map<std::string, int>& keysFound, ckt_n::ckt_t& ckt, ckt_n::ckt_t& simckt)
{
    using namespace ckt_n;

    ckt_eval_t sim(simckt, simckt.ckt_inputs);
    int newlyFound = 0, iter = 0, foundCnt = 0;
    do {

        // find some new keys.
        tv_solver_t tvs(ckt, sim);
        std::map<std::string, int> newKeysFound;
        tvs.solveSingleKeys(newKeysFound);
        // count how many we found.
        newlyFound = newKeysFound.size();
        // get rid of them in the ckt.
        ckt.rewrite_keys(newKeysFound);

        // now copy them to the result map.
        for(auto it=newKeysFound.begin(); it != newKeysFound.end(); it++) {
            assert(keysFound.find(it->first) == keysFound.end());
            keysFound[it->first] = it->second;
        }
        iter += 1;
        foundCnt += newlyFound;
    } while(newlyFound > 0);
    std::cout << "tv_solve iter=" << iter << "; cnt=" << foundCnt << std::endl;
    return foundCnt;
}

int slice_solve(std::map<std::string, int>& keysFound, ckt_n::ckt_t& ckt, ckt_n::ckt_t& simckt)
{
    int iter=0;
    int maxNodes = ckt.num_nodes() / 10 + 1;
    int maxKeys = -1;
    int cnt = 0;
    while(true) {
        std::cout << "slice iteration #" << iter++ << "; keysFound=" << keysFound.size() 
            << "; keysTotal=" << ckt.num_key_inputs() 
            << std::endl;

        // do the sliced solving.
        std::map<std::string, int> newKeysFound;
        solver_t S(ckt, simckt, verbose);
        if(S.sliceAndSolve(newKeysFound, maxKeys, maxNodes) == 0) {
            std::cout << "infeasible cplex model. " << std::endl;
            break;
        }
        cnt += newKeysFound.size();

        // copy the keys into the result map.
        for(auto it=newKeysFound.begin(); it != newKeysFound.end(); it++) {
            assert(keysFound.find(it->first) == keysFound.end());
            keysFound[it->first] = it->second;
        }

        // if necessary do the test vector solver.
        int tv_found=0;
        if(tvs_en) {
            tv_found = tv_solve(keysFound, ckt, simckt);
            cnt += tv_found;
        }
        if(newKeysFound.size() == 0 && tv_found == 0) {
            std::cout << "no new keys found." << std::endl;
            break;
        }
    }
    return cnt;
}
void solve(ckt_n::ckt_t& ckt, ckt_n::ckt_t& simckt) 
{
    using namespace ckt_n;
    using namespace AllSAT;

    std::cout << "inputs=" << ckt.num_ckt_inputs() 
        << " keys=" << ckt.num_key_inputs() 
        << " outputs=" << ckt.num_outputs()
        << " gates=" << ckt.num_gates()
        << std::endl;

    ckt.cleanup();

    // create an array of key names.
    std::vector<std::string> keyNames(ckt.num_key_inputs());
    for(unsigned i=0; i != ckt.num_key_inputs(); i++) {
        keyNames[i] = ckt.key_inputs[i]->name;
    }


    if(known_keystring.size()) {
        // TODO: refactor this to use maps.
        if(known_keystring.size() != ckt.num_key_inputs()) {
            std::cerr << "Error. Expected the known key string to have same size as number of key inputs." << std::endl;
            exit(1);
        }
        std::map<std::string, int> knownKeys;
        for(unsigned i=0; i != known_keystring.size(); i++) {
            if(known_keystring[i] == 'x') { continue; }
            else {
                if(known_keystring[i] == '1') {
                    knownKeys[ckt.key_inputs[i]->name] = 1;
                } else if(known_keystring[i] == '0') {
                    knownKeys[ckt.key_inputs[i]->name] = 0;
                } else {
                    std::cerr << "Error. Unexpected character: '" << known_keystring[i] << "'." << std::endl;
                    exit(1);
                }
            }
        }
        ckt.rewrite_keys(knownKeys);
    }

    // ckt.dump_cuts(5, 3);

    // test vector based solver.
    std::map<std::string, int> keysFound;
    if(tvs_en) {
        tv_solve(keysFound, ckt, simckt);
        if(tv_quit) exit(0);
    }

    if(slice) {
        slice_solve(keysFound, ckt, simckt);
        std::cout << "num_keys_found=" << keysFound.size() << std::endl;
        dump_keys(keyNames, keysFound);
    } 

    solver_t S(ckt, simckt, verbose);
    solver = &S;
    S.solve(solver_t::SOLVER_V0, keysFound, false);
    dump_keys(keyNames, keysFound);
    for(int i=1; i < more_keys; i++) {
        S.blockKey(keysFound);
        if(!S.getNewKey(keysFound)) break;
        dump_keys(keyNames, keysFound);
    }

    dump_status();
    solver = NULL;
}

void dump_keys(std::vector<std::string>& keyNames, std::map<std::string, int>& keysFound)
{
    std::cout << "key=";
    for(unsigned i=0; i != keyNames.size(); i++) {
        auto pos = keysFound.find(keyNames[i]);
        if(pos != keysFound.end()) {
            std::cout << pos->second;
        } else {
            std::cout << 'x';
        }
    }
    std::cout << std::endl;
}

int print_usage(const char* progname)
{
    std::cout << "Usage: " << progname << " [options] <encrypted-bench-file> <original-bench-file>" 
              << std::endl;
    std::cout << "Options may be one of the following." << std::endl;
    std::cout << "    -h            : this message." << std::endl;
    std::cout << "    -t            : enable test vector solver." << std::endl;
    std::cout << "    -v            : verbose output." << std::endl;
    std::cout << "    -p            : report progress." << std::endl;
    std::cout << "    -c            : CPU time limit (s)." << std::endl;
    std::cout << "    -m            : mem usage limit (MB)." << std::endl;
    std::cout << "    -k <keystr>   : provide known keys." << std::endl;
    std::cout << "    -s            : enable slicing and dicing." << std::endl;
    std::cout << "    -N            : extract N keys (default=1)." << std::endl;

    return 0;
}

void dump_status(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);

    double cpu_time = 
        (double) (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + 
        (double) (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) * 1e-6;
    double rss = ((double) ru.ru_maxrss) / 1024.0;

    int iter = 0, backbones_count = 0, cube_count = 0;
    if(solver != NULL) {
        iter = solver->iter;
        backbones_count = solver->backbones_count;
        cube_count = solver->cube_count;
    }

    std::cout << "iteration=" << iter
              << "; backbones_count=" << backbones_count
              << "; cube_count=" << cube_count
              << "; cpu_time=" << cpu_time
              << "; maxrss=" << rss << std::endl;
}

void alarm_handler(int signum)
{
    dump_status();
    alarm(PRINT_INTERVAL);
}

void setup_timer(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = alarm_handler;
    if(sigaction(SIGALRM, &sa, NULL) != 0) {
        std::cerr << "Unable to set up alarm signal." << std::endl;
        return;
    }
    alarm(PRINT_INTERVAL);
}

void cpux_handler(int signum)
{
    std::cout << "timeout" << std::endl;
    dump_status();
    exit(1);
}

void setup_cpux_handler(void)
{
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = cpux_handler;
    if(sigaction(SIGXCPU, &sa, NULL) != 0) {
        std::cerr << "Unable to set up CPUX signal." << std::endl;
        return;
    }
}

void bdd_test()
{
    Cudd mgr;
    BDD x1 = mgr.bddVar(0), x2 = mgr.bddVar(1), k1 = mgr.bddVar(2), k2 = mgr.bddVar(3);
    BDD fn = !((x1 ^ k1) + (!(x2 ^ k2)));

    BDD c0 = !k1 & !k2;
    BDD c1 = !k1 & k2;
    BDD c2 = k1 & !k2;
    BDD c3 = k1 & k2;

    BDD fn0 = fn.Cofactor(c0);
    BDD fn1 = fn.Cofactor(c1);
    BDD fn2 = fn.Cofactor(c2);
    BDD fn3 = fn.Cofactor(c3);

    printBDD(stdout, fn);
    printBDD(stdout, fn0);
    printBDD(stdout, fn1);
    printBDD(stdout, fn2);
    printBDD(stdout, fn3);
}

void printBDD(FILE* out, BDD bdd)
{
    DdNode* node = bdd.getNode();
    Cudd_DumpFactoredForm(bdd.manager(), 1, &node, NULL, NULL, out); 
    fprintf(out, "\n");
}

double utimediff(struct rusage* end, struct rusage* start)
{
    double tend = end->ru_utime.tv_usec*1e-6 + end->ru_utime.tv_sec;
    double tstart = start->ru_utime.tv_usec*1e-6 + start->ru_utime.tv_sec;
    return tend-tstart;
}

