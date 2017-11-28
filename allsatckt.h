#ifndef _ALLSATCKT_H_DEFINED
#define _ALLSATCKT_H_DEFINED

#include <set>
#include <map>
#include <vector>
#include <iostream>
#include "SATInterface.h"
#include "ClauseList.h"
#include <cuddObj.hh>
#include "util.h"
#include "Stats.h"

namespace AllSAT {

    class AllSATCkt
    {
        typedef std::set< BDD, bdd_compare_t > bddset_t;
        typedef std::map< sat_n::Lit, bddset_t* > cube_memo_t;
        typedef std::vector< std::vector<int> > impl_graph_t;

        sat_n::Solver S_solve;
        sat_n::Solver S_simp;
        ClauseList clist;
        Cudd mgr;

        sat_n::vec_lit_t solve_assump;
        sat_n::vec_lit_t simp_assump;
        sat_n::vec_lit_t input_values;
        sat_n::vec_lit_t io_clause;

        static void _resize(sat_n::vec_lit_t& vs, unsigned int sz);
        void _init_inputs(
            const sat_n::vec_lit_t& inputs,
            sat_n::vec_lit_t& input_values
        );
        void _simplify(
            const sat_n::vec_lit_t& assump, 
            sat_n::Lit output
        );
        void _verify_unsat(
            const sat_n::vec_lit_t& assump, 
            sat_n::Lit output
        );
        void _simplify(
            const sat_n::vec_lit_t& assumps, 
            const sat_n::vec_lit_t& outputs
        );
        void _verify_unsat(
            const sat_n::vec_lit_t& assumps, 
            const sat_n::vec_lit_t& outputs
        );
        void _dump_state(sat_n::Solver& S, std::ostream& out);

        void _buildImplGraph(
            impl_graph_t& g,
            const std::vector<bool> inputs,
            sat_n::Lit output
        );

        void _deleteMemo(cube_memo_t& memo);
        void _initCubeAndClause();

        bddset_t* 
        _findCube(
            impl_graph_t& g,
            cube_memo_t& memo,
            const std::vector<bool> inputs,
            sat_n::Lit l, 
            std::set<int> visited);

        static void _cartesianProduct(bddset_t& a, bddset_t& b);

    public:
        unsigned int MAX_CUBES_PER_LITERAL;

        sat_n::vec_lit_t cube;
        sat_n::vec_lit_t clause;

        //stack_n::Hist<int> simp_conf_hist, simp_prop_hist;
        //stack_n::Hist<int> solve_conf_hist, solve_prop_hist;

        AllSATCkt();
        ~AllSATCkt();

        sat_n::Solver& getSolver() { return S_solve; }
        sat_n::Solver& getSimplifyingSolver() { return S_simp; }

        sat_n::lbool solverModelValue(sat_n::Var var) const 
        { return S_solve.modelValue(var); }
        sat_n::lbool solverModelValue(sat_n::Lit lit) const 
        { return S_solve.modelValue(lit); }

        void initVector(
            const std::vector<sat_n::Lit>& literals,
            std::vector<sat_n::Lit>& values
        );

        sat_n::Var newVar() { clist.newVar(); S_simp.newVar(); return S_solve.newVar(); }
        int nVars() const { 
            assert(S_simp.nVars() == S_solve.nVars());
            assert(S_simp.nVars() == clist.nVars());
            return S_simp.nVars(); 
        }
        int nClauses() const { 
            return S_solve.nClauses(); 
        }

        void addClause(const sat_n::vec_lit_t& ps);
        void addClause(sat_n::Lit x);
        void addClause(sat_n::Lit x, sat_n::Lit y);
        void addClause(sat_n::Lit x, sat_n::Lit y, sat_n::Lit z);
        void addBlockingClause(const sat_n::vec_lit_t& ps);

        bool solveOnce(sat_n::Lit assump);
        bool solveOnce(sat_n::vec_lit_t& assumps);

        template<class T> void forEachCube(const std::vector<bool>& inputs, sat_n::Lit output, T& op);
        int findBackbones(
            std::vector<bool>& fixedvars,
            const std::vector<sat_n::Lit>& vars,
            const std::vector<sat_n::Lit>& assumps,
            // output is assumed to be the correct output (i.e. not the cex).
            sat_n::Lit output);

        void init_inputs(
            std::vector<sat_n::Lit>& inps1, 
            std::vector<sat_n::Lit>& inps2);
        void init_inputs(std::vector<sat_n::Lit>& inps);

        void init_inputs(
            const sat_n::vec_lit_t& inps, 
            const std::vector<bool>& flags);

        bool solve(
            const sat_n::vec_lit_t& assump, 
            const sat_n::vec_lit_t& inputs,
            sat_n::Lit output
        );

        bool solve(
            const sat_n::vec_lit_t& assump, 
            const sat_n::vec_lit_t& inputs,
            const sat_n::vec_lit_t& outputs
        );

        void simplify(
            const sat_n::vec_lit_t& assump, 
            sat_n::Lit output)
        { _simplify(assump, output); }

        void dump_solver_state(std::ostream& out) {
            _dump_state(S_solve, out);
        }
        void dump_simp_state(std::ostream& out) {
            _dump_state(S_simp, out);
        }
    };


    template<class T> void 
    AllSATCkt::forEachCube(const std::vector<bool>& inputs, sat_n::Lit output, T& op)
    {
        using namespace sat_n;

        std::set<int> visited;
        cube_memo_t memo;
        impl_graph_t g(nVars());
        _buildImplGraph(g, inputs, output);
        bddset_t* s = _findCube(g, memo, inputs, output, visited);
        for(bddset_t::iterator it = s->begin(); it != s->end(); it++) {
            BDD r = *it;
            assert(r != mgr.bddZero());

            unsigned supportSize = r.SupportSize();
            DdManager* mgrPtr = mgr.getManager();
            DdNode* rPtr = r.getNode();
            int *supportIndices = Cudd_SupportIndex(mgrPtr, rPtr);
            int nVars = Cudd_ReadSize(mgrPtr);

            vec_lit_t result;
            for(int i=0; i < nVars; i++) {
                if(supportIndices[i]) {
                    Var vi = (Var) i;
                    assert(solverModelValue(vi).isDef());
                    Lit li = (solverModelValue(vi).getBool()) ? mkLit(vi) : ~mkLit(vi);
                    result.push(li);
                }
            }
            assert(result.size() == supportSize);
            op(result);
            free(supportIndices);
        }
        _deleteMemo(memo);
    }
}
#endif
