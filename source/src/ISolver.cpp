#include <stdlib.h>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <math.h>

#include <sys/time.h>
#include <sys/resource.h>
#include <boost/lexical_cast.hpp>

#include "ISolver.h"
#include "util.h"

namespace b2qbf
{
    ISolver::ISolver()
        : clauses(0)
        , vars(0)
    {
    }

    void ISolver::initialize(std::istream& in)
    {
        clauses = -1;
        vars = -1;

        // TODO: this stuff needs to be refactored elsewhere so that this can be used
        // in a separate tool.
        std::string word;

        while(in) {
            in >> word;

            // read in coments.
            if(word == "c") {
                std::string line;
                std::getline(in, line);
                std::istringstream sin(line);

                if(sin) {
                    sin >> word;
                    // special comment that lets us know what the output literal is.
                    if(word == "output") {
                        int out;
                        sin >> out;
                        output = sat_n::mkLit(out);
                    // another special comment for debug dumping.
                    } 
                }
            } else if(word == "p") {
                // read in problem information.

                in >> word;
                if(word != "cnf") {
                    fprintf(stderr, "expected 'cnf' after p.\n");
                    exit(1);
                }

                in >> vars >> clauses;
                // create vars.
                // used to be: while(S.nVars() <= vars) { S.newVar(); cl.newVar(); }
                _createVars();

                eq.resize(vars+1, false);
                uq.resize(vars+1, false);
                eq2.resize(vars+1, false);

                // read the list of existentially quantified variables.
                in >> word;
                if(word != "e") {
                    fprintf(stderr, "expected list of existentially quantified variables.\n");
                    exit(2);
                }
                _readList(in, eq, eqList);

                // read the list of universally quantified variables.
                in >> word;
                if(word != "a") {
                    fprintf(stderr, "expected list of universally quantified variables.\n");
                    exit(2);
                }
                _readList(in, uq, uqList);

                assert(eq.size() == uq.size());
                euq.resize(eq.size());
                for(unsigned i=0; i != eq.size(); i++) {
                    euq[i] = eq[i] || uq[i];
                }

                // read the list of remaining variables.
                in >> word;
                if(word != "e") {
                    fprintf(stderr, "expected list of existentially quantified variables.\n");
                    exit(2);
                }
                _readList(in, eq2, eq2List);

                // now read clauses.
                for(int i = 0; i < clauses; i++) {
                    _readClause(in);
                }
                break;
            }
        }
    }

    void ISolver::initialize(int nVars, 
        std::vector<sat_n::Var>& eqVars, 
        std::vector<sat_n::Var>& uqVars,
        std::vector<sat_n::Var>& eq2Vars,
        sat_n::Lit out
    )
    {
        vars = nVars;
        _createVars();

        eq.resize(vars+1, false);
        uq.resize(vars+1, false);
        eq2.resize(vars+1, false);

        _setList(eqVars, eq, eqList);
        _setList(uqVars, uq, uqList);
        _setList(eq2Vars, eq2, eq2List);
        this->output = out;
    }

    void ISolver::addClause(sat_n::Lit x)
    {
        _addClauseToSolvers(x);
        clauses += 1;
    }

    void ISolver::addClause(sat_n::Lit x, sat_n::Lit y)
    {
        using namespace sat_n;
        vec_lit_t clause;
        clause.push(x);
        clause.push(y);
        _addClauseToSolvers(clause);
        clauses += 1;
    }

    void ISolver::addClause(sat_n::Lit x, sat_n::Lit y, sat_n::Lit z)
    {
        using namespace sat_n;
        vec_lit_t clause;
        clause.push(x);
        clause.push(y);
        clause.push(z);
        _addClauseToSolvers(clause);
        clauses += 1;
    }

    void ISolver::addClause(const sat_n::vec_lit_t& xs)
    {
        _addClauseToSolvers(xs);
        clauses += 1;
    }

    void ISolver::_setList(std::vector<sat_n::Var>& varListIn,
                           std::vector<bool>& varFlags,
                           std::vector<sat_n::Lit>& litListOut)
    {
        for(unsigned i=0; i != varListIn.size(); i++) {
            using namespace sat_n;

            int vi = varListIn[i];
            assert(vi >= 0 && vi < (int)varFlags.size());
            varFlags[vi] = true;
            litListOut.push_back(mkLit(vi));
        }
    }

    void ISolver::_readList(std::istream& in, 
                            std::vector<bool>& vars,
                            std::vector<sat_n::Lit>& varList)
    {
        while(1) {
            unsigned var;
            in >> var;
            if(var == 0) break;

            if(var >= vars.size()) {
                std::cerr << "var: " << var << " invalid index." << std::endl;
                exit(1);
            }

            assert(var < vars.size());
            vars[var] = true;
            varList.push_back(sat_n::mkLit(var));
        }
        std::sort(varList.begin(), varList.end());
    }

    void ISolver::_readClause(std::istream& in)
    {
        using namespace sat_n;

        vec_lit_t clause;

        while(1) {
            int num;
            in >> num;
            if(num == 0) break;

            Lit l = (num >= 0) ? mkLit(num) : (~mkLit(-num));
            clause.push(l);
        }
#if 0
        ckt_n::dump_clause(std::cout << "read clause:", clause) << std::endl;
#endif
        _addClauseToSolvers(clause);
    }

    ISolver::~ISolver()
    {
        // destructor doesn't do anything yet!
    }

    std::ostream& ISolver::dump_solution(std::ostream& out)
    {
        for(unsigned i=0; i != eqList.size(); i++) {
            using namespace sat_n;

            lbool vi = solutionValue(sat_n::var(eqList[i]));
            assert(vi.isDef());
            if(!vi.getBool()) {
                out << "-" << sat_n::var(eqList[i]);
            } else {
                out << sat_n::var(eqList[i]);
            }
            out << " ";
        }
        return (out);
    }

}
