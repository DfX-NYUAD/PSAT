#include "CEGSolver.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <sys/time.h>
#include <sys/resource.h>

namespace b2qbf {

    CEGSolver::CEGSolver() 
    { 
        numCubes = 0; 
        iterations = 0;
        S_lastcand = NULL;
    }
    CEGSolver::~CEGSolver() {}

    void CEGSolver::_addClauseToSolvers(const sat_n::vec_lit_t& c)
    {
        S_cex.addClause(c);
        cl.addClause(c);
    }

    void CEGSolver::_addClauseToSolvers(const sat_n::Lit& l)
    {
        S_cex.addClause(l);
        cl.addClause(l);
    }

    void CEGSolver::_createVars()
    {
        while(S_cand.nVars() <= vars) { 
            S_cand.newVar(); 
            S_cex.newVar();
            cl.newVar(); 
        }
    }

    bool CEGSolver::_findCandidate(sat_n::vec_lit_t& verif_assumps)
    {
        using namespace sat_n;

        bool result;
        if(S_cand.nClauses() > 0) {
            result = S_cand.solve();
            S_lastcand = &S_cand;
        } else {
            result = S_cex.solve(output);
            S_lastcand = &S_cex;
        }
        if(!result) return result;

        for(unsigned i=0; i != eqList.size(); i++) {
            Var vi = var(eqList[i]);
            lbool ri = S_lastcand->modelValue(vi);
            assert(ri.isDef());
            Lit li = (ri.getBool()) ? eqList[i] : ~eqList[i];
            verif_assumps.push(li);
        }
        verif_assumps.push(~output);
        return result;
    }

    bool CEGSolver::solve()
    {
        using namespace sat_n;
        std::vector<lbool> values(vars+1, sat_n::l_Undef);

        while(1) {
            iterations += 1;

            if(((verbose&B2QBF_VERBOSE) != 0)) {
                std::cout << "iteration #" << iterations << std::endl;
            }

            // find a candidate solution.
            vec_lit_t verif_assumps;
            bool result = _findCandidate(verif_assumps);
            if(result == false) return false;
            if(((verbose&B2QBF_VERBOSE) != 0)) {
                std::cout << "found a candidate solution." << std::endl;
            }

            bool cex = S_cex.solve(verif_assumps);
            if(cex == false) {
                verif_assumps[verif_assumps.size()-1] = output;
                bool resolve = S_cex.solve(verif_assumps);
                assert(resolve == true);
                return resolve;
            }
            if(((verbose&B2QBF_VERBOSE) != 0)) {
                std::cout << "found a counter-example." << std::endl;
            }

            for(unsigned i=0; i != uqList.size(); i++) {
                Lit l_i = uqList[i];
                values[var(l_i)] = S_cex.modelValue(var(l_i));
            }
            assert(!sign(output));
            values[var(output)] = sat_n::l_True;

            numCubes += cl.addRewrittenClauses(values, eq, S_cand);
        }
        return false;
    }

    sat_n::lbool CEGSolver::solutionValue(sat_n::Var vi)
    {
        assert(S_lastcand != NULL);
        return S_lastcand->modelValue(vi);
    }

    void CEGSolver::dump_status(std::ostream& out)
    {
        struct rusage ru;
        getrusage(RUSAGE_SELF, &ru);

        double cpu_time = 
            (double) (ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) + 
            (double) (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) * 1e-6;
        double rss = ((double) ru.ru_maxrss) / 1024.0;

        out << "iteration=" << iterations
            << "; cubes=" << numCubes
            << "; cpu_time=" << cpu_time
            << "; maxrss=" << rss << std::endl;
    }

    void CEGSolver::setParam(const std::string& param, const std::string& value)
    {
        using boost::lexical_cast;
        using boost::bad_lexical_cast;

        try {
            std::cerr << "unknown parameter: " << param << std::endl;
            exit(1);
        } catch(const bad_lexical_cast&) {
            std::cerr << "incorrect parameter type or value: " << value << std::endl;
            exit(1);
        }
    }
}
