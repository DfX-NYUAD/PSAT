#ifndef _UTIL_H_DEFINED_
#define _UTIL_H_DEFINED_

#include <iostream>
#include <vector>
#include <set>
#include <cuddObj.hh>
#include "SATInterface.h"

extern int verbose;

extern const int ALLSATCKT_VERBOSE;
extern const int B2QBF_VERBOSE;
extern const int FINDCUBE_VERBOSE;

namespace ckt_n {
#ifdef USE_MINISAT_SOLVER
    std::ostream& operator<<(std::ostream& out, const sat_n::Lit& l);
#endif
    std::ostream& dump_clause(std::ostream& out, const sat_n::vec_lit_t& ls);
    std::ostream& dump_clause(std::ostream& out, const std::vector<sat_n::Lit>& ls);
    std::ostream& dump_clause(std::ostream& out, const std::set<sat_n::Lit>& ls);
    std::ostream& dump_clause(std::ostream& out, BDD bdd);
    std::ostream& operator<<(std::ostream& out, const std::vector<bool>& bs);
}

struct bdd_compare_t {
    bool operator() (const BDD& one, const BDD& two) const {
        return one.getNode() < two.getNode();
    }
};

#endif
