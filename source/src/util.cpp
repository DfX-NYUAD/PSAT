#include "util.h"

const int ALLSATCKT_VERBOSE         = 1;
const int B2QBF_VERBOSE             = 2;
const int FINDCUBE_VERBOSE          = 4;

namespace ckt_n {
#ifdef USE_MINISAT_SOLVER
    std::ostream& operator<<(std::ostream& out, const sat_n::Lit& l)
    {
        return (out << (sat_n::sign(l) ? "-" : "") << sat_n::var(l));
    }
#endif
    
    std::ostream& dump_clause(std::ostream& out, const sat_n::vec_lit_t& ls)
    {
        for(unsigned i=0; i != ls.size(); i++) {
            const sat_n::Lit& li = ls[i];
            out << li << " ";
        }
        return out;
    }

    std::ostream& dump_clause(std::ostream& out, const std::vector<sat_n::Lit>& ls)
    {
        for(unsigned i=0; i != ls.size(); i++) {
            out << ls[i] << " ";
        }
        return out;
    }

    std::ostream& dump_clause(std::ostream& out, const std::set<sat_n::Lit>& ls)
    {
        for(std::set<sat_n::Lit>::const_iterator it=ls.begin(); it != ls.end(); it++) {
            out << *it << " ";
        }
        return out;
    }

    std::ostream& dump_clause(std::ostream& out, BDD r) 
    {
        DdManager* mgrPtr = r.manager();
        DdNode* rPtr = r.getNode();
        int *supportIndices = Cudd_SupportIndex(mgrPtr, rPtr);
        int nVars = Cudd_ReadSize(mgrPtr);
        for(int i=0; i < nVars; i++) {
            if(supportIndices[i]) {
                using namespace sat_n;

                Lit li = mkLit(i);
                DdNode *vi = Cudd_bddIthVar(mgrPtr, i);
                DdNode *res = Cudd_bddAnd(mgrPtr, rPtr, vi);
                if(res == Cudd_ReadZero(mgrPtr)) {
                    out << ~li << " ";
                } else {
                    assert(res == vi);
                    out << li << " ";
                }
            }
        }
        free(supportIndices);
        return out;
    }
    std::ostream& operator<<(std::ostream& out, const std::vector<bool>& bs)
    {
        for(unsigned i=0; i != bs.size(); i++) {
            out << (bs[i] ? 1 : 0);
        }
        return out;
    }
}


