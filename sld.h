#ifndef _SLD_H_DEFINED_
#define _SLD_H_DEFINED_

#include "node.h"
#include "ckt.h"
#include "dbl.h"
#include "sim.h"
#include "solver.h"
#include <cuddObj.hh>

#include <iostream>

extern int yyparse();
extern FILE* yyin;
extern int verbose;
extern int progress;
extern volatile solver_t* solver;
extern int backbones;
extern int PRINT_INTERVAL;
extern int version;

int print_usage(const char* progname);
void test_ckt(ckt_n::ckt_t& ckt);
void solve(ckt_n::ckt_t& ckt, ckt_n::ckt_t& simckt);
void alarm_handler(int signum);
void setup_timer(void);
void dump_status(void);

void cpux_handler(int signum);
void setup_cpux_handler(void);
void bdd_test();
void printBDD(FILE* out, BDD bdd);

int sld_main(int argc, char* argv[]);
double utimediff(struct rusage* end, struct rusage* start);

int tv_solve(std::map<std::string, int>& keysFound, ckt_n::ckt_t& ckt, ckt_n::ckt_t& sim);
int slice_solve(std::map<std::string, int>& keysFound, ckt_n::ckt_t& ckt, ckt_n::ckt_t& sim);
void dump_keys(std::vector<std::string>& keyNames, std::map<std::string, int>& keysFound);

#endif
