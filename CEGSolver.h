#ifndef _CEGSOLVER_H_DEFINED_
#define _CEGSOLVER_H_DEFINED_

#include "ISolver.h"

namespace b2qbf {
    class CEGSolver : public ISolver {
    private:
        // PRIVATE INSTANCE VARIABLES.

        // the solver object we will use.
        sat_n::Solver S_cand, S_cex;
        sat_n::Solver* S_lastcand;
        // a copy of the initial clauses.
        AllSAT::ClauseList cl;

        // PRIVATE METHODS.
        virtual void _addClauseToSolvers(const sat_n::vec_lit_t& c);
        virtual void _addClauseToSolvers(const sat_n::Lit& l);
        virtual void _createVars();
        bool _findCandidate(sat_n::vec_lit_t& verif_assumps);

    public:
        int numCubes;
        int iterations;

        CEGSolver();
        virtual ~CEGSolver();


        virtual bool solve();
        virtual sat_n::lbool solutionValue(sat_n::Var vi);

        virtual void dump_status(std::ostream& out);
        virtual void setParam(const std::string& param, const std::string& value);
    };
}
#endif
