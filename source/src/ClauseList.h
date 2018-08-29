#ifndef _CLAUSELIST_H_DEFINED_
#define _CLAUSELIST_H_DEFINED_

#include <map>
#include <vector>
#include <iostream>
#include "util.h"
#include "SATInterface.h"

namespace AllSAT {
    extern const int CL_VERBOSE;

    class ClauseList
    {
    private:
        std::vector<sat_n::Lit> clauses;
        std::vector<int> lengths;

        std::vector<sat_n::Lit> _temp_clause;
        std::vector< std::vector<int> > watches;

        void _addClause(std::vector<sat_n::Lit>& xs, int size);
        void _addWatches(int index, std::vector<sat_n::Lit>& xs, int size);
        void _ensureLength(std::vector<sat_n::Lit>& xs, int size);
    public:
        int verbose;

        ClauseList();
        ~ClauseList();

        //////// creating variables ////////
        void newVar() {
            watches.resize( watches.size() + 2 );
        }
        int nVars() const {
            assert(0 == (watches.size() & 1));
            return (watches.size() >> 1);
        }

        //////// access to  clauses ////////
        unsigned clauseLen(int i) const;
        unsigned numClauses() const { 
            assert(lengths.size() > 0);
            return lengths.size() - 1; 
        }
        sat_n::Lit clauseLit(int cl, int idx) const;

        unsigned numClausesWithLit(sat_n::Lit l) {
            assert(sat_n::toInt(l) >= 0 && sat_n::toInt(l) < watches.size());
            return watches[sat_n::toInt(l)].size();
        }

        int clauseWithLit(sat_n::Lit l, int index) {
            assert(sat_n::toInt(l) >= 0 && sat_n::toInt(l) < watches.size());
            return watches[sat_n::toInt(l)][index];
        }
        std::ostream& dump_clause(std::ostream& out, int index) const;
        ////////////////////////////////////

        int addRewrittenClauses(
            const std::vector<sat_n::lbool>& assumps, 
            const std::vector<bool>& norewriteVarFlags,
            sat_n::Solver& S);

        int addRewrittenClauses(
            std::vector<sat_n::lbool>& values,
            const std::vector<bool>& norewriteVarFlags,
            const std::vector<bool>& dualrailVarFlags,
            sat_n::Solver& S_add,
            sat_n::Solver& S_solve,
            std::map<sat_n::Lit, int>& dualRailLits
        );

        //////// adding clauses ////////
        void addClause(sat_n::Lit x);
        void addClause(sat_n::Lit x, sat_n::Lit y);
        void addClause(sat_n::Lit x, sat_n::Lit y, sat_n::Lit z);
        void addClause(const sat_n::vec_lit_t& xs);
        ////////////////////////////////

    };

}

#endif
