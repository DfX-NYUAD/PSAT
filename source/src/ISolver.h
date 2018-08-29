#ifndef _B2QBF_H_DEFINED_
#define _B2QBF_H_DEFINED_

#include <iostream>
#include <vector>
#include "SATInterface.h"
#include "allsatckt.h"
#include "ClauseList.h"

namespace b2qbf
{
    class ISolver {
    protected:
        // number of clauses.
        int clauses;
        // number of vars.
        int vars;

        // flags for existentially quantified variables.
        std::vector<bool> eq;
        // list of existentially quantified variables.
        std::vector<sat_n::Lit> eqList;

        // flags for universally quantified variables.
        std::vector<bool> uq;
        // list of universally quantified variables.
        std::vector<sat_n::Lit> uqList;

        // flags for the first two levels of quantification.
        std::vector<bool> euq;

        // flags for the remaining (existentially quantified) variables.
        std::vector<bool> eq2;
        // list of the remaining variables.
        std::vector<sat_n::Lit> eq2List;

        // Literal for the output.
        sat_n::Lit output;

        // to be implemented by subclasses.
        virtual void _createVars() = 0;
        virtual void _addClauseToSolvers(const sat_n::vec_lit_t& c) = 0;
        virtual void _addClauseToSolvers(const sat_n::Lit& l) = 0;

        // methods to read DIMACS files.
        void _readClause(std::istream& in);
        void _readList(std::istream& in, 
                       std::vector<bool>& varFlags, 
                       std::vector<sat_n::Lit>& varList);

        void _setList(std::vector<sat_n::Var>& varListIn,
                      std::vector<bool>& vars,
                      std::vector<sat_n::Lit>& litListOut);
    public:
        ISolver();
        virtual ~ISolver();

        // read a DIMACS file and initialize the problem.
        void initialize(std::istream& in);

        void initialize(int nVars, 
            std::vector<sat_n::Var>& eqVars, 
            std::vector<sat_n::Var>& uqVars,
            std::vector<sat_n::Var>& eq2Vars,
            sat_n::Lit output
        );

        void addClause(sat_n::Lit x);
        void addClause(sat_n::Lit x, sat_n::Lit y);
        void addClause(sat_n::Lit x, sat_n::Lit y, sat_n::Lit z);
        void addClause(const sat_n::vec_lit_t& xs);

        // solve is implemented by the subclasses.
        virtual bool solve() = 0;

        // method to retrive solution values.
        virtual sat_n::lbool solutionValue(sat_n::Var vi) = 0;

        // status dumping is implemented by the subclasses too.
        virtual void dump_status(std::ostream& out) = 0;

        // setting parameters is also deferred to the subclasses.
        virtual void setParam(const std::string& param, const std::string& value) = 0;

        // dumping the solution is done by this class.
        std::ostream& dump_solution(std::ostream& out);
    };

}

#endif
